#include "General.h"

// IsDouble() //{{{
/**
 * Function to test if provided string is a real number.
 */
bool IsDouble(char *a) {
  // wrong first character - can be minus, dot, or number
  if (a[0] != '-' && a[0] != '.' && (a[0] < '0' || a[0] > '9')) {
    return false;
  }
  // only one dot can be present
  bool dot = false;
  if (a[0] == '.') {
    dot = true;
  }
  // test the remaining characters - either digit, or dot (but only 1 in total)
  for (int i = 1; i < strlen(a); i++) {
    if (a[i] == '.') {
      if (dot) { // has there been a dot already?
        return false;
      } else {
        dot = true;
      }
    } else if (a[i] < '0' || a[i] > '9') {
      return false;
    }
  }
  return true;
} //}}}

// IsPosDouble() //{{{
/**
 * Function to test if provided string is a non-negative real number.
 */
bool IsPosDouble(char *a) {
  // wrong first character - can be minus, dot, or number
  if (a[0] != '.' && (a[0] < '0' || a[0] > '9')) {
    return false;
  }
  // only one dot can be present
  bool dot = false;
  if (a[0] == '.') {
    dot = true;
  }
  // test the remaining characters - either digit, or dot (but only 1 in total)
  for (int i = 1; i < strlen(a); i++) {
    if (a[i] == '.') {
      if (dot) { // has there been a dot already?
        return false;
      } else {
        dot = true;
      }
    } else if (a[i] < '0' || a[i] > '9') {
      return false;
    }
  }
  return true;
} //}}}

// IsInteger() //{{{
/**
 * Function to test if provided string is a non-negative whole number.
 */
bool IsInteger(char *a) {
  // test the remaining characters - either digit, or dot (but only 1 in total)
  for (int i = 0; i < strlen(a); i++) {
    if (a[i] < '0' || a[i] > '9') {
      return false;
    }
  }
  return true;
} //}}}

// Length() //{{{
/**
 * Function to calculate vector length.
 */
double Length(VECTOR a) {
  double length = sqrt(SQR(a.x) + SQR(a.y) + SQR(a.z));
  return length;
} //}}}

// Min3() //{{{
/**
 * Function returning the lowest number from three floats.
 */
double Min3(double x, double y, double z) {

  double min;
  if (x > y) {
    if (y > z) {
      min = z;
    } else {
      min = y;
    }
  } else if (x > z) {
    min = z;
  } else {
    min = x;
  }

  return min;
} //}}}

// Max3() //{{{
/**
 * Function returning the highest number from three floats.
 */
double Max3(double x, double y, double z) {

  double max;
  if (x < y) {
    if (y < z) {
      max = z;
    } else {
      max = y;
    }
  } else if (x < z) {
    max = z;
  } else {
    max = x;
  }

  return max;
} //}}}

// Sort3() //{{{
/**
 * Function returning sorted numbers x < y < z.
 */
VECTOR Sort3(VECTOR in) {
  VECTOR out;
  if (in.x < in.y) {
    if (in.y < in.z) {
      out.x = in.x;
      out.y = in.y;
      out.z = in.z;
    } else if (in.x < in.z) {
      out.x = in.x;
      out.y = in.z;
      out.z = in.y;
    } else {
      out.x = in.z;
      out.y = in.x;
      out.z = in.y;
    }
  } else {
    if (in.x < in.z) {
      out.x = in.y;
      out.y = in.x;
      out.z = in.z;
    } else if (in.y < in.z) {
      out.x = in.y;
      out.y = in.z;
      out.z = in.x;
    } else {
      out.x = in.z;
      out.y = in.y;
      out.z = in.x;
    }
  }
  return out;
} //}}}

// SwapInt() //{{{
/**
 * Swap two integers.
 */
void SwapInt(int *a, int *b) {
  int swap = *a;
  *a = *b;
  *b = swap;
}
// }}}

// SwapDouble() //{{{
/**
 * Swap two doubles.
 */
void SwapDouble(double *a, double *b) {
  double swap = *a;
  *a = *b;
  *b = swap;
}
// }}}

// SwapBool() //{{{
/**
 * Swap two booleans.
 */
