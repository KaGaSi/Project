#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "../AnalysisTools.h"
#include "../Options.h"
#include "../Errors.h"

void ErrorHelp(char cmd[50]) { //{{{
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "   %s <out.vsf> <out.vcf> <options>\n\n", cmd);
  fprintf(stderr, "   <out.vsf>     output structure file (*.vsf)\n");
  fprintf(stderr, "   <out.vcf>     output coordinate file (*.vcf)\n");
  fprintf(stderr, "   <options>\n");
  fprintf(stderr, "      -f <name>  FIELD-like file (default: FIELD)\n");
  fprintf(stderr, "      -v         verbose output\n");
  fprintf(stderr, "      -h         print this help and exit\n");
} //}}}

// WriteVsf() //{{{
/*
 * Function creating `.vsf` structure file for use in conjunction with
 * `.vcf` coordinate file for better visualisation via VMD program.
 */
void WriteVsf(char *input_vsf, Counts Counts, BeadType *BeadType, Bead *Bead,
              MoleculeType *MoleculeType, Molecule *Molecule) {

  // open structure file //{{{
  FILE *fw;
  if ((fw = fopen(input_vsf, "w")) == NULL) {
    ErrorFileOpen(input_vsf, 'w');
    exit(1);
  } //}}}

  // find most common type of bead not in molecules and make it default //{{{
  int type_def = 0;
  int count[Counts.TypesOfBeads];
  for (int i = 0; i < Counts.TypesOfBeads; i++) {
    count[i] = 0;
  }

  // count unbonded beads of each type
  for (int i = 0; i < Counts.Beads; i++) {
    if (Bead[i].Molecule == -1) {
      count[Bead[i].Type]++;
    }
  }

  // find the highest number
  for (int i = 1; i < Counts.TypesOfBeads; i++) {
    if (count[i] > count[0]) {
      count[0] = count[i];
      type_def = i;
    }
  } //}}}

  fprintf(fw, "atom default name %s mass %4.2f charge %5.2f\n", BeadType[type_def].Name, BeadType[type_def].Mass, BeadType[type_def].Charge);
  for (int i = 0; i < Counts.BeadsInVsf; i++) {
    if (Bead[i].Type != type_def || Bead[i].Molecule != -1) {
      fprintf(fw, "atom %7d ", i);
      fprintf(fw, "name %8s ", BeadType[Bead[i].Type].Name);
      fprintf(fw, "mass %4.2f ", BeadType[Bead[i].Type].Mass);
      fprintf(fw, "charge %5.2f", BeadType[Bead[i].Type].Charge);
      if (Bead[i].Molecule != -1) {
        int type = Molecule[Bead[i].Molecule].Type;
        fprintf(fw, " resname %10s ", MoleculeType[type].Name);
        fprintf(fw, "resid %5d\n", Bead[i].Molecule + 1);
      } else {
        putc('\n', fw);
      }
    }
  }

  // print bonds //{{{
  putc('\n', fw);
  int counter = 0;
  // go through all molecule types
  for (int i = 0; i < Counts.TypesOfMolecules; i++) {
    // go through all molecules of type 'i'
    for (int j = 0; j < MoleculeType[i].Number; j++) {
      // go through all bonds of 'j'-th molecule of type 'i'
      fprintf(fw, "# resid %d\n", counter+1); // in VMD resid start with 1
      for (int k = 0; k < MoleculeType[i].nBonds; k++) {
        fprintf(fw, "bond %6d: %6d\n", Molecule[counter].Bead[MoleculeType[i].Bond[k][0]],
                                       Molecule[counter].Bead[MoleculeType[i].Bond[k][1]]);
      }

      counter++;
    }
  } //}}}

  // close structure file
  fclose(fw);
} //}}}

