/* csol
 * Copyright (c) 2017 Niels Sonnich Poulsen (http://nielssp.dk)
 * Licensed under the MIT license.
 * See the LICENSE file or http://opensource.org/licenses/MIT for more information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

int file_exists(const char *path) {
  FILE *f = fopen(path, "r");
  if (f) {
    fclose(f);
    return 1;
  }
  return 0;
}

char *combine_paths(const char *path1, const char *path2) {
  size_t length1 = strlen(path1);
  size_t length2 = strlen(path2);
  size_t combined_length = length1 + length2 + 2;
  char *combined_path = malloc(combined_length);
  strcpy(combined_path, path1);
  if (path1[length1 - 1] != '/') {
    strcat(combined_path, "/");
  }
  strcat(combined_path, path2);
  return combined_path;
}
