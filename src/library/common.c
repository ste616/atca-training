/**
 * ATCA Training Library: common.c
 * (C) Jamie Stevens CSIRO 2019
 *
 * This library contains functions that are useful for many of the
 * applications.
 */
#include <string.h>
#include "common.h"
#include "cpgplot.h"
#include "memory.h"

int interpret_array_string(char *array_string) {
  // The array string should be a comma-separated list of antenna.
  int a = 0, r = 0;
  char *token;
  const char s[2] = ",";
  
  token = strtok(array_string, s);
  while (token != NULL) {
    a = atoi(token);
    if ((a >= 1) && (a <= MAXANTS)) {
      r |= 1<<a;
    }
    token = strtok(NULL, s);
  }

  return r;
}

void init_plotcontrols(struct plotcontrols *plotcontrols,
		       int xaxis_type, int yaxis_type, int pols,
		       int corr_type) {
  int i;

  // Initialise the plotcontrols structure.
  // Set some defaults.
  if (xaxis_type == DEFAULT) {
    xaxis_type = PLOT_CHANNEL;
  }
  if (yaxis_type == DEFAULT) {
    yaxis_type = PLOT_AMPLITUDE | PLOT_AMPLITUDE_LINEAR;
  }
  if (pols == DEFAULT) {
    pols = PLOT_POL_XX | PLOT_POL_YY;
  }
  if (corr_type == DEFAULT) {
    corr_type = PLOT_AUTOCORRELATIONS | PLOT_CROSSCORRELATIONS;
  }
  plotcontrols->plot_options = xaxis_type | yaxis_type | pols | corr_type;

  // Work out the number of pols.
  plotcontrols->npols = 0;
  if (pols & PLOT_POL_XX) {
    plotcontrols->npols++;
  }
  if (pols & PLOT_POL_YY) {
    plotcontrols->npols++;
  }
  if (pols & PLOT_POL_XY) {
    plotcontrols->npols++;
  }
  if (pols & PLOT_POL_YX) {
    plotcontrols->npols++;
  }
  
  plotcontrols->channel_range_limit = NO;
  plotcontrols->yaxis_range_limit = NO;
  for (i = 0; i < MAXIFS; i++) {
    plotcontrols->if_num_spec[i] = 1;
  }
  plotcontrols->array_spec = 0;
  for (i = 1; i <= MAXANTS; i++) {
    plotcontrols->array_spec |= 1<<i;
  }
}

