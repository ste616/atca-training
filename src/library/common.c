/**
 * ATCA Training Library: common.c
 * (C) Jamie Stevens CSIRO 2019
 *
 * This library contains functions that are useful for many of the
 * applications.
 */
#include <string.h>
#include <math.h>
#include "common.h"
#include "cpgplot.h"
#include "memory.h"

int interpret_array_string(char *array_string) {
  // The array string should be a comma-separated list of antenna.
  int a = 0, r = 0;
  char *token, copy[BUFSIZE];
  const char s[2] = ",";

  // We make a copy of the string since strtok is destructive.
  (void)strncpy(copy, array_string, BUFSIZE);
  token = strtok(copy, s);
  while (token != NULL) {
    a = atoi(token);
    if ((a >= 1) && (a <= MAXANTS)) {
      r |= 1<<a;
    }
    token = strtok(NULL, s);
  }

  return r;
}

void init_spd_plotcontrols(struct spd_plotcontrols *plotcontrols,
			   int xaxis_type, int yaxis_type, int pols,
			   int corr_type, int pgplot_device) {
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

  plotcontrols->interactive = YES;

  // Keep the PGPLOT device.
  plotcontrols->pgplot_device = pgplot_device;
}

#define NAVAILABLE_PANELS 3

void init_vis_plotcontrols(struct vis_plotcontrols *plotcontrols,
			   int xaxis_type, int paneltypes, int *visbands,
			   int pgplot_device,
			   struct panelspec *panelspec) {
  int npanels = 0, i, j;
  int available_panels[NAVAILABLE_PANELS] = { PLOT_AMPLITUDE,
					      PLOT_PHASE,
					      PLOT_DELAY };
  
  // Initialise the plotcontrols structure and form the panelspec
  // structure using the same information.
  if (xaxis_type == DEFAULT) {
    xaxis_type = PLOT_TIME;
  }
  // Count the number of panels we need.
  for (i = 0; i < NAVAILABLE_PANELS; i++) {
    if (paneltypes & available_panels[i]) {
      npanels++;
    }
  }
  plotcontrols->num_panels = npanels;
  // Order the panels.
  MALLOC(plotcontrols->panel_type, plotcontrols->num_panels);
  for (i = 0, j = 0; i < NAVAILABLE_PANELS; i++) {
    if (paneltypes & available_panels[i]) {
      plotcontrols->panel_type[j] = available_panels[i];
      j++;
    }
  }
  plotcontrols->plot_options = xaxis_type | paneltypes;

  // Keep the PGPLOT device.
  plotcontrols->pgplot_device = pgplot_device;
  
  // Set up the panelspec structure for this.
  splitpanels(1, npanels, pgplot_device, 1, 1, panelspec);

  // Initialise the products to display.
  plotcontrols->nproducts = 0;
  plotcontrols->vis_products = NULL;

  for (i = 0; i < 2; i++) {
    plotcontrols->visbands[i] = visbands[i];
  }
}

void free_panelspec(struct panelspec *panelspec) {
  int i;
  for (i = 0; i < panelspec->nx; i++) {
    FREE(panelspec->x1[i]);
    FREE(panelspec->x2[i]);
    FREE(panelspec->y1[i]);
    FREE(panelspec->y2[i]);
    FREE(panelspec->px_x1[i]);
    FREE(panelspec->px_x2[i]);
    FREE(panelspec->px_y1[i]);
    FREE(panelspec->px_y2[i]);
  }
  FREE(panelspec->x1);
  FREE(panelspec->x2);
  FREE(panelspec->y1);
  FREE(panelspec->y2);
  FREE(panelspec->px_x1);
  FREE(panelspec->px_x2);
  FREE(panelspec->px_y1);
  FREE(panelspec->px_y2);
}