void SwapBool(bool *a, bool *b) {
  bool swap = *a;
  *a = *b;
  *b = swap;
}
// }}}

// SortArray() //{{{
/**
 * Sort an array using the bubble sort algorithm. If mode = 0, sort
 * ascendingly; if mode = 1, sort descendingly.
 */
void SortArray(int **array, int length, int mode) {
  if (mode != 0 && mode != 1) {
    fprintf(stderr, "\033[1;31m");
    fprintf(stderr, "\nError - SortArray(): use 0 or 1 for sorting mode\n");
    fprintf(stderr, "\033[0m");
    exit(1);
  }
  for (int i = 0 ; i < (length-1); i++) {
    bool done = true;
    for (int j = 0 ; j < (length-i-1); j++) {
      if (mode == 0 && (*array)[j] > (*array)[j+1]) {
        SwapInt(&(*array)[j], &(*array)[j+1]);
        done = false;
      }
      if (mode == 1 && (*array)[j] < (*array)[j+1]) {
        SwapInt(&(*array)[j], &(*array)[j+1]);
        done = false;
      }
    }
    if (done)
      break;
  }
} //}}}

// SplitLine() //{{{
/**
 * Function that splits the provided line into individual strings (using tab,
 * space, and colon as a delimiter) and removes newline character from the end
 * of the last string.
 */
int SplitLine(char out[30][100], char *line, char delim[8]) {
  // trim whitespaces at the beginning and end of line
  strcpy(line, TrimLine(line));
  // split into words separated by delimiters in delim array
  char *split[30];
  split[0] = strtok(line, delim); // first word
  int words = 0;
  while (split[words] != NULL && words < 29) {
    words++; // start from 1, as the first split is already done
    split[words] = strtok(NULL, " \t:");
  }
  // if the last word ends with newline, make it into '\0'
  if (split[words-1][strlen(split[words-1])-1] == '\n') {
    split[words-1][strlen(split[words-1])-1] = '\0';
  }
  // if the last words is just '\0', disregard it
  if (split[words-1][0] == '\0') {
    words--;
  }
  // copy splits into the output array
  for (int i = 0; i < words; i++) {
    strcpy(out[i], split[i]);
  }
  if (words == 0) {
    out[0][0] = '\0';
  }
  return words;
} //}}}

// TrimLine() //{{{
/**
 * Function to trim whitespace from the
 * beginning and end of a string.
 */
char * TrimLine(char *line) {
  int length = strlen(line);
  static char trimmed[LINE];
  strcpy(trimmed, line);
  // 1) trailing whitespace
  while (length > 1 &&
         (trimmed[length-1] == ' ' ||
          trimmed[length-1] == '\n' ||
          trimmed[length-1] == '\t')) {
    trimmed[length-1] = '\0';
    length--;
  }
  // 2) preceding whitespace
  while (length > 1 &&
         (trimmed[0] == ' ' ||
          trimmed[0] == '\n' ||
          trimmed[0] == '\t')) {
    for (int i = 0; i < length; i++) { // line[length] contains '\0'
      trimmed[i] = trimmed[i+1];
    }
    length--;
  }
  // 3) if only 1 character remains, make it into newline (otherwise segfault happens)
  if (length == 1 && (trimmed[0] == ' ' || trimmed[0] == '\t')) {
    trimmed[0] = '\n';
  }

  return trimmed;
} //}}}

// PrintCommand() //{{{
/**
 * Function to print full command.
 */
void PrintCommand(FILE *ptr, int argc, char *argv[]) {
  // first argument can contain whole path - remove that
  char *split[30], str[LINE];
  strcpy(str, argv[0]);
  split[0] = strtok(str, "/"); // first word
  int words = 0;
  while (split[words] != NULL && words < 29) {
    words++; // start from 1, as the first split is already done
    split[words] = strtok(NULL, "/");
  }
  // print last split of argv[0], i.e., pathless command name
  fprintf(ptr, " %s", split[words-1]);
  // print the rest of the command
  for (int i = 1; i < argc; i++)
    fprintf(ptr, " %s", argv[i]);
  fprintf(ptr, "\n");
}
