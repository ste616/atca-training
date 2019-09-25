/**
 * ATCA Training Application: summariser.c
 * (C) Jamie Stevens CSIRO 2019
 *
 * This app summarises plots out visibilities from an RPFITS file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "reader.h"
#include "memory.h"
#include "cpgplot.h"

#define BUFSIZE 1024

int main(int argc, char *argv[]) {
  // The argument list should all be RPFITS files.
  int i = 0, j = 0, k = 0, l = 0, m = 0, res = 0, keep_reading = 1;
  int read_cycle = 1, nscans = 0, vis_length = 0, if_no, read_response = 0;
  float min_vis, max_vis, *xpts = NULL;
  struct scan_data *scan_data = NULL, **all_scans = NULL;
  struct cycle_data *cycle_data = NULL;
  char ptitle[BUFSIZE];
  
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
      if (read_response & READER_DATA_AVAILABLE) {
	  // Now start reading the cycle data.
	read_cycle = 1;
	while (read_cycle) {
	  cycle_data = scan_add_cycle(scan_data);
	  read_response = read_cycle_data(&(scan_data->header_data),
					  cycle_data);
	  //fprintf(stderr, "found read response %d\n", read_response);
	  //printf("cycle has %d points\n", cycle_data->num_points);
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

    // Plot the data from this file.
    printf("Plotting...\n");
    cpgopen("/xs");
    cpgask(1);
    for (j = 0; j < nscans; j++) {
      scan_data = all_scans[j];
      for (k = 0; k < scan_data->num_cycles; k++) {
	cycle_data = scan_data->cycles[k];
	for (l = 0; l < cycle_data->num_points; l++) {
	  cpgpage();
	  // Determine the limits.
	  if_no = cycle_data->if_no[l] - 1;
	  vis_length = scan_data->header_data.if_num_stokes[if_no] *
	    scan_data->header_data.if_num_channels[if_no] * 2;
	  REALLOC(xpts, vis_length);
	  min_vis = INFINITY;
	  max_vis = -INFINITY;
	  for (m = 0; m < vis_length; m++) {
	    xpts[m] = m;
	    if (cycle_data->vis[l][m] != cycle_data->vis[l][m]) {
	      // This is NaN.
	      continue;
	    }
	    min_vis = (cycle_data->vis[l][m] < min_vis) ?
	      cycle_data->vis[l][m] : min_vis;
	    max_vis = (cycle_data->vis[l][m] > max_vis) ?
	      cycle_data->vis[l][m] : max_vis;
	  }
	  snprintf(ptitle, BUFSIZE, "%d-%d bin %d IF %d",
		   cycle_data->ant1[l], cycle_data->ant2[l],
		   cycle_data->bin[l], cycle_data->if_no[l]);
	  printf("%s min %.5f max %.5f\n", ptitle, min_vis, max_vis);
	  cpgswin(0, vis_length, min_vis, max_vis);
	  cpgbox("BCNTS", 0, 0, "BCNTS", 0, 0);
	  cpgline(vis_length, xpts, cycle_data->vis[l]);
	  cpglab("X", "Y", ptitle);
	}
      }
    }
    cpgclos();
  }

  // Free all the scans.
  printf("Read in %d scans from all files.\n", nscans);
  for (i = 0; i < nscans; i++) {
    free_scan_data(all_scans[i]);
  }
  
  exit(0);
  
}
