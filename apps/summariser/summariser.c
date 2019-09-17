/**
 * ATCA Training Application: summariser.c
 * (C) Jamie Stevens CSIRO 2019
 *
 * This app summarises one or more RPFITS files.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rpfits/reader.h"

int main(int argc, char *argv[]) {
  // The argument list should all be RPFITS files.
  int i = 0, res = 0, keep_reading = 1;
  struct cycle_data cycle_data;

  for (i = 1; i < argc; i++) {
    // Try to open the RPFITS file.
    res = open_rpfits_file(argv[i]);
    printf("Attempt to open RPFITS file %s, %d\n", argv[i], res);

    // Read in the first cycle.
    while (keep_reading) {
      keep_reading = read_cycle(&cycle_data);
      printf("cycle has obs date %s\n", cycle_data.obsdate);
      printf("  type %s, source %s\n", cycle_data.obstype, cycle_data.source_name);
    }

    // Close it before moving on.
    res = close_rpfits_file();
    printf("Attempt to close RPFITS file, %d\n", res);
  }

  exit(0);
  
}
