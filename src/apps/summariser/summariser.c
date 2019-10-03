/**
 * ATCA Training Application: summariser.c
 * (C) Jamie Stevens CSIRO 2019
 *
 * This app summarises one or more RPFITS files.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atrpfits.h"
#include "memory.h"

int main(int argc, char *argv[]) {
  // The argument list should all be RPFITS files.
  int i = 0, j = 0, k = 0, res = 0, keep_reading = 1, read_response = 0;
  int read_cycle = 1, nscans = 0;
  struct scan_data *scan_data = NULL, **all_scans = NULL;
  struct cycle_data *cycle_data = NULL;
  
  for (i = 1; i < argc; i++) {
    // Try to open the RPFITS file.
    res = open_rpfits_file(argv[i]);
    printf("Attempt to open RPFITS file %s, %d\n", argv[i], res);

    while (keep_reading) {
      // Make a new scan and add it to the list.
      scan_data = prepare_new_scan_data();
      nscans += 1;
      ARRAY_APPEND(all_scans, nscans, scan_data);

      // Read in the scan header.
      read_response = read_scan_header(&(scan_data->header_data));
      printf("scan has obs date %s, time %.1f\n",
	     scan_data->header_data.obsdate,
	     scan_data->header_data.ut_seconds);
      printf("  type %s, source %s, calcode %s\n",
	     scan_data->header_data.obstype,
	     scan_data->header_data.source_name,
	     scan_data->header_data.calcode);
      printf("  coordinates RA = %.4f, Dec = %.4f\n",
	     scan_data->header_data.rightascension_hours,
	     scan_data->header_data.declination_degrees);
      printf("  number of IFs = %d:\n", scan_data->header_data.num_ifs);
      for (i = 0; i < scan_data->header_data.num_ifs; i++) {
	printf("   %d: num channels = %d, num stokes = %d\n", i,
	       scan_data->header_data.if_num_channels[i],
	       scan_data->header_data.if_num_stokes[i]);
	printf("      ");
	for (j = 0; j < scan_data->header_data.if_num_stokes[i]; j++) {
	  printf("[%s] ", scan_data->header_data.if_stokes_names[i][j]);
	}
	printf("\n");
      }
      if (read_response & READER_DATA_AVAILABLE) {
	  // Now start reading the cycle data.
	read_cycle = 1;
	while (read_cycle) {
	  cycle_data = scan_add_cycle(scan_data);
	  read_response = read_cycle_data(&(scan_data->header_data),
					  cycle_data);
	  //fprintf(stderr, "found read response %d\n", read_response);
	  /*printf("cycle has %d baselines\n", cycle_data->n_baselines);*/
	  if (!(read_response & READER_DATA_AVAILABLE)) {
	    read_cycle = 0;
	  }
	}
      } 
      if (read_response == READER_EXHAUSTED) {
	// No more data in this file.
	keep_reading = 0;
      } // Otherwise we've probably hit another header.
      printf("scan had %d cycles\n", scan_data->num_cycles);
      /*for (j = 0; j < scan_data->num_cycles; j++) {
	/*printf("   %04d: number of products = %d\n", (j + 1),
	  scan_data->cycles[j]->num_points);* /
	printf(" %04d: unflagged baselines ", (j + 1));
	for (k = 0; k < scan_data->cycles[j]->num_points; k++) {
	  if (scan_data->cycles[j]->flag[k] == RPFITS_FLAG_GOOD) {
	    printf(" %d-%d", scan_data->cycles[j]->ant1[k],
		   scan_data->cycles[j]->ant2[k]);
	  }
	}
	printf("\n");
	}*/

    }

    // Close it before moving on.
    res = close_rpfits_file();
    printf("Attempt to close RPFITS file, %d\n", res);
  }

  // Free all the scans.
  printf("Read in %d scans from all files.\n", nscans);
  for (i = 0; i < nscans; i++) {
    free_scan_data(all_scans[i]);
  }
  
  exit(0);
  
}