void splitpanels(int nx, int ny, int pgplot_device, int abut,
		 float margin_reduction,
		 struct panelspec *panelspec) {
  int i, j;
  float panel_width, panel_height, panel_px_width, panel_px_height;
  float padding_fraction = 2, padding_x, padding_y;
  float padding_px_x, padding_px_y, dpx_x, dpx_y;
  float orig_charheight, charheight;

  // Allocate some memory.
  panelspec->nx = nx;
  panelspec->ny = ny;
  MALLOC(panelspec->x1, nx);
  MALLOC(panelspec->x2, nx);
  MALLOC(panelspec->y1, nx);
  MALLOC(panelspec->y2, nx);
  MALLOC(panelspec->px_x1, nx);
  MALLOC(panelspec->px_x2, nx);
  MALLOC(panelspec->px_y1, nx);
  MALLOC(panelspec->px_y2, nx);
  for (i = 0; i < nx; i++) {
    MALLOC(panelspec->x1[i], ny);
    MALLOC(panelspec->x2[i], ny);
    MALLOC(panelspec->y1[i], ny);
    MALLOC(panelspec->y2[i], ny);
    MALLOC(panelspec->px_x1[i], ny);
    MALLOC(panelspec->px_x2[i], ny);
    MALLOC(panelspec->px_y1[i], ny);
    MALLOC(panelspec->px_y2[i], ny);
  }

  // Get the original settings.
  cpgslct(pgplot_device);
  cpgqvp(3, &(panelspec->orig_px_x1), &(panelspec->orig_px_x2),
	 &(panelspec->orig_px_y1), &(panelspec->orig_px_y2));
  cpgqvp(0, &(panelspec->orig_x1), &(panelspec->orig_x2),
	 &(panelspec->orig_y1), &(panelspec->orig_y2));
  // Reduce the margins.
  panelspec->orig_x1 /= margin_reduction;
  dpx_x = panelspec->orig_px_x1 - (panelspec->orig_px_x1 / margin_reduction);
  dpx_y = panelspec->orig_px_y1 - (panelspec->orig_px_y1 / margin_reduction);
  panelspec->orig_px_x1 /= margin_reduction;
  panelspec->orig_x2 = 1 - panelspec->orig_x1;
  panelspec->orig_px_x2 += dpx_x;
  panelspec->orig_y1 /= margin_reduction;
  panelspec->orig_px_y1 /= margin_reduction;
  panelspec->orig_y2 = 1 - panelspec->orig_y1;
  panelspec->orig_px_y2 += dpx_y;
  /* printf("viewport is x = %.2f -> %.2f, y = %.2f -> %.2f\n", */
  /* 	 panelspec->orig_x1, panelspec->orig_x2, */
  /* 	 panelspec->orig_y1, panelspec->orig_y2); */
  if (abut == 0) {
    // Space between the panels should be some fraction of the margin.
    padding_x = panelspec->orig_x1 * padding_fraction;
    padding_y = panelspec->orig_y1 * padding_fraction;
    padding_px_x = panelspec->orig_px_x1 * padding_fraction;
    padding_px_y = panelspec->orig_px_y1 * padding_fraction;
  } else {
    // No space between the panels.
    padding_x = 0;
    padding_y = 0;
    padding_px_x = 0;
    padding_px_y = 0;
  }
  
  panel_width = (panelspec->orig_x2 - panelspec->orig_x1 -
		 (float)(nx - 1) * padding_x) / (float)nx;
  panel_px_width = (panelspec->orig_px_x2 - panelspec->orig_px_x1 -
		 (float)(nx - 1) * padding_px_x) / (float)nx;
  panel_height = (panelspec->orig_y2 - panelspec->orig_y1 -
		  (float)(ny - 1) * padding_y) / (float)ny;
  panel_px_height = (panelspec->orig_px_y2 - panelspec->orig_px_y1 -
		  (float)(ny - 1) * padding_px_y) / (float)ny;

  for (i = 0; i < nx; i++) {
    for (j = 0; j < ny; j++) {
      panelspec->x1[i][j] = panelspec->orig_x1 + i * (panel_width + padding_x);
      panelspec->x2[i][j] = panelspec->x1[i][j] + panel_width;
      panelspec->y2[i][j] = panelspec->orig_y2 - j * (panel_height + padding_y);
      panelspec->y1[i][j] = panelspec->y2[i][j] - panel_height;
      panelspec->px_x1[i][j] = panelspec->orig_px_x1 +
	i * (panel_px_width + padding_px_x);
      panelspec->px_x2[i][j] = panelspec->px_x1[i][j] + panel_px_width;
      panelspec->px_y2[i][j] = panelspec->orig_px_y2 -
	j * (panel_px_height + padding_px_y);
      panelspec->px_y1[i][j] = panelspec->px_y2[i][j] - panel_px_height;
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
		      struct spd_plotcontrols *plot_controls,
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

void pol_to_vis_name(int pol, int if_num, char *vis_name) {
  if (pol & PLOT_POL_XX) {
    if (if_num == 1) {
      strcpy(vis_name, "AA");
    } else if (if_num == 2) {
      strcpy(vis_name, "CC");
    }
  } else if (pol & PLOT_POL_YY) {
    if (if_num == 1) {
      strcpy(vis_name, "BB");
    } else if (if_num == 2) {
      strcpy(vis_name, "DD");
    }
  } else if (pol & PLOT_POL_XY) {
    if (if_num == 1) {
      strcpy(vis_name, "AB");
    } else if (if_num == 2) {
      strcpy(vis_name, "CD");
    }
  } else if (pol & PLOT_POL_YX) {
    if (if_num == 1) {
      strcpy(vis_name, "BA");
    } else if (if_num == 2) {
      strcpy(vis_name, "DC");
    }
  }
}

void add_vis_line(struct vis_line ***vis_lines, int *n_vis_lines,
		  int ant1, int ant2, int if_number, int pol) {
  struct vis_line *new_line = NULL;
  char polname[BUFSIZE];
  // Add a potential vis line.
  *n_vis_lines += 1;
  REALLOC(*vis_lines, *n_vis_lines);
  MALLOC(new_line, 1);
  new_line->ant1 = ant1;
  new_line->ant2 = ant2;
  new_line->if_number = if_number;
  new_line->pol = pol;
  pol_to_vis_name(new_line->pol, new_line->if_number, polname);
  snprintf(new_line->label, BUFSIZE, "%d%d%s",
	   new_line->ant1, new_line->ant2, polname);
  (*vis_lines)[*n_vis_lines - 1] = new_line;
}

void vis_interpret_pol(char *pol, struct vis_product *vis_product) {
  if (strcmp(pol, "aa") == 0) {
    vis_product->if_spec = VIS_PLOT_IF1;
    vis_product->pol_spec = PLOT_POL_XX;
  } else if (strcmp(pol, "bb") == 0) {
    vis_product->if_spec = VIS_PLOT_IF1;
    vis_product->pol_spec = PLOT_POL_YY;
  } else if (strcmp(pol, "ab") == 0) {
    vis_product->if_spec = VIS_PLOT_IF1;
    vis_product->pol_spec = PLOT_POL_XY;
  } else if (strcmp(pol, "a") == 0) {
    vis_product->if_spec = VIS_PLOT_IF1;
    vis_product->pol_spec = PLOT_POL_XX | PLOT_POL_XY;
  } else if (strcmp(pol, "b") == 0) {
    vis_product->if_spec = VIS_PLOT_IF1;
    vis_product->pol_spec = PLOT_POL_YY | PLOT_POL_XY;
  } else if (strcmp(pol, "cc") == 0) {
    vis_product->if_spec = VIS_PLOT_IF2;
    vis_product->pol_spec = PLOT_POL_XX;
  } else if (strcmp(pol, "dd") == 0) {
    vis_product->if_spec = VIS_PLOT_IF2;
    vis_product->pol_spec = PLOT_POL_YY;
  } else if (strcmp(pol, "cd") == 0) {
    vis_product->if_spec = VIS_PLOT_IF2;
    vis_product->pol_spec = PLOT_POL_XY;
  } else if (strcmp(pol, "c") == 0) {
    vis_product->if_spec = VIS_PLOT_IF2;
    vis_product->pol_spec = PLOT_POL_XX | PLOT_POL_XY;
  } else if (strcmp(pol, "d") == 0) {
    vis_product->if_spec = VIS_PLOT_IF2;
    vis_product->pol_spec = PLOT_POL_YY | PLOT_POL_XY;
  }
}

void vis_interpret_product(char *product, struct vis_product **vis_product) {
  int ant1, ant2, nmatch = 0, i;
  char p1[3];

  // Work out if we need to make a vis_product.
  if (*vis_product == NULL) {
    MALLOC(*vis_product, 1);
  }
  
  nmatch = sscanf(product, "%1d%1d%2s", &ant1, &ant2, p1);
  if (nmatch == 3) {
    // Everything was specified.
    (*vis_product)->antenna_spec = 1<<ant1 | 1<<ant2;
    vis_interpret_pol(p1, *vis_product);
  } else if (nmatch == 2) {
    // The character wasn't matched.
    (*vis_product)->antenna_spec = 1<<ant1 | 1<<ant2;
    nmatch = sscanf(product, "%1d%1d%1s", &ant1, &ant2, p1);
    if (nmatch == 3) {
      // We were just given a single pol indicator.
      vis_interpret_pol(p1, *vis_product);
    } else {
      // We were only given baselines.
      (*vis_product)->if_spec = VIS_PLOT_IF1 | VIS_PLOT_IF2;
      (*vis_product)->pol_spec = PLOT_POL_XX | PLOT_POL_YY | PLOT_POL_XY;
    }
  } else if (nmatch == 1) {
    // We were given the first antenna number.
    (*vis_product)->antenna_spec = 1<<ant1;
    nmatch = sscanf(product, "%1d%2s", &ant1, p1);
    if (nmatch == 2) {
      vis_interpret_pol(p1, *vis_product);
    } else {
      nmatch = sscanf(product, "%1d%1s", &ant1, p1);
      if (nmatch == 2) {
	vis_interpret_pol(p1, *vis_product);
      } else {
	// Only given the first antenna number.
	(*vis_product)->if_spec = VIS_PLOT_IF1 | VIS_PLOT_IF2;
	(*vis_product)->pol_spec = PLOT_POL_XX | PLOT_POL_YY | PLOT_POL_XY;
      }
    }
  } else {
    // We weren't given any antenna specifications.
    (*vis_product)->antenna_spec = 0;
    for (i = 1; i <= MAXANTS; i++) {
      (*vis_product)->antenna_spec |= 1<<i;
    }
    nmatch = sscanf(product, "%2s", p1);
    if (nmatch == 1) {
      vis_interpret_pol(p1, *vis_product);
    } else {
      nmatch = sscanf(product, "%1s", p1);
      if (nmatch == 1) {
	vis_interpret_pol(p1, *vis_product);
      } else {
	(*vis_product)->if_spec = VIS_PLOT_IF1 | VIS_PLOT_IF2;
	(*vis_product)->pol_spec = PLOT_POL_XX | PLOT_POL_YY | PLOT_POL_XY;
      }
    }
  }
}

int find_if_name(struct scan_header_data *scan_header_data,
		 char *name) {
  int res = -1, i, j;
  for (i = 0; i < scan_header_data->num_ifs; i++) {
    for (j = 0; j < 3; j++) {
      if (strcmp(scan_header_data->if_name[i][j], name) == 0) {
	res = scan_header_data->if_label[i];
	break;
      }
    }
    if (res >= 0) {
      break;
    }
  }

  if (res < 0) {
    res = 1; // For safety.
  }
  
  return res;
}

float fracwidth(struct panelspec *panelspec,
		float axis_min_x, float axis_max_x, int x, int y, char *label) {
  // This routine works out the fractional width of the string in label,
  // compared to the entire width of the box.
  float dx, xc[4], yc[4], dlx;
  int i;

  dx = fabsf(axis_max_x - axis_min_x);
  cpgqtxt(axis_min_x, 0, 0, 0, label, xc, yc);
  dlx = fabsf(xc[2] - xc[1]);
  return (dlx / dx);
}

void make_vis_plot(struct vis_quantities ****cycle_vis_quantities,
		   int ncycles, int *cycle_numifs, int npols,
		   struct panelspec *panelspec,
		   struct vis_plotcontrols *plot_controls) {
  int nants = 0, i = 0, n_vis_lines = 0, j = 0, k = 0, p = 0;
  int singleant = 0, l = 0, m = 0, n = 0;
  int **n_plot_lines = NULL;
  float ****plot_lines = NULL, min_x, max_x, min_y, max_y;
  float cxpos, dxpos;
  char xopts[BUFSIZE], yopts[BUFSIZE], panellabel[BUFSIZE], panelunits[BUFSIZE];
  char antstring[BUFSIZE];
  struct vis_line **vis_lines = NULL;

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

  // Select the PGPLOT device.
  cpgslct(plot_controls->pgplot_device);

  // We make up to 16 lines per panel.
  // The first dimension will be the panel number.
  MALLOC(plot_lines, plot_controls->num_panels);
  MALLOC(n_plot_lines, plot_controls->num_panels);
  // The second dimension will be the line number.
  // We need to work out how to allocate up to 16 lines.
  for (p = 0; p < plot_controls->nproducts; p++) {
    // Check if only a single antenna is requested.
    for (i = 1; i <= MAXANTS; i++) {
      if ((plot_controls->vis_products[p]->antenna_spec == (1<<i)) &&
	  (plot_controls->array_spec & (1<<i))) {
	singleant = i;
	break;
      }
    }
    for (i = 1, n_vis_lines = 0; i <= MAXANTS; i++) {
      if (((1 << i) & plot_controls->array_spec) &&
	  (((1 << i) & plot_controls->vis_products[p]->antenna_spec) ||
	   (singleant > i))) {
	for (j = i; j <= MAXANTS; j++) {
	  if (((1 << j) & plot_controls->array_spec) &&
	      ((1 << j) & plot_controls->vis_products[p]->antenna_spec)) {
	    if (i != j) {
	      // Cross-correlations allow XX and YY.
	      if (plot_controls->vis_products[p]->pol_spec & PLOT_POL_XX) {
		if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF1) {
		  add_vis_line(&vis_lines, &n_vis_lines,
			       i, j, plot_controls->visbands[0], POL_XX);
		}
		if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF2) {
		  add_vis_line(&vis_lines, &n_vis_lines,
			       i, j, plot_controls->visbands[1], POL_XX);
		}
	      }
	      if (plot_controls->vis_products[p]->pol_spec & PLOT_POL_YY) {
		if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF1) {
		  add_vis_line(&vis_lines, &n_vis_lines,
			       i, j, plot_controls->visbands[0], POL_YY);
		}
		if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF2) {
		  add_vis_line(&vis_lines, &n_vis_lines,
			       i, j, plot_controls->visbands[1], POL_YY);
		}
	      }
	    } else {
	      // Auto-correlations allow XY and YX.
	      if (plot_controls->vis_products[p]->pol_spec & PLOT_POL_XY) {
		if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF1) {
		  add_vis_line(&vis_lines, &n_vis_lines,
			       i, j, plot_controls->visbands[0], POL_XY);
		}
		if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF2) {
		  add_vis_line(&vis_lines, &n_vis_lines,
			       i, j, plot_controls->visbands[1], POL_XY);
		}
	      }
	    }
	  }
	}
      }
    }
  }
  printf("generating %d vis lines\n", n_vis_lines);
  if (n_vis_lines == 0) {
    // Nothing to plot!
    return;
  }

  // Sort the lines; TODO, for now just take the first 16.
  if (n_vis_lines > 16) {
    n_vis_lines = 16;
  }
  for (i = 0; i < n_vis_lines; i++) {
    vis_lines[i]->pgplot_colour = (i + 1);
  }
  
  for (i = 0; i < plot_controls->num_panels; i++) {
    // Make the lines.
    MALLOC(plot_lines[i], n_vis_lines);
    MALLOC(n_plot_lines[i], n_vis_lines);

    // Keep track of the min and max values to plot.
    min_x = INFINITY;
    max_x = -INFINITY;
    min_y = INFINITY;
    max_y = -INFINITY;
    
    // The third dimension of the plot lines will be the X and Y
    // coordinates.
    for (j = 0; j < n_vis_lines; j++) {
      n_plot_lines[i][j] = 0;
      MALLOC(plot_lines[i][j], 2);
      plot_lines[i][j][0] = NULL;
      plot_lines[i][j][1] = NULL;
      // The fourth dimension is the points to plot in the line.
      // We accumulate these now.
      for (k = 0; k < ncycles; k++) {
	//printf(" cycle %d has %d IFs\n", k, cycle_numifs[k]);
	for (l = 0; l < cycle_numifs[k]; l++) {
	  for (m = 0; m < npols; m++) {
	    /* printf("comparing pols %d to %d, window %d to %d\n", */
	    /* 	   cycle_vis_quantities[k][l][m]->pol, vis_lines[j]->pol, */
	    /* 	   cycle_vis_quantities[k][l][m]->window, vis_lines[j]->if_number); */
	    if ((cycle_vis_quantities[k][l][m]->pol &
		 vis_lines[j]->pol) &&
		(cycle_vis_quantities[k][l][m]->window ==
		 vis_lines[j]->if_number)) {
	      //printf("   found appropriate vis quantity!\n");
	      // Find the correct baseline.
	      for (n = 0; n < cycle_vis_quantities[k][l][m]->nbaselines; n++) {
		if (cycle_vis_quantities[k][l][m]->baseline[n] ==
		    ants_to_base(vis_lines[j]->ant1, vis_lines[j]->ant2)) {
		  // Found it.
		  n_plot_lines[i][j] += 1;
		  REALLOC(plot_lines[i][j][0], n_plot_lines[i][j]);
		  REALLOC(plot_lines[i][j][1], n_plot_lines[i][j]);
		  plot_lines[i][j][0][n_plot_lines[i][j] - 1] =
		    cycle_vis_quantities[k][l][m]->ut_seconds;
		  if (plot_lines[i][j][0][n_plot_lines[i][j] - 1] < min_x) {
		    min_x = plot_lines[i][j][0][n_plot_lines[i][j] - 1];
		  }
		  if (plot_lines[i][j][0][n_plot_lines[i][j] - 1] > max_x) {
		    max_x = plot_lines[i][j][0][n_plot_lines[i][j] - 1];
		  }
		  // Need to fix for bin.
		  if (plot_controls->panel_type[i] == PLOT_AMPLITUDE) {
		    plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
		      cycle_vis_quantities[k][l][m]->amplitude[n][0];
		  } else if (plot_controls->panel_type[i] == PLOT_PHASE) {
		    plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
		      cycle_vis_quantities[k][l][m]->phase[n][0];
		  }
		  if (plot_lines[i][j][1][n_plot_lines[i][j] - 1] < min_y) {
		    min_y = plot_lines[i][j][1][n_plot_lines[i][j] -1 ];
		  }
		  if (plot_lines[i][j][1][n_plot_lines[i][j] - 1] > max_y) {
		    max_y = plot_lines[i][j][1][n_plot_lines[i][j] - 1];
		  }
		  break;
		}
	      }
	    }
	  }
	}
      }
    }
    // Make the panel.
    changepanel(0, i, panelspec);
    printf("plotting panel %d with %.2f <= x <= %.2f, %.2f <= y <= %.2f\n",
	   i, min_x, max_x, min_y, max_y);
    cpgswin(min_x, max_x, min_y, max_y);
    cpgsci(3);
    cpgsch(1.1);
    if (i % 2 == 0) {
      (void)strcpy(yopts, "BCNTS");
    } else {
      (void)strcpy(yopts, "BCMTS");
    }
    if (i == (plot_controls->num_panels - 1)) {
      (void)strcpy(xopts, "BCNTSZH");
    } else {
      (void)strcpy(xopts, "BCTSZ");
    }
    cpgtbox(xopts, 0, 0, yopts, 0, 0);
    if (plot_controls->panel_type[i] == PLOT_AMPLITUDE) {
      (void)strcpy(panellabel, "Amplitude");
      (void)strcpy(panelunits, "(Pseudo-Jy)");
    } else if (plot_controls->panel_type[i] == PLOT_PHASE) {
      (void)strcpy(panellabel, "Phase");
      (void)strcpy(panelunits, "(degrees)");
    }
    cpgmtxt("L", 2.2, 0.5, 0.5, panellabel);
    cpgmtxt("R", 2.2, 0.5, 0.5, panelunits);
    if (i == (plot_controls->num_panels - 1)) {
      cpgmtxt("B", 3, 0.5, 0.5, "UT");
    } else if (i == 0) {
      dxpos = fracwidth(panelspec, min_x, max_x, 0, i, "Ants:");
      cxpos = 0;
      cpgsci(3);
      cpgmtxt("T", 0.5, cxpos, 0, "Ants:");
      cxpos = dxpos;
      for (j = 1; j <= MAXANTS; j++) {
	if ((1 << j) & plot_controls->array_spec) {
	  (void)sprintf(antstring, "%d", j);
	} else {
	  (void)strcpy(antstring, "-");
	}
	cpgsci(j + 3);
	dxpos = fracwidth(panelspec, min_x, max_x, 0, i, antstring);
	cpgmtxt("T", 0.5, cxpos, 0, antstring);
	cxpos += dxpos;
      }
    }
    cpgsci(1);
    for (j = 0; j < n_vis_lines; j++) {
      cpgsci(vis_lines[j]->pgplot_colour);
      cpgline(n_plot_lines[i][j], plot_lines[i][j][0], plot_lines[i][j][1]);
    }
  }
}

