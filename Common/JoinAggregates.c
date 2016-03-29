#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "../AnalysisTools.h"

void ErrorHelp(char cmd[50]) { //{{{
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "   %s <input.vcf> <input.agg> <output.vcf> <options>\n\n", cmd);

  fprintf(stderr, "   <input.vcf>         input filename (vcf format)\n");
  fprintf(stderr, "   <input.agg>         input filename with information about aggregates (agg format)\n");
  fprintf(stderr, "   <output.vcf>        output filename with joined coordinates (vcf format)\n");
  fprintf(stderr, "   <options>\n");
  fprintf(stderr, "      -i <name>        use input .vsf file different from dl_meso.vsf\n");
  fprintf(stderr, "      -b <name>        file containing bond alternatives to FIELD\n");
  fprintf(stderr, "      -v               verbose output\n");
  fprintf(stderr, "      -V               verbose output with more information\n");
  fprintf(stderr, "      -h               print this help and exit\n");
} //}}}

int main(int argc, char *argv[]) {

  // -h option - print help and exit //{{{
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0) {
      printf("JoinAggregates removes periodic boundary conditions from aggregates. It is  \n");
      printf("meant as a replacement of '-j' option in Aggregates utility when this       \n");
      printf("option is omitted, but later the joined coordinates are required. Distance  \n");
      printf("and beadtypes for aggregate check are read from input.agg file.             \n\n");

      printf("The utility uses dl_meso.vsf (or other input structure file) and FIELD      \n");
      printf("(along with optional bond file) files to determine all information about    \n");
      printf("the system.                                                                 \n\n");

      printf("Usage:\n");
      printf("   %s <input.vcf> <input.agg> <output.vcf> <options>\n\n", argv[0]);

      printf("   <input.vcf>         input filename (vcf format)\n");
      printf("   <input.agg>         input filename with information about aggregates (agg format)\n");
      printf("   <output.vcf>        output filename with joined coordinates (vcf format)\n");
      printf("   <options>\n");
      printf("      -i <name>        use input .vsf file different from dl_meso.vsf\n");
      printf("      -b <name>        file containing bond alternatives to FIELD\n");
      printf("      -v               verbose output\n");
      printf("      -V               verbose output with more information\n");
      printf("      -h               print this help and exit\n");
      exit(0);
    }
  } //}}}

  // print command to stdout //{{{
  for (int i = 0; i < argc; i++)
    printf(" %s", argv[i]);
  printf("\n\n"); //}}}

  // check if correct number of arguments //{{{
  if (argc < 3) {
    fprintf(stderr, "Too little arguments!\n\n");
    ErrorHelp(argv[0]);
    exit(1);
  } //}}}

  // -i <name> option - filename of input structure file //{{{
  char vsf_file[32];
  vsf_file[0] = '\0'; // check if -i option is used
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0) {

      // wrong argument to -i option
      if ((i+1) >= argc || argv[i+1][0] == '-') {
        fprintf(stderr, "\nMissing argument to '-i' option ");
        fprintf(stderr, "(or filename beginning with a dash)!\n");
        exit(1);
      }

      // check if .vsf ending is present
      char *vsf = strrchr(argv[i+1], '.');
      if (!vsf || strcmp(vsf, ".vsf")) {
        fprintf(stderr, "'-i' arguments does not have .vsf ending!\n");
        ErrorHelp(argv[0]);
        exit(1);
      }

      strcpy(vsf_file, argv[i+1]);
    }
  }

  // -i option is not used
  if (vsf_file[0] == '\0') {
    strcpy(vsf_file, "dl_meso.vsf");
  } //}}}

  // -b <name> option - filename of input bond file //{{{
  char bonds_file[32];
  bonds_file[0] = '\0'; // check if -b option is used
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-b") == 0) {

      // wrong argument to -i option
      if ((i+1) >= argc || argv[i+1][0] == '-') {
        fprintf(stderr, "\nMissing argument to '-b' option ");
        fprintf(stderr, "(or filename beginning with a dash)!\n\n");
        ErrorHelp(argv[0]);
        exit(1);
      }

      strcpy(bonds_file, argv[i+1]);
    }
  } //}}}

  // -v option - verbose output //{{{
  bool verbose = false;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-v") == 0) {
      verbose = true;

      break;
    }
  } //}}}

  // -V option - verbose output with comments from input .vcf file //{{{
  bool verbose2 = false;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-V") == 0) {
      verbose = true;
      verbose2 = true;

      break;
    }
  } //}}}

  int count = 0; // count mandatory arguments

  // <input.vcf> - filename of input vcf file (must end with .vcf) //{{{
  char input_vcf[32];
  strcpy(input_vcf, argv[++count]);

  // test if <input.vcf> filename ends with '.vsf' (required by VMD)
  char *dot = strrchr(input_vcf, '.');
  if (!dot || strcmp(dot, ".vcf")) {
    fprintf(stderr, "<input.vcf> '%s' does not have .vcf ending!\n", input_vcf);
    ErrorHelp(argv[0]);
    exit(1);
  } //}}}

  // <input.agg> - filename of input file with aggregate information //{{{
  char input_agg[32];
  strcpy(input_agg, argv[++count]); //}}}

  // <output.vcf> - filename of output agg file (must end with .agg) //{{{
  char output_vcf[32];
  strcpy(output_vcf, argv[++count]);

  // test if <output.agg> filename ends with '.vcf' (required by VMD)
  dot = strrchr(output_vcf, '.');
  if (!dot || strcmp(dot, ".vcf")) {
    fprintf(stderr, "<output.vcf> '%s' does not have .agg ending!\n", output_vcf);
    ErrorHelp(argv[0]);
    exit(1);
  } //}}}

  // variables - structures //{{{
  BeadType *BeadType; // structure with info about all bead types
  MoleculeType *MoleculeType; // structure with info about all molecule types
  Bead *Bead; // structure with info about every bead
  Molecule *Molecule; // structure with info about every molecule
  Counts Counts; // structure with number of beads, molecules, etc. //}}}

  // read system information
  bool indexed = ReadStructure(vsf_file, input_vcf, bonds_file, &Counts, &BeadType, &Bead, &MoleculeType, &Molecule);

  // open input aggregate file and read info from first line (Aggregates command) //{{{
  FILE *agg;
  if ((agg = fopen(input_agg, "r")) == NULL) {
    fprintf(stderr, "Cannot open file %s!\n", input_agg);
    exit(1);
  }

  // read minimum distance for closeness check (<distance> argument in Aggregates utility)
  double distance;
  fscanf(agg, "%*s %*s %lf", &distance);

  // skip <contacts> and <output.agg> in Aggregates command
  fscanf(agg, "%*s %*s");

  // read <type names> from Aggregates command //{{{
  int test;
  while ((test = getc(agg)) != '-') {
    ungetc(test, agg);

    char name[10];
    fscanf(agg, "%s ", name);
    int type = FindBeadType(name, Counts, BeadType);

    // Error - specified bead type name not in vcf input file
    if (type == -1) {
      fprintf(stderr, "Bead type '%s' is not in %s coordinate file!\n", name, input_vcf);
      exit(1);
    }

    BeadType[type].Use = true;
  } //}}}

  while (getc(agg) != '\n')
    ;
  while (getc(agg) != '\n')
    ; //}}}

  // open input coordinate file //{{{
  FILE *vcf;
  if ((vcf = fopen(input_vcf, "r")) == NULL) {
    fprintf(stderr, "Cannot open file %s!\n", input_vcf);
    exit(1);
  } //}}}

  // get pbc from coordinate file //{{{
  char str[32];
  // skip till 'pbc' keyword
  do {
    if (fscanf(vcf, "%s", str) != 1) {
      fprintf(stderr, "Cannot read string from '%s' file!\n", input_vcf);
    }
  } while (strcmp(str, "pbc") != 0);

  // read pbc
  Vector BoxLength;
  if (fscanf(vcf, "%lf %lf %lf", &BoxLength.x, &BoxLength.y, &BoxLength.z) != 3) {
    fprintf(stderr, "Cannot read pbc from %s!\n", input_vcf);
    exit(1);
  }

  // skip remainder of pbc line
  while (getc(vcf) != '\n')
    ;
  // skip blank line
  while (getc(vcf) != '\n')
    ;

  // print pbc if verbose output
  if (verbose) {
    printf("   box size: %lf x %lf x %lf\n\n", BoxLength.x, BoxLength.y, BoxLength.z);
  } //}}}

  // write bead type names and pbc to <output.vcf> //{{{
  for (int i = 0; i < Counts.TypesOfBeads; i++) {
    BeadType[i].Write = true;
  }

  // open <joined.vcf>
  FILE *out;
  if ((out = fopen(output_vcf, "w")) == NULL) {
    fprintf(stderr, "Cannot open output %s vcf file!\n", output_vcf);
    exit(1);
  }

  // write bead type names
  for (int i = 0; i < Counts.TypesOfBeads; i++) {
    // only those bead types that are to be used
    if (BeadType[i].Write) {
      fprintf(out, "# %s\n", BeadType[i].Name);
    }
  }

  fprintf(out, "\npbc %lf %lf %lf\n", BoxLength.x, BoxLength.y, BoxLength.z);

  fclose(out); //}}}

  // create array for the first line of a timestep ('# <number and/or other comment>') //{{{
  char *stuff;
  stuff = malloc(128*sizeof(int));

  // initialize the array
  for (int i = 0; i < 128; i++) {
    stuff[i] = '\0';
  } //}}}

  // allocate Aggregate struct //{{{
  Aggregate *Aggregate = calloc(Counts.Molecules,sizeof(*Aggregate));
  for (int i = 0; i < Counts.Molecules; i++) {
    // assumes all monomeric beads can be near one aggregate - memory-heavy, but reliable
    Aggregate[i].Monomer = calloc(Counts.Unbonded,sizeof(int));
    // assumes all bonded beads can be in one aggregate - memory-heavy, but reliable
    Aggregate[i].Bead = calloc(Counts.Bonded,sizeof(int));
    // maximum of all molecules can be in one aggregate
    Aggregate[i].Molecule = calloc(Counts.Molecules,sizeof(int));
  } //}}}

  // print information - verbose output //{{{
  if (verbose) {
    VerboseOutput(verbose2, input_vcf, bonds_file, Counts, BeadType, Bead, MoleculeType, Molecule);
  } //}}}

  // main loop //{{{
  count = 0; // count timesteps
  while ((test = getc(vcf)) != EOF) {
    ungetc(test, vcf);

    fflush(stdout);
    printf("\rStep: %6d", ++count);

    // read indexed timestep from input .vcf file //{{{
    if (indexed) {
      if ((test = ReadCoorIndexed(vcf, Counts, &Bead, &stuff)) != 0) {
        fprintf(stderr, "Cannot read coordinates from %s! (%d. step; %d. bead)\n", input_vcf, count, test);
        exit(1);
      } //}}}
    // or read ordered timestep from input .vcf file //{{{
    } else {
      if ((test = ReadCoorOrdered(vcf, Counts, &Bead, &stuff)) != 0) {
        fprintf(stderr, "Cannot read coordinates from %s! (%d. step; %d. bead)\n", input_vcf, count, test);
        exit(1);
      }
    } //}}}

    ReadAggregates(agg, &Counts, &Aggregate, MoleculeType, Molecule);

    RemovePBCMolecules(Counts, BoxLength, BeadType, &Bead, MoleculeType, Molecule);

    RemovePBCAggregates(distance, Aggregate, Counts, BoxLength, BeadType, &Bead, MoleculeType, Molecule);

    // open output .vcf file for appending //{{{
    if ((out = fopen(output_vcf, "a")) == NULL) {
      fprintf(stderr, "Cannot open file %s!\n", output_vcf);
      exit(1);
    } //}}}

    WriteCoorIndexed(out, Counts, BeadType, Bead, stuff);

    fclose(out);

    // print comment at the beginning of a timestep - detailed verbose output //{{{
    if (verbose2) {
      printf("\n%s", stuff);
    } //}}}
  }

  fclose(vcf);
  fclose(agg);

  fflush(stdout);
  printf("\rLast Step: %6d\n", count); //}}}

  // free memory - to make valgrind happy //{{{
  free(BeadType);
  for (int i = 0; i < Counts.TypesOfMolecules; i++) {
    for (int j = 0; j < MoleculeType[i].nBonds; j++) {
      free(MoleculeType[i].Bond[j]);
    }
    free(MoleculeType[i].Bond);
  }
  free(MoleculeType);
  free(Bead);
  for (int i = 0; i < Counts.Molecules; i++) {
    free(Molecule[i].Bead);

    free(Aggregate[i].Molecule);
    free(Aggregate[i].Bead);
    free(Aggregate[i].Monomer);
  }
  free(Molecule);
  free(Aggregate);
  free(stuff);
  //}}}

  return 0;
}