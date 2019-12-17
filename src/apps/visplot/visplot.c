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

// This structure holds pre-calculated panel positions for a PGPLOT
// device with nx x ny panels.
struct panelspec {
  int nx;
  int ny;
  float **x1;
  float **x2;
  float **y1;
  float **y2;
  float orig_x1;
  float orig_x2;
  float orig_y1;
  float orig_y2;
};

void splitpanels(int nx, int ny, struct panelspec *panelspec) {
  int i, j;
  float panel_width, panel_height;

  // Allocate some memory.
  panelspec->nx = nx;
  panelspec->ny = ny;
  MALLOC(panelspec->x1, nx);
  MALLOC(panelspec->x2, nx);
  MALLOC(panelspec->y1, nx);
  MALLOC(panelspec->y2, nx);
  for (i = 0; i < nx; i++) {
    MALLOC(panelspec->x1[i], ny);
    MALLOC(panelspec->x2[i], ny);
    MALLOC(panelspec->y1[i], ny);
    MALLOC(panelspec->y2[i], ny);
  }

  // Get the original settings.
  cpgqvp(0, &(panelspec->orig_x1), &(panelspec->orig_x2),
	 &(panelspec->orig_y1), &(panelspec->orig_y2));
  panel_width = (panelspec->orig_x2 - panelspec->orig_x1) / (float)nx;
  panel_height = (panelspec->orig_y2 - panelspec->orig_y1) / (float)ny;

  for (i = 0; i < nx; i++) {
    for (j = 0; j < ny; j++) {
      panelspec->x1[i][j] = panelspec->orig_x1 + i * panel_width;
      panelspec->x2[i][j] = panelspec->orig_x1 + (i + 1) * panel_width;
      panelspec->y1[i][j] = panelspec->orig_y2 - (j + 1) * panel_height;
      panelspec->y2[i][j] = panelspec->orig_y2 - j * panel_height;
    }
  }
}

void changepanel(int x, int y, struct panelspec *panelspec) {
  if ((x >= 0) && (x < panelspec->nx) &&
      (y >= 0) && (y < panelspec->ny)) {
    cpgsvp(panelspec->x1[x][y], panelspec->x2[x][y],
	   panelspec->y1[x][y], panelspec->y2[x][y]);
  } else if ((x == -1) && (y == -1)) {
    // Set it back to original.
    cpgsvp(panelspec->orig_x1, panelspec->orig_x2,
	   panelspec->orig_y1, panelspec->orig_y2);
  }
}

