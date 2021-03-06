#include "../AnalysisTools.h"

void Help(char cmd[50], bool error) { //{{{
  FILE *ptr;
  if (error) {
    ptr = stderr;
  } else {
    ptr = stdout;
    fprintf(stdout, "\
DensityMolecules utility calculates number density for all bead types from \
the centre of mass (or any bead in a molecule) of specified molecules \
similarly to how DensityAggregates calculates the density for \
aggregates.\n\n");
  }

  fprintf(ptr, "Usage:\n");
  fprintf(ptr, "   %s <input> <width> <output.rho> <mol name(s)> <options>\n\n", cmd);

  fprintf(ptr, "   <input>            input filename (either vcf or vtf format)\n");
  fprintf(ptr, "   <width>            width of a single bin\n");
  fprintf(ptr, "   <output.rho>       output density file (automatic ending 'molecule_name.rho' added)\n");
  fprintf(ptr, "   <mol name(s)>      molecule name(s) to calculate density for\n");
  fprintf(ptr, "   <options>\n");
  fprintf(ptr, "      --joined        specify that <input> contains joined coordinates\n");
  fprintf(ptr, "      -n <int>        number of bins to average\n");
  fprintf(ptr, "      -st <int>       starting timestep for calculation\n");
  fprintf(ptr, "      -e <end>        ending timestep for calculation\n");
  fprintf(ptr, "      -c <name> <int> use <int>-th molecule bead instead of centre of mass\n");
  CommonHelp(error);
} //}}}