void splitpanels(int nx, int ny, struct panelspec *panelspec) {
  int i, j;
  float panel_width, panel_height, margin_reduction = 5;
  float padding_fraction = 2, padding_x, padding_y;
  float orig_charheight, charheight;

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
  // Reduce the margins.
  panelspec->orig_x1 /= margin_reduction;
  panelspec->orig_x2 = 1 - panelspec->orig_x1;
  panelspec->orig_y1 /= margin_reduction;
  panelspec->orig_y2 = 1 - panelspec->orig_y1;
  printf("viewport is x = %.2f -> %.2f, y = %.2f -> %.2f\n",
	 panelspec->orig_x1, panelspec->orig_x2,
	 panelspec->orig_y1, panelspec->orig_y2);
  // Space between the panels should be some fraction of the margin.
  padding_x = panelspec->orig_x1 * padding_fraction;
  padding_y = panelspec->orig_y1 * padding_fraction;
  
  panel_width = (panelspec->orig_x2 - panelspec->orig_x1 -
		 (float)(nx - 1) * padding_x) / (float)nx;
  panel_height = (panelspec->orig_y2 - panelspec->orig_y1 -
		  (float)(ny - 1) * padding_y) / (float)ny;

  for (i = 0; i < nx; i++) {
    for (j = 0; j < ny; j++) {
      panelspec->x1[i][j] = panelspec->orig_x1 + i * (panel_width + padding_x);
      panelspec->x2[i][j] = panelspec->x1[i][j] + panel_width;
      panelspec->y2[i][j] = panelspec->orig_y2 - j * (panel_height + padding_y);
      panelspec->y1[i][j] = panelspec->y2[i][j] - panel_height;
    }
  }

  // Change the character height.
  cpgqch(&orig_charheight);
  charheight = orig_charheight / 2;
  cpgsch(charheight);
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

void plotnum_to_xy(struct panelspec *panelspec, int plotnum,
		   int *px, int *py) {
  *px = plotnum % panelspec->nx;
  *py = (int)((plotnum - *px) / panelspec->nx);
}

void plotpanel_minmax(struct ampphase **plot_ampphase,
		      struct plotcontrols *plot_controls,
		      int plot_baseline_idx, int npols, int *polidx,
		      float *plotmin_x, float *plotmax_x,
		      float *plotmin_y, float *plotmax_y) {
  // This routine does the computation required to work out what the
  // axis ranges of a single plot with plotoptions will be. It takes into
  // account whether it's an auto or cross correlation, and the polarisations
  // that are being plotted.
  int i = 0, j = 0, bltype, ant1, ant2;
  float swapf;

  // Get the x-axis range first.
  if (plot_controls->plot_options & PLOT_CHANNEL) {
    *plotmin_x = 0;
    *plotmax_x = plot_ampphase[0]->nchannels;
    if (plot_controls->channel_range_limit == YES) {
      if ((plot_controls->channel_range_min >= 0) &&
	  (plot_controls->channel_range_min < plot_ampphase[0]->nchannels)) {
	*plotmin_x = plot_controls->channel_range_min;
      }
      if ((plot_controls->channel_range_max > 0) &&
	  (plot_controls->channel_range_max < plot_ampphase[0]->nchannels) &&
	  (plot_controls->channel_range_max > *plotmin_x)) {
	*plotmax_x = plot_controls->channel_range_max;
      }
    }
  } else if (plot_controls->plot_options & PLOT_FREQUENCY) {
    *plotmin_x = plot_ampphase[0]->frequency[0];
    *plotmax_x = plot_ampphase[0]->frequency
      [plot_ampphase[0]->nchannels - 1];
    if (plot_controls->channel_range_limit == YES) {
      if ((plot_controls->channel_range_min >= 0) &&
	  (plot_controls->channel_range_min < plot_ampphase[0]->nchannels)) {
	*plotmin_x = plot_ampphase[0]->frequency
	  [plot_controls->channel_range_min];
      }
      if ((plot_controls->channel_range_max > 0) &&
	  (plot_controls->channel_range_max < plot_ampphase[0]->nchannels) &&
	  (plot_controls->channel_range_max > plot_controls->channel_range_min)) {
	*plotmax_x = plot_ampphase[0]->frequency
	  [plot_controls->channel_range_max];
      }
    }
    // Check for frequency inversion.
    if (*plotmin_x > *plotmax_x) {
      swapf = *plotmin_x;
      *plotmin_x = *plotmax_x;
      *plotmax_x = swapf;
    }
  }
    
  // Now the y-axis. If we've been given a range by the user,
  // we can set that and return immediately.
  if (plot_controls->yaxis_range_limit == YES) {
    *plotmin_y = plot_controls->yaxis_range_min;
    *plotmax_y = plot_controls->yaxis_range_max;
    return;
  }
  
  // If we have no pols to plot, we're done.
  if (npols == 0) {
    *plotmin_y = 0;
    *plotmax_y = 1;
    return;
  }
  
  // Get the initial value for min and max.
  if (plot_controls->plot_options & PLOT_AMPLITUDE) {
    *plotmin_y = plot_ampphase[polidx[0]]->min_amplitude[plot_baseline_idx];
    *plotmax_y = plot_ampphase[polidx[0]]->max_amplitude[plot_baseline_idx];
  } else if (plot_controls->plot_options & PLOT_PHASE) {
    *plotmin_y = plot_ampphase[polidx[0]]->min_phase[plot_baseline_idx];
    *plotmax_y = plot_ampphase[polidx[0]]->max_phase[plot_baseline_idx];
  } 

  // Account for all the other polarisations.
  for (i = 1; i < npols; i++) {
    if (plot_controls->plot_options & PLOT_AMPLITUDE) {
      MINASSIGN(*plotmin_y,
		plot_ampphase[polidx[i]]->min_amplitude[plot_baseline_idx]);
      MAXASSIGN(*plotmax_y,
		plot_ampphase[polidx[i]]->max_amplitude[plot_baseline_idx]);
    } else if (plot_controls->plot_options & PLOT_PHASE) {
      MINASSIGN(*plotmin_y,
		plot_ampphase[polidx[i]]->min_phase[plot_baseline_idx]);
      MAXASSIGN(*plotmax_y,
		plot_ampphase[polidx[i]]->max_phase[plot_baseline_idx]);
    }
  }
  
  // Refine the min/max values.
  if (plot_controls->plot_options & PLOT_CONSISTENT_YRANGE) {
    // The user wants us to keep a consistent Y axis range for
    // all the plots. But we also need a different Y axis range for
    // auto and cross correlations. So we need to know what type of
    // correlation the specified baseline is first.
    base_to_ants(plot_ampphase[0]->baseline[plot_baseline_idx], &ant1, &ant2);
    if (ant1 == ant2) {
      // Autocorrelation.
      bltype = 0;
    } else {
      bltype = 1;
    }
    for (i = 0; i < plot_ampphase[0]->nbaselines; i++) {
      base_to_ants(plot_ampphase[0]->baseline[i], &ant1, &ant2);
      // TODO: check the antennas are in the array spec.
      if (((ant1 == ant2) && (bltype == 0)) ||
	  ((ant1 != ant2) && (bltype == 1))) {
	for (j = 0; j < npols; j++) {
	  if (plot_controls->plot_options & PLOT_AMPLITUDE) {
	    MINASSIGN(*plotmin_y,
		      plot_ampphase[polidx[j]]->min_amplitude[i]);
	    MAXASSIGN(*plotmax_y,
		      plot_ampphase[polidx[j]]->max_amplitude[i]);
	  } else if (plot_controls->plot_options & PLOT_PHASE) {
	    MINASSIGN(*plotmin_y,
		      plot_ampphase[polidx[j]]->min_phase[i]);
	    MAXASSIGN(*plotmax_y,
		      plot_ampphase[polidx[j]]->max_phase[i]);
	  }
	}
      }
    }
  }

  // Check that the min and max are not the same.
  if (*plotmin_x == *plotmax_x) {
    *plotmin_x -= 1;
    *plotmax_x += 1;
  }
  if (*plotmin_y == *plotmax_y) {
    *plotmin_y -= 1;
    *plotmax_y += 1;
  }
}

int find_pol(struct ampphase ***cycle_ampphase, int npols, int ifnum, int poltype) {
  int i;
  for (i = 0; i < npols; i++) {
    if (cycle_ampphase[ifnum][i]->pol == poltype) {
      return i;
    }
  }
  return -1;
}

void make_plot(struct ampphase ***cycle_ampphase, struct panelspec *panelspec,
	       struct plotcontrols *plot_controls) {
  int x = 0, y = 0, i, j, ant1, ant2, nants = 0, px, py, iauto = 0, icross = 0;
  int npols = 0, *polidx = NULL, poli, num_ifs = 0, panels_per_if = 0;
  int idxif, ni, ri, rj, rp, freq_malloced = 0;
  float xaxis_min, xaxis_max, yaxis_min, yaxis_max, theight = 0.4;
  float **freq_amp = NULL, **freq_phase = NULL, *freq_ordered = NULL;
  char ptitle[BUFSIZE], ptype[BUFSIZE];
  struct ampphase **ampphase_if = NULL;

  // Work out how many antennas we will show.
  for (i = 1, nants = 0; i <= MAXANTS; i++) {
    if ((1 << i) & plot_controls->array_spec) {
      nants++;
    }
  }
  if (nants == 0) {
    // Nothing to plot!
    return;
  }

  // How many panels per IF?
  if (plot_controls->plot_options & PLOT_AUTOCORRELATIONS) {
    panels_per_if += nants;
  }
  if (plot_controls->plot_options & PLOT_CROSSCORRELATIONS) {
    panels_per_if += (nants * (nants - 1)) / 2;
  }
  
  changepanel(-1, -1, panelspec);
  cpgpage();
  for (idxif = 0, ni = 0; idxif < MAXIFS; idxif++) {
    iauto = 0;
    icross = 0;
    if (plot_controls->if_num_spec[idxif]) {
      ampphase_if = cycle_ampphase[ni];
      if (ampphase_if == NULL) {
	continue;
      }
      
      // Which polarisations are we plotting?
      if (plot_controls->plot_options & PLOT_POL_XX) {
	poli = find_pol(cycle_ampphase, plot_controls->npols, ni, POL_XX);
	if (poli >= 0) {
	  REALLOC(polidx, ++npols);
	  polidx[npols - 1] = poli;
	}
      }
      if (plot_controls->plot_options & PLOT_POL_YY) {
	poli = find_pol(cycle_ampphase, plot_controls->npols, ni, POL_YY);
	if (poli >= 0) {
	  REALLOC(polidx, ++npols);
	  polidx[npols - 1] = poli;
	}
      }
      if (plot_controls->plot_options & PLOT_POL_XY) {
	poli = find_pol(cycle_ampphase, plot_controls->npols, ni, POL_XY);
	if (poli >= 0) {
	  REALLOC(polidx, ++npols);
	  polidx[npols - 1] = poli;
	}
      }
      if (plot_controls->plot_options & PLOT_POL_YX) {
	poli = find_pol(cycle_ampphase, plot_controls->npols, ni, POL_YX);
	if (poli >= 0) {
	  REALLOC(polidx, ++npols);
	  polidx[npols - 1] = poli;
	}
      }

      if (plot_controls->plot_options & PLOT_FREQUENCY) {
	MALLOC(freq_amp, npols);
	MALLOC(freq_phase, npols);
	for (rp = 0; rp < npols; rp++) {
	  freq_amp[rp] = NULL;
	  freq_phase[rp] = NULL;
	}
      }
      
      for (i = 0; i < ampphase_if[0]->nbaselines; i++) {
	// Work out the antennas in this baseline.
	base_to_ants(ampphase_if[0]->baseline[i], &ant1, &ant2);
	// Check if we are plotting both of these antenna.
	if (((1 << ant1) & plot_controls->array_spec) &&
	    ((1 << ant2) & plot_controls->array_spec)) {
	  // Work out which panel to use.
	  if ((ant1 == ant2) &&
	      (plot_controls->plot_options & PLOT_AUTOCORRELATIONS)) {
	    px = (num_ifs * panels_per_if + iauto) % panelspec->nx;
	    py = (int)((num_ifs * panels_per_if + iauto - px) / panelspec->nx);
	    iauto++;
	  } else if ((ant1 != ant2) &&
		     (plot_controls->plot_options & PLOT_CROSSCORRELATIONS)) {
	    if (plot_controls->plot_options & PLOT_AUTOCORRELATIONS) {
	      px = (num_ifs * panels_per_if + nants + icross) % panelspec->nx;
	      py = (int)((num_ifs * panels_per_if + nants + icross - px) /
			 panelspec->nx);
	    } else {
	      px = (num_ifs * panels_per_if + icross) % panelspec->nx;
	      py = (int)((num_ifs * panels_per_if + icross - px) / panelspec->nx);
	    }
	    icross++;
	  }
	  // Check if we've exceeded the space for this plot.
	  if (py >= panelspec->ny) {
	    continue;
	  }
	  // Set the panel.
	  changepanel(px, py, panelspec);
	  // Set the title for the plot.
	  if (plot_controls->plot_options & PLOT_AMPLITUDE) {
	    snprintf(ptype, BUFSIZE, "AMPL.");
	  } else if (plot_controls->plot_options & PLOT_PHASE) {
	    snprintf(ptype, BUFSIZE, "PHASE");
	  }
	  snprintf(ptitle, BUFSIZE, "%s: FQ:%d BSL%d%d",
		   ptype, (idxif + 1), ant1, ant2);

	  // Check if we need to make an inverted frequency array.
	  if (plot_controls->plot_options & PLOT_FREQUENCY) {
	    if (ampphase_if[0]->f_frequency[i][0] >
		ampphase_if[0]->f_frequency[i]
		[ampphase_if[0]->f_nchannels[i] - 1]) {
	      // Inverted band.
	      if (freq_malloced == 1) {
		FREE(freq_ordered);
		for (rp = 0; rp < npols; rp++) {
		  if (plot_controls->plot_options & PLOT_AMPLITUDE) {
		    FREE(freq_amp[rp]);
		  } else {
		    FREE(freq_phase[rp]);
		  }
		}
	      }
	      MALLOC(freq_ordered, ampphase_if[0]->f_nchannels[i]);
	      for (rp = 0; rp < npols; rp++) {
		if (plot_controls->plot_options & PLOT_AMPLITUDE) {
		  MALLOC(freq_amp[rp], ampphase_if[polidx[rp]]->f_nchannels[i]);
		} else {
		  MALLOC(freq_phase[rp], ampphase_if[polidx[rp]]->f_nchannels[i]);
		}
	      }
	      freq_malloced = 1;
	      for (rp = 0; rp < npols; rp++) {
		for (ri = 0, rj = ampphase_if[0]->f_nchannels[i] - 1;
		     ri < ampphase_if[0]->f_nchannels[i]; ri++, rj--) {
		  // Swap the frequencies.
		  if (rp == 0) {
		    freq_ordered[ri] = ampphase_if[polidx[rp]]->f_frequency[i][rj];
		  }
		  if (plot_controls->plot_options & PLOT_AMPLITUDE) {
		    freq_amp[rp][ri] = ampphase_if[polidx[rp]]->f_amplitude[i][rj];
		  } else {
		    freq_phase[rp][ri] = ampphase_if[polidx[rp]]->f_phase[i][rj];
		  }
		}
	      }
	    } else {
	      if (freq_malloced) {
		FREE(freq_ordered);
		for (rp = 0; rp < npols; rp++) {
		  FREE(freq_amp[rp]);
		  FREE(freq_phase[rp]);
		}
	      }
	      freq_ordered = ampphase_if[polidx[rp]]->f_frequency[i];
	      for (rp = 0; rp < npols; rp++) {
		freq_amp[rp] = ampphase_if[polidx[rp]]->f_amplitude[i];
		freq_phase[rp] = ampphase_if[polidx[rp]]->f_phase[i];
	      }
	      freq_malloced = 0;
	    }
	  }

	  
	  plotpanel_minmax(ampphase_if, plot_controls, i, npols, polidx,
			   &xaxis_min, &xaxis_max, &yaxis_min, &yaxis_max);
	  /* printf("max/max x = %.6f / %.6f, y = %.6f / %.6f\n", */
	  /* 	 xaxis_min, xaxis_max, yaxis_min, yaxis_max); */
	  cpgsci(1);
	  cpgswin(xaxis_min, xaxis_max, yaxis_min, yaxis_max);
	  cpgbox("BCNTS", 0, 0, "BCNTS", 0, 0);
	  cpgmtxt("T", theight, 0.5, 0.5, ptitle);
	  
	  for (j = 0; j < npols; j++) {
	    cpgsci(polidx[j] + 1);
	    if (plot_controls->plot_options & PLOT_AMPLITUDE) {
	      if (plot_controls->plot_options & PLOT_CHANNEL) {
		cpgline(ampphase_if[polidx[j]]->f_nchannels[i],
			ampphase_if[polidx[j]]->f_channel[i],
			ampphase_if[polidx[j]]->f_amplitude[i]);
	      } else if (plot_controls->plot_options & PLOT_FREQUENCY) {
		cpgline(ampphase_if[polidx[j]]->f_nchannels[i],
			freq_ordered, freq_amp[j]);
	      }
	    } else if (plot_controls->plot_options & PLOT_PHASE) {
	      if (plot_controls->plot_options & PLOT_CHANNEL) {
		cpgline(ampphase_if[polidx[j]]->f_nchannels[i],
			ampphase_if[polidx[j]]->f_channel[i],
			ampphase_if[polidx[j]]->f_phase[i]);
	      } else if (plot_controls->plot_options & PLOT_FREQUENCY) {
		cpgline(ampphase_if[polidx[j]]->f_nchannels[i],
			freq_ordered, freq_phase[j]);
	      }
	    }
	  }
	}
      }
      num_ifs++;
      ni++;
      if (freq_malloced == 1) {
	FREE(freq_ordered);
	for (rp = 0; rp < npols; rp++) {
	  if (plot_controls->plot_options & PLOT_AMPLITUDE) {
	    FREE(freq_amp[rp]);
	  } else if (plot_controls->plot_options & PLOT_PHASE) {
	    FREE(freq_phase[rp]);
	  }
	}
	FREE(freq_amp);
	FREE(freq_phase);
	freq_malloced = 0;
      }
    }
  }
  
}