int main(int argc, char *argv[]) {
  // The argument list should all be RPFITS files.
  int i = 0, j = 0, k = 0, l = 0, m = 0, res = 0, keep_reading = 1;
  int read_cycle = 1, nscans = 0, vis_length = 0, if_no, read_response = 0;
  int plotpol = POL_XX, plotif = 0, r = 0, ant1, ant2, num_ifs = 2, num_pols = 2;
  int p = 0, q = 0, sp = 0, rp, ri, rx, ry;
  float min_vis, max_vis, *xpts = NULL;
  struct scan_data *scan_data = NULL, **all_scans = NULL;
  struct cycle_data *cycle_data = NULL;
  struct ampphase ***cycle_ampphase = NULL;
  struct vis_quantities *cycle_vis_quantities = NULL;
  char ptitle[BUFSIZE];
  struct ampphase_options ampphase_options;
  struct panelspec panelspec;
  
  cpgopen("/xs");
  cpgask(1);

  // Get phase in degrees.
  ampphase_options.phase_in_degrees = true;
  ampphase_options.delay_averaging = 1;
  ampphase_options.min_tvchannel = 513;
  ampphase_options.max_tvchannel = 1537;
  ampphase_options.averaging_method = AVERAGETYPE_MEAN | AVERAGETYPE_SCALAR;

  // We will get data for all IFs and pols.
  MALLOC(cycle_ampphase, num_ifs);
  for (i = 0; i < num_ifs; i++) {
    MALLOC(cycle_ampphase[i], num_pols);
    for (j = 0; j < num_pols; j++) {
      cycle_ampphase[i][j] = NULL;
    }
  }

  // Our view panel is split into 36 subplots.
  // That is 15 baselines, two plots per baseline (one amp, one phase),
  // plus the 6 autocorrelation amplitude plots.
  splitpanels(6, 6, &panelspec);
  
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
	for (q = 0; q < num_ifs; q++) {
	  for (p = 0; p < num_pols; p++) {
	    switch(p) {
	    case 0:
	      sp = POL_XX;
	      break;
	    case 1:
	      sp = POL_YY;
	      break;
	    default:
	      sp = POL_XX;
	    }
	    r = vis_ampphase(&(scan_data->header_data), cycle_data,
			     &(cycle_ampphase[q][p]), sp, q, 1,
			     &ampphase_options);
	    if (r < 0) {
	      printf("error encountered while calculating amp and phase\n");
	      free_ampphase(&(cycle_ampphase[q][p]));
	      exit(0);
	    }
	  }
	  /* r = ampphase_average(cycle_ampphase[q][p], &cycle_vis_quantities, */
	  /* 			 &ampphase_options); */
	  changepanel(-1, -1, &panelspec);
	  cpgpage();
	  for (l = 0; l < cycle_ampphase[q][0]->nbaselines; l++) {
	    base_to_ants(cycle_ampphase[q][0]->baseline[l], &ant1, &ant2);
	    /* printf("%d-%d tvchan amp = %.4f  phase = %.4f\n", */
	    /* 	   ant1, ant2, cycle_vis_quantities->amplitude[l], */
	    /* 	   cycle_vis_quantities->phase[l]); */
	    if (ant1 == ant2) {
	      changepanel(ant1 - 1, 0, &panelspec);
	    } else {
	      rp = 0;
	      for (ri = 1; ri < ant1; ri++) {
		rp += 6 - ri;
	      }
	      rp += ant2 - ant1 - 1;
	      rp *= 2;
	      rx = rp % panelspec.nx;
	      ry = 1 + (int)floorf(((float)rp - (float)rx) / (float)panelspec.nx);
	      changepanel(rx, ry, &panelspec);
	    }
	    snprintf(ptitle, BUFSIZE, "%d-%d bin %d IF %d",
		     ant1, ant2, 0, q);
	    cpgswin(0, cycle_ampphase[q][0]->nchannels,
		    cycle_ampphase[q][0]->min_amplitude[l],
		    cycle_ampphase[q][0]->max_amplitude[l]);
	    cpgsci(1);
	    cpgbox("BCTS", 0, 0, "BCTS", 0, 0);
	    cpgline(cycle_ampphase[q][0]->f_nchannels[l],
		    cycle_ampphase[q][0]->f_channel[l],
		    cycle_ampphase[q][0]->f_amplitude[l]);
	    /* cpglab("Channel", "Amplitude", ptitle); */
	    cpgsci(2);
	    cpgline(cycle_ampphase[q][1]->f_nchannels[l],
		    cycle_ampphase[q][1]->f_channel[l],
		    cycle_ampphase[q][1]->f_amplitude[l]);
	    if (ant1 != ant2) {
	      cpgsci(1);
	      rp += 1;
	      rx = rp % panelspec.nx;
	      ry = 1 + (int)floorf(((float)rp - (float)rx) / (float)panelspec.nx);
	      changepanel(rx, ry, &panelspec);
	      cpgswin(0, cycle_ampphase[q][0]->nchannels,
		      -200, 200);
	      cpgbox("BCTS", 0, 0, "BCTS", 0, 0);
	      cpgline(cycle_ampphase[q][0]->f_nchannels[l],
		      cycle_ampphase[q][0]->f_channel[l],
		      cycle_ampphase[q][0]->f_phase[l]);
	      /* cpglab("Channel", "Phase", ptitle); */
	      cpgsci(2);
	      cpgline(cycle_ampphase[q][1]->f_nchannels[l],
		      cycle_ampphase[q][1]->f_channel[l],
		      cycle_ampphase[q][1]->f_phase[l]);
	    }
	  }
	}
	for (q = 0; q < num_ifs; q++) {
	  for (p = 0; p < num_pols; p++) {
	    free_ampphase(&(cycle_ampphase[q][p]));
	  }
	}
	/* free_vis_quantities(&cycle_vis_quantities); */
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