int main(int argc, char *argv[]) {

  // -h/--version options - print stuff and exit //{{{
  if (VersionOption(argc, argv)) {
    exit(0);
  }
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0) {
      Help(argv[0], false);
      exit(0);
    }
  }
  int req_args = 4; //}}}

  // check if correct number of arguments //{{{
  int count = 0;
  for (int i = 1; i < argc && argv[count+1][0] != '-'; i++) {
    count++;
  }

  if (count < req_args) {
    ErrorArgNumber(count, req_args);
    Help(argv[0], true);
    exit(1);
  } //}}}

  // test if options are given correctly //{{{
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-' &&
        strcmp(argv[i], "-i") != 0 &&
        strcmp(argv[i], "-v") != 0 &&
        strcmp(argv[i], "--silent") != 0 &&
        strcmp(argv[i], "-h") != 0 &&
        strcmp(argv[i], "--script") != 0 &&
        strcmp(argv[i], "--version") != 0 &&
        strcmp(argv[i], "--joined") != 0 &&
        strcmp(argv[i], "-n") != 0 &&
        strcmp(argv[i], "-st") != 0 &&
        strcmp(argv[i], "-e") != 0 &&
        strcmp(argv[i], "-c") != 0) {

      ErrorOption(argv[i]);
      Help(argv[0], true);
      exit(1);
    }
  } //}}}

  count = 0; // count mandatory arguments

  // <input> - input coordinate file //{{{
  char input_coor[LINE];
  strcpy(input_coor, argv[++count]);

  // test if <input> filename ends with '.vcf' or '.vtf' (required by VMD)
  int ext = 2;
  char extension[2][5];
  strcpy(extension[0], ".vcf");
  strcpy(extension[1], ".vtf");
  if (ErrorExtension(input_coor, ext, extension)) {
    Help(argv[0], true);
    exit(1);
  }
  // if vtf, copy to input_vsf
  char *input_vsf = calloc(LINE,sizeof(char));
  if (strcmp(strrchr(input_coor, '.'),".vtf") == 0) {
    strcpy(input_vsf, input_coor);
  } else {
    strcpy(input_vsf, "traject.vsf");
  } //}}}

  // <width> - number of starting timestep //{{{
  // Error - non-numeric argument
  if (!IsInteger(argv[++count])) {
    ErrorNaN("<width>");
    Help(argv[0], true);
    exit(1);
  }
  double width = atof(argv[count]); //}}}

  // <output.rho> - filename with bead densities //{{{
  char output_rho[LINE];
  strcpy(output_rho, argv[++count]); //}}}

  // variables - structures //{{{
  BEADTYPE *BeadType; // structure with info about all bead types
  MOLECULETYPE *MoleculeType; // structure with info about all molecule types
  BEAD *Bead; // structure with info about every bead
  int *Index; // link between indices in vsf and in program (i.e., opposite of Bead[].Index)
  MOLECULE *Molecule; // structure with info about every molecule
  COUNTS Counts = InitCounts; // structure with number of beads, molecules, etc. //}}}

  // options before reading system data //{{{
  bool silent;
  bool verbose;
  bool script;
  CommonOptions(argc, argv, &input_vsf, &verbose, &silent, &script);

  // are provided coordinates joined? //{{{
  bool joined = BoolOption(argc, argv, "--joined"); //}}}

  // starting timestep //{{{
  int start = 1;
  if (IntegerOption(argc, argv, "-st", &start)) {
    exit(1);
  } //}}}

  // ending timestep //{{{
  int end = -1;
  if (IntegerOption(argc, argv, "-e", &end)) {
    exit(1);
  } //}}}

  // error if ending step is lower than starging step //{{{
  if (end != -1 && start > end) {
    fprintf(stderr, "\033[1;31m");
    fprintf(stderr, "\nError: Starting step (%d) is higher than ending step (%d)\n", start, end);
    fprintf(stderr, "\033[0m");
    exit(1);
  } //}}}

  // number of bins to average //{{{
  int avg = 1;
  if (IntegerOption(argc, argv, "-n", &avg)) {
    exit(1);
  } //}}}
  //}}}

  // print command to stdout //{{{
  if (!silent) {
    PrintCommand(stdout, argc, argv);
  } //}}}

  // read system information
  bool indexed = ReadStructure(input_vsf, input_coor, &Counts, &BeadType, &Bead, &Index, &MoleculeType, &Molecule);

  // vsf file is not needed anymore
  free(input_vsf);

  // <molecule names> - types of molecules for calculation //{{{
  while (++count < argc && argv[count][0] != '-') {

    int mol_type = FindMoleculeType(argv[count], Counts, MoleculeType);

    if (mol_type == -1) {
      fprintf(stderr, "\033[1;31m");
      fprintf(stderr, "\nError: \033[1;33m%s\033[1;31m", input_coor);
      fprintf(stderr, " - non-existent molecule \033[1;33m%s\033[1;31m", argv[count]);
      fprintf(stderr, "\033[0m");
      ErrorMoleculeType(Counts, MoleculeType);
      exit(1);
    } else {
      MoleculeType[mol_type].Use = true;
    }

    // write initial stuff to output density file //{{{
    FILE *out;

    char str2[1050];
    sprintf(str2, "%s%s.rho", output_rho, argv[count]);
    if ((out = fopen(str2, "w")) == NULL) {
      ErrorFileOpen(str2, 'w');
      exit(1);
    }

    // print command to output file
    putc('#', out);
    PrintCommand(out, argc, argv);

    // print bead type names to output file //{{{
    fprintf(out, "# for each bead type: (1) rdp; (2) stderr; (3) rnp; (4) stderr\n");
    fprintf(out, "# columns: (1) distance;");
    for (int i = 0; i < Counts.TypesOfBeads; i++) {
      fprintf(out, " (%d) %s", 4*i+2, BeadType[i].Name);
      if (i != (Counts.TypesOfBeads-1)) {
        putc(';', out);
      }
    }
    putc('\n', out); //}}}

    fclose(out); //}}}
  } //}}}

  // -c option - specify which bead to use as a molecule centre //{{{
  // array for considering whether to use COM or specified bead number //{{{
  int centre[Counts.TypesOfMolecules];
  for (int i = 0; i < Counts.TypesOfMolecules; i++) {
    centre[i] = -1; // helper value
  } //}}}

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-c") == 0) {
      int j = 1; // count extra arguments
      while ((i+j) < argc && argv[i+j][0] != '-') {

        int mol_type = FindMoleculeType(argv[i+j], Counts, MoleculeType);

        if (mol_type == -1) {
          fprintf(stderr, "\033[1;31m");
          fprintf(stderr, "\nError: \033[1;33m-c\033[1;31m option - non-existent molecule \033[1;33m%s\033[1;31m\n", argv[i+j]);
          fprintf(stderr, "\033[0m");
          ErrorMoleculeType(Counts, MoleculeType);
          exit(1);
        } else {
          // Error - non-numeric argument //{{{
          if (!IsInteger(argv[i+j+1])) {
            ErrorNaN("-c");
            Help(argv[0], true);
            exit(1);
          } //}}}

          MoleculeType[mol_type].Use = true;

          centre[mol_type] = atoi(argv[i+j+1]) - 1; // bead indices start from 0

          // Error - too high bead number //{{{
          if (centre[mol_type] > MoleculeType[mol_type].nBeads) {
            fprintf(stderr, "\033[1;31m");
            fprintf(stderr, "\nError: \033[1;33m-c\033[1;31m option - bead index \033[1;33m%d\033[1;31m", centre[mol_type]+1);
            fprintf(stderr, " is too high for molecule \033[1;33m%s\033[1;31m", MoleculeType[mol_type].Name);
            fprintf(stderr, "\033[0m");
            exit(1);
          } //}}}
        }

        j += 2; // +2 because there's "<mol name> number"
      }
    }
  } //}}}

  // open input coordinate file //{{{
  FILE *vcf;
  if ((vcf = fopen(input_coor, "r")) == NULL) {
    ErrorFileOpen(input_coor, 'r');
    exit(1);
  } //}}}

  VECTOR BoxLength = GetPBC(vcf, input_coor);

  // number of bins //{{{
  double max_dist = 0.5 * Min3(BoxLength.x, BoxLength.y, BoxLength.z);
  int bins = ceil(max_dist / width); //}}}

  // allocate memory for density arrays //{{{
  double ***rho = malloc(Counts.TypesOfBeads*sizeof(double **));
  double ***rho_2 = malloc(Counts.TypesOfBeads*sizeof(double **));
  for (int i = 0; i < Counts.TypesOfBeads; i++) {
    rho[i] = malloc(Counts.TypesOfMolecules*sizeof(double *));
    rho_2[i] = malloc(Counts.TypesOfMolecules*sizeof(double *));
    for (int j = 0; j < Counts.TypesOfMolecules; j++) {
      rho[i][j] = calloc(bins,sizeof(double));
      rho_2[i][j] = calloc(bins,sizeof(double));
    }
  } //}}}

  // create array for the first line of a timestep ('# <number and/or other comment>')
  char *stuff = calloc(LINE, sizeof(char));

  // print information - verbose output //{{{
  if (verbose) {
    VerboseOutput(input_coor, Counts, BoxLength, BeadType, Bead, MoleculeType, Molecule);
    fprintf(stdout, "Chosen molecule types:");
    for (int i = 0; i < Counts.TypesOfMolecules; i++) {
      if (MoleculeType[i].Use) {
        fprintf(stdout, " %s", MoleculeType[i].Name);
      }
    }
    putchar('\n');
  } //}}}

  // skip first start-1 steps //{{{
  count = 0;
  int test;
  for (int i = 1; i < start && (test = getc(vcf)) != EOF; i++) {
    ungetc(test, vcf);

    count++;

    // print step? //{{{
    if (!silent && !script) {
      fflush(stdout);
      fprintf(stdout, "\rDiscarding step: %d", count);
    } //}}}

    if (SkipCoor(vcf, Counts, &stuff)) {
      fprintf(stderr, "\033[1;31m");
      fprintf(stderr, "Error: premature end of \033[1;33m%s\033[1;31m file\n\n", input_coor);
      fprintf(stderr, "\033[0m");
      exit(1);
    }
  }
  // print number of starting step? //{{{
  if (!silent) {
    if (script) {
      fprintf(stdout, "Starting step: %d\n", start);
    } else {
      fflush(stdout);
      fprintf(stdout, "\r                          ");
      fprintf(stdout, "\rStarting step: %d\n", start);
    }
  } //}}}
  // is the vcf file continuing?
  if (ErrorDiscard(start, count, input_coor, vcf)) {
    exit(1);
  }
  //}}}

  // main loop //{{{
  count = 0; // count timesteps
  int count_vcf = start - 1;
  while ((test = getc(vcf)) != EOF) {
    ungetc(test, vcf);

    count++;
    count_vcf++;

    // print step? //{{{
    if (!silent && !script) {
      fflush(stdout);
      fprintf(stdout, "\rStep: %d", count_vcf);
    } //}}}

    // read coordinates //{{{
    if ((test = ReadCoordinates(indexed, vcf, Counts, Index, &Bead, &stuff)) != 0) {
      // print newline to stdout if Step... doesn't end with one
      ErrorCoorRead(input_coor, test, count_vcf, stuff);
      exit(1);
    } //}}}

    // join molecules if un-joined coordinates provided //{{{
    if (!joined) {
      RemovePBCMolecules(Counts, BoxLength, BeadType, &Bead, MoleculeType, Molecule);
    } //}}}

    // allocate memory for temporary density arrays //{{{
    double ***temp_rho = malloc(Counts.TypesOfBeads*sizeof(double **));
    for (int i = 0; i < Counts.TypesOfBeads; i++) {
      temp_rho[i] = malloc(Counts.TypesOfMolecules*sizeof(double *));
      for (int j = 0; j < Counts.TypesOfMolecules; j++) {
        temp_rho[i][j] = calloc(bins,sizeof(double));
      }
    } //}}}

    // calculate densities //{{{
    for (int i = 0; i < Counts.Molecules; i++) {
      int mol_type_i = Molecule[i].Type;
      if (MoleculeType[mol_type_i].Use) {

        // determine centre to calculate densities from //{{{
        VECTOR com;
        if (centre[mol_type_i] == -1 ) { // use molecule's centre of mass
          com = CentreOfMass(MoleculeType[mol_type_i].nBeads, Molecule[i].Bead, Bead, BeadType);
        } else { // use centre[mol_type_i]-th molecule's bead as com
          com.x = Bead[Molecule[i].Bead[centre[mol_type_i]]].Position.x;
          com.y = Bead[Molecule[i].Bead[centre[mol_type_i]]].Position.y;
          com.z = Bead[Molecule[i].Bead[centre[mol_type_i]]].Position.z;
        } //}}}

        // free temporary density array //{{{
        for (int j = 0; j < Counts.TypesOfBeads; j++) {
          for (int k = 0; k < Counts.TypesOfMolecules; k++) {
            for (int l = 0; l < bins; l++) {
              temp_rho[j][k][l] = 0;
            }
          }
        } //}}}

        // molecule beads //{{{
        for (int j = 0; j < MoleculeType[mol_type_i].nBeads; j++) {
          int bead_j = Molecule[i].Bead[j];

          VECTOR dist = Distance(Bead[bead_j].Position, com, BoxLength);
          dist.x = Length(dist);

          if (dist.x < max_dist) {
            int k = dist.x / width;

            temp_rho[Bead[bead_j].Type][mol_type_i][k]++;
          }
        } //}}}

        // monomeric beads //{{{
        for (int j = 0; j < Counts.Unbonded; j++) {
          VECTOR dist = Distance(Bead[j].Position, com, BoxLength);
          dist.x = Length(dist);

          if (dist.x < max_dist) {
            int k = dist.x / width;

            temp_rho[Bead[j].Type][mol_type_i][k]++;
          }
        } //}}}

        // add from temporary density array to global density arrays //{{{
        for (int j = 0; j < Counts.TypesOfBeads; j++) {
          for (int k = 0; k < bins; k++) {
            rho[j][mol_type_i][k] += temp_rho[j][mol_type_i][k];
            rho_2[j][mol_type_i][k] += SQR(temp_rho[j][mol_type_i][k]);
          }
        } //}}}
      }
    } //}}}

    // free temporary density array //{{{
    for (int i = 0; i < Counts.TypesOfBeads; i++) {
      for (int j = 0; j < Counts.TypesOfMolecules; j++) {
        free(temp_rho[i][j]);
      }
      free(temp_rho[i]);
    }
    free(temp_rho); //}}}

    if (end == count_vcf)
      break;
  }
  fclose(vcf);

  // print last step? //{{{
  if (!silent) {
    if (script) {
      fprintf(stdout, "Last Step: %d\n", count_vcf);
    } else {
      fflush(stdout);
      fprintf(stdout, "\r                          ");
      fprintf(stdout, "\rLast Step: %d\n", count_vcf);
    }
  } //}}}
  //}}}

  // write densities to output file(s) //{{{
  for (int i = 0; i < Counts.TypesOfMolecules; i++) {
    if (MoleculeType[i].Use) {
      FILE *out;

      char str2[1050];
      sprintf(str2, "%s%s.rho", output_rho, MoleculeType[i].Name);
      if ((out = fopen(str2, "a")) == NULL) {
        ErrorFileOpen(str2, 'a');
        exit(1);
      }

      // calculate rdf
      for (int j = 1; j < (bins-avg); j++) {

        // calculate volume of every shell that will be averaged
        double shell[avg];
        for (int k = 0; k < avg; k++) {
          shell[k] = 4 * PI * CUBE(width) *(CUBE(j+k+1) - CUBE(j+k)) / 3;
        }

        fprintf(out, "%.2f", width*(j+0.5*avg));

        for (int k = 0; k < Counts.TypesOfBeads; k++) {
          double temp_rdp = 0, temp_number = 0,
                 temp_rdp_err = 0, temp_number_err = 0;

          // sum rdfs from all shells to be averaged
          for (int l = 0; l < avg; l++) {
            temp_rdp += rho[k][i][j+l] / (shell[l] * MoleculeType[i].Number * count);
            temp_rdp_err += rho_2[k][i][j+l] / (shell[l] * MoleculeType[i].Number * count);
            temp_number += rho[k][i][j+l] / MoleculeType[i].Number;
            temp_number_err += rho_2[k][i][j+l] / MoleculeType[i].Number;
          }

          temp_rdp_err = sqrt(temp_rdp_err - temp_rdp);
          temp_number_err = sqrt(temp_number_err - temp_number);

          // print average value to output file
          fprintf(out, " %10f %10f", temp_rdp/avg, temp_rdp_err/avg);
          fprintf(out, " %10f %10f", temp_number/avg, temp_number_err/avg);
        }
        putc('\n',out);
      }

      fclose(out);
    }
  } //}}}

  // free memory - to make valgrind happy //{{{
  free(BeadType);
  free(Index);
  FreeMoleculeType(Counts, &MoleculeType);
  FreeMolecule(Counts, &Molecule);
  FreeBead(Counts, &Bead);
  free(stuff);
  for (int i = 0; i < Counts.TypesOfBeads; i++) {
    for (int j = 0; j < Counts.TypesOfMolecules; j++) {
      free(rho[i][j]);
      free(rho_2[i][j]);
    }
    free(rho[i]);
    free(rho_2[i]);
  }
  free(rho_2);
  free(rho); //}}}

  return 0;
}
