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
#include <complex.h>
#include <stdbool.h>
#include "atrpfits.h"
#include "memory.h"
#include "cpgplot.h"

#define BUFSIZE 1024

int main(int argc, char *argv[]) {
  // The argument list should all be RPFITS files.
  int i = 0, j = 0, k = 0, l = 0, m = 0, res = 0, keep_reading = 1;
  int read_cycle = 1, nscans = 0, vis_length = 0, if_no, read_response = 0;
  int plotpol = POL_XX, plotif = 0, r = 0, ant1, ant2;
  float min_vis, max_vis, *xpts = NULL;
  struct scan_data *scan_data = NULL, **all_scans = NULL;
  struct cycle_data *cycle_data = NULL;
  struct ampphase *cycle_ampphase = NULL;
  char ptitle[BUFSIZE];
  struct ampphase_options ampphase_options;
  
  cpgopen("/xs");
  cpgask(1);

  // Get phase in degrees.
  ampphase_options.phase_in_degrees = true;
  
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
      for (k = 0; k < scan_data->num_cycles; k++) {
	cycle_data = scan_data->cycles[k];
	// We will plot only XX pol of IF 1.
	r = vis_ampphase(&(scan_data->header_data), cycle_data,
			 &cycle_ampphase, plotpol, plotif, 1,
			 &ampphase_options);
	if (r < 0) {
	  printf("error encountered while calculating amp and phase\n");
	  free_ampphase(&cycle_ampphase);
	  continue;
	}
	for (l = 0; l < cycle_ampphase->nbaselines; l++) {
	  cpgpage();
	  base_to_ants(cycle_ampphase->baseline[l], &ant1, &ant2);
	  snprintf(ptitle, BUFSIZE, "%d-%d bin %d IF %d",
		   ant1, ant2, 0, plotif);
	  cpgswin(0, cycle_ampphase->nchannels,
		  cycle_ampphase->min_amplitude[l],
		  cycle_ampphase->max_amplitude[l]);
	  cpgbox("BCNTS", 0, 0, "BCNTS", 0, 0);
	  cpgline(cycle_ampphase->f_nchannels[l],
		  cycle_ampphase->f_channel[l],
		  cycle_ampphase->f_amplitude[l]);
	  cpglab("Channel", "Amplitude", ptitle);
	  cpgpage();
	  cpgswin(0, cycle_ampphase->nchannels,
		  -200, 200);
	  cpgbox("BCNTS", 0, 0, "BCNTS", 0, 0);
	  cpgline(cycle_ampphase->f_nchannels[l],
		  cycle_ampphase->f_channel[l],
		  cycle_ampphase->f_phase[l]);
	  cpglab("Channel", "Phase", ptitle);
	}
	free_ampphase(&cycle_ampphase);
      }
    
      if (read_response == READER_EXHAUSTED) {
	// No more data in this file.
	keep_reading = 0;
      } // Otherwise we've probably hit another header.
      printf("scan had %d cycles\n", scan_data->num_cycles);

    }

    // Close it before moving on.
    res = close_rpfits_file();
    printf("Attempt to close RPFITS file, %d\n", res);

  }

  cpgclos();
  
  // Free all the scans.
  printf("Read in %d scans from all files.\n", nscans);
  for (i = 0; i < nscans; i++) {
    free_scan_data(all_scans[i]);
  }
  
  exit(0);
  
}