int main(int argc, char *argv[]) {

  // -h option - print help and exit //{{{
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0) {
      fprintf(stdout, "\
GenVsf reads information from FIELD-like file and creates \
vsf structure file and generates coordinates for all beads \
(used, e.g., as initial configuration for a simulation).\n\n");

/*      fprintf(stdout, "\
The utility uses traject.vsf (or other input structure file) and FIELD (along \
with optional bond file) files to determine all information about the \
system.\n\n");
*/

      fprintf(stdout, "Usage:\n");
      fprintf(stdout, "   %s <out.vsf> <out.vcf> <options>\n\n", argv[0]);
      fprintf(stdout, "   <out.vsf>     output structure file (*.vsf)\n");
      fprintf(stdout, "   <out.vcf>     output coordinate file (*.vcf)\n");
      fprintf(stdout, "   <options>\n");
      fprintf(stdout, "      -f <name>  FIELD-like file (default: FIELD)\n");
      fprintf(stdout, "      -v         verbose output\n");
      fprintf(stdout, "      -h         print this help and exit\n");
      exit(0);
    }
  }

  int req_args = 2; //}}}

  // check if correct number of arguments //{{{
  int count = 0;
  for (int i = 1; i < argc && argv[count][0] != '-'; i++) {
    count++;
  }

  if (count < req_args) {
    ErrorArgNumber(count, req_args);
    ErrorHelp(argv[0]);
    exit(1);
  } //}}}

  // test if options are given correctly //{{{
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-' &&
        strcmp(argv[i], "-f") != 0 &&
        strcmp(argv[i], "-v") != 0) {

      ErrorOption(argv[i]);
      ErrorHelp(argv[0]);
      exit(1);
    }
  } //}}}

  // print command to stdout //{{{
  for (int i = 0; i < argc; i++)
    fprintf(stdout, " %s", argv[i]);
  putchar('\n'); //}}}

  // options before reading system data //{{{
  // output verbosity //{{{
  bool verbose = BoolOption(argc, argv, "-v"); // verbose output
  // }}}

  // FIELD-like file //{{{
  char *input = calloc(32, sizeof(char *));
  if (FileOption(argc, argv, "-f", &input)) {
    exit(1);
  }
  if (input[0] == '\0') {
    strcpy(input, "FIELD");
  } //}}}
  //}}}

  count = 0; // count arguments

  // <out.vsf> - output structure file (must end with .vsf) //{{{
  char output[32];
  strcpy(output, argv[++count]);

  // test if <output.vsf> filename ends with '.vsf' or '.vtf' (required by VMD)
  int ext = 1;
  char **extension = malloc(ext*sizeof(char *));
  for (int i = 0; i < ext; i++) {
    extension[i] = malloc(5*sizeof(char));
  }
  strcpy(extension[0], ".vsf");
  if (!ErrorExtension(output, ext, extension)) {
    ErrorHelp(argv[0]);
    exit(1);
  }
  for (int i = 0; i < ext; i++) {
    free(extension[i]);
  }
  free(extension); //}}}

  // <out.vcf> - output vcf file //{{{
  char *output_vcf = calloc(32, sizeof(char));
  char stuff[1024];
  strcpy(output_vcf, argv[++count]);

  // test if outpuf_vcf has '.vcf' extension - required by vmd //{{{
  ext = 1;
  extension = malloc(ext*sizeof(char *));
  extension[0] = malloc(5*sizeof(char));
  strcpy(extension[0], ".vcf");
  if (!ErrorExtension(output_vcf, ext, extension)) {
    ErrorHelp(argv[0]);
    exit(1);
  }
  for (int i = 0; i < ext; i++) {
    free(extension[i]);
  }
  free(extension); //}}}
  //}}}

  // variables - structures //{{{
  BeadType *BeadType; // structure with info about all bead types
  MoleculeType *MoleculeType; // structure with info about all molecule types
  Bead *Bead; // structure with info about every bead
  Molecule *Molecule; // structure with info about every molecule
  Counts Counts; // structure with number of beads, molecules, etc. //}}}

  // read system information from FIELD-like //{{{
  // open FIELD-like file //{{{
  FILE *fr;
  if ((fr = fopen(input, "r")) == NULL) {
    ErrorFileOpen(input, 'r');
    exit(1);
  } //}}}

  // read box size //{{{
  char line[1024], *box[3];
  fgets(line, sizeof(line), fr);
  box[0] = strtok(line, " \t");
  box[1] = strtok(NULL, " \t");
  box[2] = strtok(NULL, " \t");
  Vector BoxLength;
  BoxLength.x = atof(box[0]);
  BoxLength.y = atof(box[1]);
  BoxLength.z = atof(box[2]); //}}}

  // read number of bead types //{{{
  while(fgets(line, sizeof(line), fr)) {
    char *split;
    split = strtok(line, " \t ");
    if (strcmp(split, "species") == 0 ||
        strcmp(split, "Species") == 0 ||
        strcmp(split, "SPECIES") == 0 ) {
      Counts.TypesOfBeads = atoi(strtok(NULL, " \t"));
      break;
    }
  } //}}}

  BeadType = calloc(Counts.TypesOfBeads,sizeof(struct BeadType));
  Bead = malloc(1*sizeof(struct Bead));

  // read info about bead types //{{{
  Counts.Unbonded = 0;
  Counts.Beads = 0;
  for (int i = 0; i < Counts.TypesOfBeads; i++) {
    fgets(line, sizeof(line), fr);

    // split the line into array
    char *split[4];
    split[0] = strtok(line, " \t");
    for (int j = 1; j < 4; j++) {
      split[j] = strtok(NULL, " \t");
    }

    strcpy(BeadType[i].Name, split[0]);
    BeadType[i].Mass = atof(split[1]);
    BeadType[i].Charge = atof(split[2]);
    BeadType[i].Number = atoi(split[3]);
    Counts.Unbonded += BeadType[i].Number;

    // realloc & fill Bead array
    Bead = realloc(Bead, Counts.Unbonded*sizeof(struct Bead));
    for (int j = Counts.Beads; j < Counts.Unbonded; j++) {
      Bead[j].Type = i;
      Bead[j].Molecule = -1;
      Bead[j].Index = j;
    }

    Counts.Beads = Counts.Unbonded;
  } //}}}

  // read number of molecule types //{{{
  while(fgets(line, sizeof(line), fr)) {
    char *split;
    split = strtok(line, " \t ");
    if (strcmp(split, "molecule") == 0 ||
        strcmp(split, "Molecule") == 0 ||
        strcmp(split, "MOLECULE") == 0 ) {
      Counts.TypesOfMolecules = atoi(strtok(NULL, " \t"));
      break;
    }
  } //}}}

  MoleculeType = calloc(Counts.TypesOfMolecules,sizeof(struct MoleculeType));
  Molecule = calloc(1,sizeof(struct Molecule));

  // read info about molecule types (and calculate numbers of beads and stuff) //{{{
  Counts.Bonded = 0;
  Counts.Molecules = 0;
  for (int i = 0; i < Counts.TypesOfMolecules; i++) {
    // name //{{{
    fgets(line, sizeof(line), fr);
    // trim trailing whitespace in line
    int length = strlen(line);
    // last string character needs to be '\0'
    while (length > 1 &&
           (line[length-1] == ' ' ||
            line[length-1] == '\n' ||
            line[length-1] == '\t')) {
      line[length-1] = '\0';
      length--;
    }
    strcpy(MoleculeType[i].Name, strtok(line, " \t")); //}}}
    // number of molecules //{{{
    fgets(line, sizeof(line), fr);
    strtok(line, " \t ");
    MoleculeType[i].Number = atoi(strtok(NULL, " \t")); //}}}
    // number of beads //{{{
    fgets(line, sizeof(line), fr);
    strtok(line, " \t ");
    MoleculeType[i].nBeads = atoi(strtok(NULL, " \t")); //}}}
    // number of bonded beads
    Counts.Bonded += MoleculeType[i].Number * MoleculeType[i].nBeads;
    // realloc Bead and Molecule arrays //{{{
    Bead = realloc(Bead, (Counts.Unbonded+Counts.Bonded)*sizeof(struct Bead));
    Molecule = realloc(Molecule, (Counts.Molecules+MoleculeType[i].Number)*sizeof(struct Molecule));
    for (int j = 0; j < MoleculeType[i].Number; j++) {
      Molecule[Counts.Molecules+j].Bead = calloc(MoleculeType[i].nBeads, (sizeof(int)));
      Molecule[Counts.Molecules+j].Type = i;
    } //}}}
    // number & ids of beadtypes & mass //{{{
    MoleculeType[i].nBTypes = 0;
    MoleculeType[i].BType = calloc(1,sizeof(int));
    MoleculeType[i].Mass = 0;
    for (int j = 0; j < MoleculeType[i].nBeads; j++) {
      fgets(line, sizeof(line), fr);
      bool test = false;
      int type = FindBeadType(strtok(line, " \t "), Counts, BeadType);
      for (int k = 0; k < MoleculeType[i].nBTypes; k++) {
        if (MoleculeType[i].BType[k] == type) {
          test = true;
          break;
        }
      }
      if (!test) {
        MoleculeType[i].nBTypes++;
        MoleculeType[i].BType = realloc(MoleculeType[i].BType,MoleculeType[i].nBTypes*sizeof(int));
        MoleculeType[i].BType[MoleculeType[i].nBTypes-1] = type;
      }
      MoleculeType[i].Mass += BeadType[type].Mass;
      BeadType[type].Number += MoleculeType[i].Number;
      // fill Bead & Molecule arrays
      for (int k = 0; k < MoleculeType[i].Number; k++) {
        int id = Counts.Beads + MoleculeType[i].nBeads * k + j;
        Bead[id].Type = type;
        Bead[id].Molecule = k;
        Bead[id].Index = id;
        Molecule[Counts.Molecules+k].Bead[j] = id;
      }
    } //}}}
    // number of bonds //{{{
    fgets(line, sizeof(line), fr);
    strtok(line, " \t ");
    MoleculeType[i].nBonds = atoi(strtok(NULL, " \t")); //}}}
    // connectivity //{{{
    MoleculeType[i].Bond = malloc(MoleculeType[i].nBonds*sizeof(int *));
    for (int j = 0; j < MoleculeType[i].nBonds; j++) {
      MoleculeType[i].Bond[j] = calloc(2, sizeof(int));
      fgets(line, sizeof(line), fr);
      strtok(line, " \t ");
      MoleculeType[i].Bond[j][0] = atoi(strtok(NULL, " \t")) - 1;
      MoleculeType[i].Bond[j][1] = atoi(strtok(NULL, " \t")) - 1;
    } //}}}
    // total number of beads
    Counts.Beads = Counts.Unbonded + Counts.Bonded;
    // total number of molecules
    Counts.Molecules = MoleculeType[i].Number;

    // skip till 'finish' //{{{
    while(fgets(line, sizeof(line), fr)) {
      char *split;
      split = strtok(line, " \t\n");
      if (strcmp(split, "finish") == 0 ||
          strcmp(split, "Finish") == 0 ||
          strcmp(split, "FINISH") == 0 ) {
        break;
      }
    } //}}}
  } //}}}

  // allocate Bead Aggregate array - needed only to free() //{{{
  for (int i = 0; i < Counts.Beads; i++) {
    Bead[i].Aggregate = calloc(10,sizeof(double));
  } //}}}

  // calculate total number of beads //{{{
  Counts.Beads = 0;
  for (int i = 0; i < Counts.TypesOfBeads; i++) {
    Counts.Beads += BeadType[i].Number;
  }
  Counts.Bonded = Counts.Beads - Counts.Unbonded;
  Counts.BeadsInVsf = Counts.Beads; //}}}

  // calculate total number of molecules //{{{
  Counts.Molecules = 0;
  for (int i = 0; i < Counts.TypesOfMolecules; i++) {
    Counts.Molecules += MoleculeType[i].Number;
  } //}}}

  // write all molecules & beads //{{{
  for (int i = 0; i < Counts.TypesOfMolecules; i++) {
    MoleculeType[i].Write = true;
  }
  for (int i = 0; i < Counts.TypesOfBeads; i++) {
    BeadType[i].Write = true;
  } //}}}

  fclose(fr); //}}}

  // create & fill output vsf file
  WriteVsf(output, Counts, BeadType, Bead, MoleculeType, Molecule);

  // Generate coordinates //{{{
  // open FIELD-like file //{{{
  if ((fr = fopen(input, "r")) == NULL) {
    ErrorFileOpen(input, 'r');
    exit(1);
  } //}}}

  // skip to molecule section //{{{
  while(fgets(line, sizeof(line), fr)) {
    char *split;
    split = strtok(line, " \t ");
    if (strcmp(split, "molecule") == 0 ||
        strcmp(split, "Molecule") == 0 ||
        strcmp(split, "MOLECULE") == 0 ) {
      Counts.TypesOfMolecules = atoi(strtok(NULL, " \t"));
      break;
    }
  } //}}}

  // read bond lenghts for all molecule types and create prototype molecules //{{{
  double *prototype_z[Counts.TypesOfMolecules]; // prototype molecule z coordinate
  for (int i = 0; i < Counts.TypesOfMolecules; i++) {
    // allocate array for bond lengths
    prototype_z[i] = calloc(MoleculeType[i].nBeads,sizeof(double));
    prototype_z[i][0] = 0;

    // skip to bonds section //{{{
    while(fgets(line, sizeof(line), fr)) {
      char *split;
      split = strtok(line, " \t");
      if (strcmp(split, "bonds") == 0 ||
          strcmp(split, "Bonds") == 0 ||
          strcmp(split, "BONDS") == 0 ) {
        break;
      }
    } //}}}

    for (int j = 0; j < MoleculeType[i].nBonds; j++) {
      fgets(line, sizeof(line), fr);
      char *split;
      split = strtok(line, " \t"); // bond type
      split = strtok(NULL, " \t"); // first id
      split = strtok(NULL, " \t"); // second id
      split = strtok(NULL, " \t"); // spring constant
      split = strtok(NULL, " \t"); // length - what is needed
      prototype_z[i][j+1] = prototype_z[i][j] + atof(split);
    }

    printf("\nbox = (%lf, %lf, %lf)\n\nPrototype %s molecule (z coordinate):\n", BoxLength.x, BoxLength.y, BoxLength.z, MoleculeType[i].Name);
    for (int j = 0; j < MoleculeType[i].nBeads; j++) {
      printf("%2d: %lf\n", j, prototype_z[i][j]);
    }
  } //}}}

  // longest molecule type //{{{
  int longest = -1;
  for (int i = 0; i < Counts.TypesOfMolecules; i++) {
    if (MoleculeType[i].nBeads > longest) {
      longest = i;
    }
  }
  double length = prototype_z[longest][MoleculeType[longest].nBeads-1];
  printf("Length = %lf\n", length); //}}}

  double dist = 0.7; // distance between layers (and beads)
  int x_n = BoxLength.x / dist, y_n = BoxLength.y / dist; // number of positions per x and y axes
  int x = 0, y = 0; // coordinates that are incrementally increased
  double z = 0.1; // coordinates that are incrementally increased
  int count_free = 0; // count number of unbonded beads that are placed

  int layer[Counts.TypesOfMolecules];
  int total_layers = 0;
  for (int i = 0; i < Counts.TypesOfMolecules; i++) {
    layer[i] = x_n * (int)(BoxLength.z / (prototype_z[i][MoleculeType[i].nBeads-1] + dist));
    printf("%s per layer: %d ", MoleculeType[i].Name, layer[i]);
    layer[i] = MoleculeType[i].Number / layer[i] + 1;
    printf("(%d layers)\n", layer[i]);
    total_layers += layer[i];
  }
  int sol_layers = Counts.Unbonded / (x_n * y_n);
  int free_layer_per_mol_layer = sol_layers / total_layers;
  printf("%d molecule layers and %d solvent layers (%d solvent layers per one molecule layer).\n", total_layers, sol_layers, free_layer_per_mol_layer);

  // molecules layers with solvent layers in between //{{{
  for (int i = 0; i < Counts.Molecules; i++) {
    int type = Molecule[i].Type;
    for (int j = 0; j < MoleculeType[type].nBeads; j++) {
      int id = Molecule[i].Bead[j];
      Bead[id].Position.x = (x % x_n) * dist + 0.1;
      Bead[id].Position.y = (y % y_n) * dist + 0.1;
      Bead[id].Position.z = z + prototype_z[type][j];
    }

    z += prototype_z[type][MoleculeType[type].nBeads-1] + dist;
    // add solvent when molecule do not fit till the end of z direction
    if ((z+prototype_z[type][MoleculeType[type].nBeads-1]) >= BoxLength.z) {
      for (; count_free < Counts.Unbonded; count_free++) {
        Bead[count_free].Position.x = (x % x_n) * dist + 0.1;
        Bead[count_free].Position.y = (y % y_n) * dist + 0.1;
        Bead[count_free].Position.z = z;
        z += dist;
        if (z >= BoxLength.z) {
          break;
        }
      }
      z = 0.1;
    }
    // if z is filled, change also x and y
    if (z == 0.1) {
      x++;
      int y_old = y;
      y = x / x_n;
      // when y changes, whole xz layer of molecules was added -- add xz layers of unbonded beads
      if ((y-y_old) > 0) {
        for (int j = 0; j < free_layer_per_mol_layer; j++) {
          for (; count_free < Counts.Unbonded; count_free++) {
            Bead[count_free].Position.x = (x % x_n) * dist + 0.1;
            Bead[count_free].Position.y = (y % y_n) * dist + 0.1;
            Bead[count_free].Position.z = z;
            z += dist;
            if (z >= BoxLength.z) {
              z = 0.1;
            }
            if (z == 0.1) {
              x++;
              y_old = y;
              y = x / x_n;
              // when y changes, whole xz layer of unbonded beads was added
              if ((y-y_old) > 0) {
                break;
              }
            }
          }
        }
      }
    }
  } //}}}

  // remaining unbonded beads //{{{
  for (; count_free < Counts.Unbonded; count_free++) {
    Bead[count_free].Position.x = (x % x_n) * dist + 0.1;
    Bead[count_free].Position.y = (y % y_n) * dist + 0.1;
    Bead[count_free].Position.z = z;
//  printf("%d %lf %lf %lf\n", count_free, Bead[count_free].Position.x, Bead[count_free].Position.y, Bead[count_free].Position.z);
    z += dist;
    if (z >= BoxLength.z) {
      z = 0.1;
    }
    if (z >= BoxLength.z) {
      z = 0.1;
    }
    if (z == 0.1) {
      x++;
      y = x / x_n;
    }
  } //}}}

  // tweak y_dist and do it again //{{{
  double y_dist = ((BoxLength.y - 0.5) / (y * dist)) * dist;
  x = 0; // coordinates that are incrementally increased
  y = 0; // coordinates that are incrementally increased
  z = 0.1; // coordinates that are incrementally increased
  double y_coor = 0.1;
  count_free = 0; // count number of unbonded beads that are placed
  printf("y_dist = %lf\n", y_dist);

  // molecules layers with solvent layers in between //{{{
  for (int i = 0; i < Counts.Molecules; i++) {
    int type = Molecule[i].Type;
    for (int j = 0; j < MoleculeType[type].nBeads; j++) {
      int id = Molecule[i].Bead[j];
      Bead[id].Position.x = (x % x_n) * dist + 0.1;
      Bead[id].Position.y = y_coor;
      Bead[id].Position.z = z + prototype_z[type][j];
    }

    z += prototype_z[type][MoleculeType[type].nBeads-1] + dist;
    // add solvent when molecule do not fit till the end of z direction
    if ((z+prototype_z[type][MoleculeType[type].nBeads-1]) >= BoxLength.z) {
      for (; count_free < Counts.Unbonded; count_free++) {
        Bead[count_free].Position.x = (x % x_n) * dist + 0.1;
        Bead[count_free].Position.y = y_coor;
        Bead[count_free].Position.z = z;
        z += dist;
        if (z >= BoxLength.z) {
          break;
        }
      }
      z = 0.1;
    }
    // if z is filled, change also x and y
    if (z == 0.1) {
      x++;
      int y_old = y;
      y = x / x_n;
      // when y changes, whole xz layer of molecules was added -- add xz layers of unbonded beads
      if ((y-y_old) > 0) {
        y_coor += y_dist;
        for (int j = 0; j < free_layer_per_mol_layer; j++) {
          for (; count_free < Counts.Unbonded; count_free++) {
            Bead[count_free].Position.x = (x % x_n) * dist + 0.1;
            Bead[count_free].Position.y = y_coor;
            Bead[count_free].Position.z = z;
            z += dist;
            if (z >= BoxLength.z) {
              z = 0.1;
            }
            if (z == 0.1) {
              x++;
              y_old = y;
              y = x / x_n;
              // when y changes, whole xz layer of unbonded beads was added
              if ((y-y_old) > 0) {
                y_coor += y_dist;
                break;
              }
            }
          }
        }
      }
    }
  } //}}}

  // remaining unbonded beads //{{{
  for (; count_free < Counts.Unbonded; count_free++) {
    Bead[count_free].Position.x = (x % x_n) * dist + 0.1;
    Bead[count_free].Position.y = y_coor;
    Bead[count_free].Position.z = z;
//  printf("%d %lf %lf %lf\n", count_free, Bead[count_free].Position.x, Bead[count_free].Position.y, Bead[count_free].Position.z);
    z += dist;
    if (z >= BoxLength.z) {
      z = 0.1;
    }
    if (z >= BoxLength.z) {
      z = 0.1;
    }
    if (z == 0.1) {
      x++;
      int y_old = y;
      y = x / x_n;
      if ((y-y_old) > 0) {
        y_coor += y_dist;
      }
    }
  } //}}}
  //}}}
  //}}}

  // open output .vcf file for writing //{{{
  FILE *out;
  if ((out = fopen(output_vcf, "w")) == NULL) {
    ErrorFileOpen(output_vcf, 'w');
    exit(1);
  } //}}}

  // write pbc
  fprintf(out, "pbc %lf %lf %lf\n", BoxLength.x, BoxLength.y, BoxLength.z);
  // write coordinates
  WriteCoorIndexed(out, Counts, BeadType, Bead, MoleculeType, Molecule, stuff);
  fclose(out);

  // print information - verbose option //{{{
  if (verbose) {
    char null[1] = {'\0'};
    putchar('\n');
    putchar('\n');
    VerboseOutput(false, null, null, Counts, BeadType, Bead, MoleculeType, Molecule);
  } //}}}

  // free memory - to make valgrind happy //{{{
  free(BeadType);
  FreeMoleculeType(Counts, &MoleculeType);
  FreeMolecule(Counts, &Molecule);
  FreeBead(Counts, &Bead);
  for (int i = 0; i < Counts.TypesOfMolecules; i++) {
    free(prototype_z[i]);
  }
  free(input); //}}}

  return 0;
}