void make_spd_plot(struct ampphase ***cycle_ampphase, struct panelspec *panelspec,
		   struct spd_plotcontrols *plot_controls) {
  int x = 0, y = 0, i, j, ant1, ant2, nants = 0, px, py, iauto = 0, icross = 0;
  int npols = 0, *polidx = NULL, poli, num_ifs = 0, panels_per_if = 0;
  int idxif, ni, ri, rj, rp, bi, bn, pc, inverted = NO;
  float xaxis_min, xaxis_max, yaxis_min, yaxis_max, theight = 0.4;
  float *freq_ordered = NULL, *freq_amp = NULL, *freq_phase = NULL;
  char ptitle[BUFSIZE], ptype[BUFSIZE], ftype[BUFSIZE];
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

  // Select the PGPLOT device.
  cpgslct(plot_controls->pgplot_device);
  
  /* printf("time of cycle = %s %.6f\n", cycle_ampphase[0][0]->obsdate, */
  /* 	 cycle_ampphase[0][0]->ut_seconds); */
  changepanel(-1, -1, panelspec);
  if (plot_controls->interactive == NO) {
    cpgask(0);
  }
  cpgpage();

  for (idxif = 0, ni = 0; idxif < MAXIFS; idxif++) {
    iauto = 0;
    icross = 0;
    npols = 0;
    FREE(polidx);
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
	    bn = 2;
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
	    bn = 1;
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
	  if (ampphase_if[0]->window_name[0] == 'f') {
	    snprintf(ftype, BUFSIZE, "FQ:%s", ampphase_if[0]->window_name + 1);
	  } else if (ampphase_if[0]->window_name[0] == 'z') {
	    snprintf(ftype, BUFSIZE, "ZM:%s", ampphase_if[0]->window_name + 1);
	  }
	  snprintf(ptitle, BUFSIZE, "%s: %s BSL%d%d",
		   ptype, ftype, ant1, ant2);

	  plotpanel_minmax(ampphase_if, plot_controls, i, npols, polidx,
			   &xaxis_min, &xaxis_max, &yaxis_min, &yaxis_max);
	  /* printf("max/max x = %.6f / %.6f, y = %.6f / %.6f\n", */
	  /* 	 xaxis_min, xaxis_max, yaxis_min, yaxis_max); */
	  cpgsci(1);
	  cpgswin(xaxis_min, xaxis_max, yaxis_min, yaxis_max);
	  cpgbox("BCNTS", 0, 0, "BCNTS", 0, 0);
	  cpgmtxt("T", theight, 0.5, 0.5, ptitle);
	  
	  // We loop over the bins we need to plot.
	  if (bn > ampphase_if[0]->nbins[i]) {
	    // We asked for too many bins.
	    bn = ampphase_if[0]->nbins[i];
	  }

	  // Check if we need to make an inverted frequency array.
	  if (plot_controls->plot_options & PLOT_FREQUENCY) {
	    if (ampphase_if[0]->f_frequency[i][0][0] >
		ampphase_if[0]->f_frequency[i][0]
		[ampphase_if[0]->f_nchannels[i][0] - 1]) {
	      // Inverted band.
	      FREE(freq_ordered);
	      FREE(freq_amp);
	      FREE(freq_phase);
	      MALLOC(freq_ordered, ampphase_if[0]->f_nchannels[i][0]);
	      MALLOC(freq_amp, ampphase_if[0]->f_nchannels[i][0]);
	      MALLOC(freq_phase, ampphase_if[0]->f_nchannels[i][0]);
	      inverted = YES;
	    }
	  }

	  pc = 1;
	  for (rp = 0; rp < npols; rp++) {
	    for (bi = 0; bi < bn; bi++) {
	      if (inverted == YES) {
		for (ri = 0, rj = ampphase_if[polidx[rp]]->f_nchannels[i][bi] - 1;
		     ri < ampphase_if[polidx[rp]]->f_nchannels[i][bi];
		     ri++, rj--) {
		  // Swap the frequencies.
		  if (rp == 0) {
		    freq_ordered[ri] =
		      ampphase_if[polidx[rp]]->f_frequency[i][bi][rj];
		  }
		  if (plot_controls->plot_options & PLOT_AMPLITUDE) {
		    freq_amp[ri] =
		      ampphase_if[polidx[rp]]->f_amplitude[i][bi][rj];
		  } else {
		    freq_phase[ri] =
		      ampphase_if[polidx[rp]]->f_phase[i][bi][rj];
		  }
		}
	      } else {
		freq_ordered = ampphase_if[polidx[rp]]->f_frequency[i][bi];
		freq_amp = ampphase_if[polidx[rp]]->f_amplitude[i][bi];
		freq_phase = ampphase_if[polidx[rp]]->f_phase[i][bi];
	      }
	      cpgsci(pc);
	      if (plot_controls->plot_options & PLOT_AMPLITUDE) {
		if (plot_controls->plot_options & PLOT_CHANNEL) {
		  cpgline(ampphase_if[polidx[rp]]->f_nchannels[i][bi],
			  ampphase_if[polidx[rp]]->f_channel[i][bi],
			  ampphase_if[polidx[rp]]->f_amplitude[i][bi]);
		} else if (plot_controls->plot_options & PLOT_FREQUENCY) {
		  cpgline(ampphase_if[polidx[rp]]->f_nchannels[i][bi],
			  freq_ordered, freq_amp);
		}
	      } else if (plot_controls->plot_options & PLOT_PHASE) {
		if (plot_controls->plot_options & PLOT_CHANNEL) {
		  cpgline(ampphase_if[polidx[rp]]->f_nchannels[i][bi],
			  ampphase_if[polidx[rp]]->f_channel[i][bi],
			  ampphase_if[polidx[rp]]->f_phase[i][bi]);
		} else if (plot_controls->plot_options & PLOT_FREQUENCY) {
		  cpgline(ampphase_if[polidx[rp]]->f_nchannels[i][bi],
			  freq_ordered, freq_phase);
		}
	      }
	      pc++;
	    }
	  }

	  if (inverted == YES) {
	    FREE(freq_ordered);
	    FREE(freq_amp);
	    FREE(freq_phase);
	  }
	}
      }
      num_ifs++;
      ni++;
    }
  }
  FREE(polidx);
  
}
