/**
 * ATCA Training Library: common.c
 * (C) Jamie Stevens CSIRO 2019
 *
 * This library contains functions that are useful for many of the
 * applications.
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <regex.h>
#include <readline/readline.h>
#include <readline/history.h>
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

void count_polarisations(struct spd_plotcontrols *plotcontrols) {

  plotcontrols->npols = 0;
  if (plotcontrols->plot_options & PLOT_POL_XX) {
    plotcontrols->npols++;
  }
  if (plotcontrols->plot_options & PLOT_POL_YY) {
    plotcontrols->npols++;
  }
  if (plotcontrols->plot_options & PLOT_POL_XY) {
    plotcontrols->npols++;
  }
  if (plotcontrols->plot_options & PLOT_POL_YX) {
    plotcontrols->npols++;
  }

}

void change_spd_plotcontrols(struct spd_plotcontrols *plotcontrols,
                             int *xaxis_type, int *yaxis_type, int *pols) {
  // Change some values.
  if (xaxis_type != NULL) {
    // There can only be one x-axis type.
    if (plotcontrols->plot_options & PLOT_CHANNEL) {
      plotcontrols->plot_options -= PLOT_CHANNEL;
    } else if (plotcontrols->plot_options & PLOT_FREQUENCY) {
      plotcontrols->plot_options -= PLOT_FREQUENCY;
    }
    plotcontrols->plot_options |= *xaxis_type;
  }

  if (yaxis_type != NULL) {
    if (plotcontrols->plot_options & PLOT_AMPLITUDE) {
      plotcontrols->plot_options -= PLOT_AMPLITUDE;
    }
    if (plotcontrols->plot_options & PLOT_PHASE) {
      plotcontrols->plot_options -= PLOT_PHASE;
    }
    if (plotcontrols->plot_options & PLOT_AMPLITUDE_LINEAR) {
      plotcontrols->plot_options -= PLOT_AMPLITUDE_LINEAR;
    }
    if (plotcontrols->plot_options & PLOT_AMPLITUDE_LOG) {
      plotcontrols->plot_options -= PLOT_AMPLITUDE_LOG;
    }
    if (plotcontrols->plot_options & PLOT_CONSISTENT_YRANGE) {
      plotcontrols->plot_options -= PLOT_CONSISTENT_YRANGE;
    }
    plotcontrols->plot_options |= *yaxis_type;
  }

  if (pols != NULL) {
    if (plotcontrols->plot_options & PLOT_POL_XX) {
      plotcontrols->plot_options -= PLOT_POL_XX;
    }
    if (plotcontrols->plot_options & PLOT_POL_YY) {
      plotcontrols->plot_options -= PLOT_POL_YY;
    }
    if (plotcontrols->plot_options & PLOT_POL_XY) {
      plotcontrols->plot_options -= PLOT_POL_XY;
    }
    if (plotcontrols->plot_options & PLOT_POL_YX) {
      plotcontrols->plot_options -= PLOT_POL_YX;
    }
    plotcontrols->plot_options |= *pols;
  }

}

int change_spd_plotflags(struct spd_plotcontrols *plotcontrols,
                          long int changed_flags, int add_remove) {
  int i, changemade = NO;
  long int available_flags[PLOT_FLAGS_AVAILABLE] =
    { PLOT_FLAG_POL_XX, PLOT_FLAG_POL_YY, PLOT_FLAG_POL_XY, PLOT_FLAG_POL_YX,
      PLOT_FLAG_AUTOCORRELATIONS, PLOT_FLAG_CROSSCORRELATIONS };
                                                    
  // Enable or disable flags.
  for (i = 0; i < PLOT_FLAGS_AVAILABLE; i++) {
    if (changed_flags & available_flags[i]) {
      if ((plotcontrols->plot_flags & available_flags[i]) &&
          (add_remove < 0)) {
        // Remove this flag.
        plotcontrols->plot_flags -= available_flags[i];
        changemade = YES;
      } else if ((!(plotcontrols->plot_flags & available_flags[i])) &&
                 (add_remove > 0)) {
        // Add this flag.
        plotcontrols->plot_flags += available_flags[i];
        changemade = YES;
      }
    }
  }

  return changemade;
}

void init_spd_plotcontrols(struct spd_plotcontrols *plotcontrols,
			   int xaxis_type, int yaxis_type, int pols,
			   int pgplot_device) {
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
  plotcontrols->plot_options = xaxis_type | yaxis_type | pols;

  // By default, only display XX and YY pols, and both auto and cross
  // correlations.
  plotcontrols->plot_flags = PLOT_FLAG_POL_XX | PLOT_FLAG_POL_YY |
    PLOT_FLAG_AUTOCORRELATIONS | PLOT_FLAG_CROSSCORRELATIONS;

  // Work out the number of pols.
  count_polarisations(plotcontrols);
  
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

void change_vis_plotcontrols_visbands(struct vis_plotcontrols *plotcontrols,
                                      int nvisbands, char **visbands) {
  int i;
  if (nvisbands <= 0) {
    return;
  }
  // Free any memory we don't need any more.
  if (nvisbands < plotcontrols->nvisbands) {
    for (i = nvisbands; i < plotcontrols->nvisbands; i++) {
      FREE(plotcontrols->visbands[i]);
    }
  }

  // Allocate new memory if necessary.
  if (nvisbands != plotcontrols->nvisbands) {
    REALLOC(plotcontrols->visbands, nvisbands);
    for (i = plotcontrols->nvisbands; i < nvisbands; i++) {
      CALLOC(plotcontrols->visbands[i], VISBANDLEN);
    }
  }

  // Copy the strings.
  for (i = 0; i < nvisbands; i++) {
    strncpy(plotcontrols->visbands[i], visbands[i], VISBANDLEN);
  }
  plotcontrols->nvisbands = nvisbands;
}
                             

void init_vis_plotcontrols(struct vis_plotcontrols *plotcontrols,
                           int xaxis_type, int paneltypes, int nvisbands, char **visbands,
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
  splitpanels(1, npanels, pgplot_device, 1, 1, 0, panelspec);

  // Initialise the products to display.
  plotcontrols->nproducts = 0;
  plotcontrols->vis_products = NULL;
  plotcontrols->nvisbands = nvisbands;
  MALLOC(plotcontrols->visbands, nvisbands);
  for (i = 0; i < nvisbands; i++) {
    CALLOC(plotcontrols->visbands[i], VISBANDLEN);
    strncpy(plotcontrols->visbands[i], visbands[i], VISBANDLEN);
  }

  // Some default cycle time, but this should always be set (seconds).
  plotcontrols->cycletime = 120;

  // The default history lengths are set to 20 minutes.
  plotcontrols->history_length = 20;
  plotcontrols->history_start = 20;
}

void free_vis_plotcontrols(struct vis_plotcontrols *plotcontrols) {
  int i;
  for (i = 0; i < plotcontrols->nproducts; i++) {
    FREE(plotcontrols->vis_products[i]);
  }
  FREE(plotcontrols->vis_products);
  FREE(plotcontrols->panel_type);
  for (i = 0; i < plotcontrols->nvisbands; i++) {
    FREE(plotcontrols->visbands[i]);
  }
  FREE(plotcontrols->visbands);
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
                 float margin_reduction, int make_info_area,
                 struct panelspec *panelspec) {
  int i, j;
  float panel_width, panel_height, panel_px_width, panel_px_height;
  float padding_fraction = 1.8, padding_x, padding_y;
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
  if (panelspec->measured == NO) {
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
    panelspec->orig_y1 /= 0.7 * margin_reduction;
    panelspec->orig_px_y1 /= 0.7 * margin_reduction;
    if (make_info_area) {
      panelspec->orig_y2 = 1 - 2 * panelspec->orig_y1;
    } else {
      panelspec->orig_y2 = 1 - panelspec->orig_y1;
    }
    panelspec->orig_px_y2 += dpx_y;

    if (make_info_area) {
      // Keep the information area dimensions.
      panelspec->information_x1 = panelspec->orig_x1;
      panelspec->information_x2 = panelspec->orig_x2;
      panelspec->information_y1 = panelspec->orig_y2 + panelspec->orig_y1;
      panelspec->information_y2 = 1.0; //panelspec->orig_y2 + panelspec->orig_y1;
    } else {
      panelspec->information_x1 = 0;
      panelspec->information_x2 = 0;
      panelspec->information_y1 = 0;
      panelspec->information_y2 = 0;
    }
  }
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

  if (panelspec->measured == NO) {
    // Change the character height.
    cpgqch(&orig_charheight);
    charheight = orig_charheight / 2;
    cpgsch(charheight);

    // Don't do this stuff again if the user changes the number of
    // panels.
    panelspec->measured = YES;
  }
  
}

void changepanel(int x, int y, struct panelspec *panelspec) {
  if ((x >= 0) && (x < panelspec->nx) &&
      (y >= 0) && (y < panelspec->ny)) {
    cpgsvp(panelspec->x1[x][y], panelspec->x2[x][y],
           panelspec->y1[x][y], panelspec->y2[x][y]);
  } else if ((x == PANEL_ORIGINAL) && (y == PANEL_ORIGINAL)) {
    // Set it back to original.
    cpgsvp(panelspec->orig_x1, panelspec->orig_x2,
           panelspec->orig_y1, panelspec->orig_y2);
  } else if ((x == PANEL_INFORMATION) && (y == PANEL_INFORMATION)) {
    // Set it to the information panel.
    /* fprintf(stderr, "CHANGEPANEL to X %.4f -> %.4f, Y %.4f -> %.4f\n", */
    /*         panelspec->information_x1, panelspec->information_x2, */
    /*         panelspec->information_y1, panelspec->information_y2); */
    cpgsvp(panelspec->information_x1, panelspec->information_x2,
           panelspec->information_y1, panelspec->information_y2);
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
  if (pol == POL_XX) {
    if (if_num == 1) {
      strcpy(vis_name, "AA");
    } else if (if_num == 2) {
      strcpy(vis_name, "CC");
    }
  } else if (pol == POL_YY) {
    if (if_num == 1) {
      strcpy(vis_name, "BB");
    } else if (if_num == 2) {
      strcpy(vis_name, "DD");
    }
  } else if (pol == POL_XY) {
    if (if_num == 1) {
      strcpy(vis_name, "AB");
    } else if (if_num == 2) {
      strcpy(vis_name, "CD");
    }
  } else if (pol == POL_YX) {
    if (if_num == 1) {
      strcpy(vis_name, "BA");
    } else if (if_num == 2) {
      strcpy(vis_name, "DC");
    }
  }
}

void add_vis_line(struct vis_line ***vis_lines, int *n_vis_lines,
                  int ant1, int ant2, int ifnum, char *if_label, int pol) {
  struct vis_line *new_line = NULL;
  char polname[8];
  // Add a potential vis line.
  *n_vis_lines += 1;
  REALLOC(*vis_lines, *n_vis_lines);
  MALLOC(new_line, 1);
  new_line->ant1 = ant1;
  new_line->ant2 = ant2;
  //new_line->if_number = if_number;
  strncpy(new_line->if_label, if_label, BUFSIZE);
  new_line->pol = pol;
  pol_to_vis_name(new_line->pol, ifnum, polname);
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

int vis_interpret_product(char *product, struct vis_product **vis_product) {
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
    return 0;
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
    return 0;
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
    return 0;
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
    return 0;
  }
}

int find_if_name(struct scan_header_data *scan_header_data,
                 char *name) {
  int res = -1, i, j;
  /* fprintf(stderr, "[find_if_name] searching for band %s\n", name); */
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

  /* fprintf(stderr, "[find_if_name] found band %d %s\n", res, scan_header_data->if_name[res - 1][0]); */
  return res;
}

float fracwidth(struct panelspec *panelspec,
		float axis_min_x, float axis_max_x, int x, int y, char *label) {
  // This routine works out the fractional width of the string in label,
  // compared to the entire width of the box.
  float dx, xc[4], yc[4], dlx;

  // We don't currently need panelspec, but I want to keep it here.
  if (panelspec == NULL) {
    dx = 0;
  }
  x = y = 0;
  dx = fabsf(axis_max_x - axis_min_x);
  cpgqtxt(axis_min_x, 0, x, y, label, xc, yc);
  dlx = fabsf(xc[2] - xc[1]);
  return (dlx / dx);
}

void make_vis_plot(struct vis_quantities ****cycle_vis_quantities,
                   int ncycles, int *cycle_numifs, int npols,
                   struct panelspec *panelspec,
                   struct vis_plotcontrols *plot_controls,
                   struct scan_header_data **header_data) {
  int nants = 0, i = 0, n_vis_lines = 0, j = 0, k = 0, p = 0;
  int singleant = 0, l = 0, m = 0, n = 0, connidx = 0;
  int **n_plot_lines = NULL;
  float ****plot_lines = NULL, min_x, max_x, min_y, max_y;
  float cxpos, dxpos, labtotalwidth, labspacing, dy;
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
  cpgpage();
  
  // We make up to 16 lines per panel.
  // The first dimension will be the panel number.
  MALLOC(plot_lines, plot_controls->num_panels);
  MALLOC(n_plot_lines, plot_controls->num_panels);
  // The second dimension will be the line number.
  // We need to work out how to allocate up to 16 lines.
  for (p = 0, n_vis_lines = 0; p < plot_controls->nproducts; p++) {
    // Check if only a single antenna is requested.
    for (i = 1; i <= MAXANTS; i++) {
      if ((plot_controls->vis_products[p]->antenna_spec == (1<<i)) &&
          (plot_controls->array_spec & (1<<i))) {
        singleant = i;
        break;
      }
    }
    for (i = 1; i <= MAXANTS; i++) {
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
                               i, j, 1, plot_controls->visbands[0], POL_XX);
                }
                if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF2) {
                  add_vis_line(&vis_lines, &n_vis_lines,
                               i, j, 2, plot_controls->visbands[1], POL_XX);
                }
              }
              if (plot_controls->vis_products[p]->pol_spec & PLOT_POL_YY) {
                if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF1) {
                  add_vis_line(&vis_lines, &n_vis_lines,
                               i, j, 1, plot_controls->visbands[0], POL_YY);
                }
                if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF2) {
                  add_vis_line(&vis_lines, &n_vis_lines,
                               i, j, 2, plot_controls->visbands[1], POL_YY);
                }
              }
            } else {
              // Auto-correlations allow XY and YX.
              if (plot_controls->vis_products[p]->pol_spec & PLOT_POL_XY) {
                if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF1) {
                  add_vis_line(&vis_lines, &n_vis_lines,
                               i, j, 1, plot_controls->visbands[0], POL_XY);
                }
                if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF2) {
                  add_vis_line(&vis_lines, &n_vis_lines,
                               i, j, 2, plot_controls->visbands[1], POL_XY);
                }
              }
            }
          }
        }
      }
    }
  }
  //printf("generating %d vis lines\n", n_vis_lines);
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

  // Keep track of the min and max values to plot.
  // We can determine the time range for all panels at once.
  min_x = INFINITY;
  max_x = -INFINITY;

  for (j = 0; j < n_vis_lines; j++) {
    for (k = 0; k < ncycles; k++) {
      for (l = 0; l < cycle_numifs[k]; l++) {
        for (m = 0; m < npols; m++) {
          if ((cycle_vis_quantities[k][l][m]->pol ==
               vis_lines[j]->pol) &&
              (cycle_vis_quantities[k][l][m]->window ==
               find_if_name(header_data[k], vis_lines[j]->if_label))) {
            for (n = 0; n < cycle_vis_quantities[k][l][m]->nbaselines; n++) {
              if (cycle_vis_quantities[k][l][m]->baseline[n] ==
                  ants_to_base(vis_lines[j]->ant1, vis_lines[j]->ant2)) {
                if (cycle_vis_quantities[k][l][m]->flagged_bad[n] > 0) {
                  continue;
                }
                MINASSIGN(min_x, cycle_vis_quantities[k][l][m]->ut_seconds);
                MAXASSIGN(max_x, cycle_vis_quantities[k][l][m]->ut_seconds);
                break;
              }
            }
            break;
          }
        }
      }
    }
  }
  // Adjust the time constraints based on the history specification.
  MAXASSIGN(min_x, (max_x - plot_controls->history_start * 60));
  /* min_x = (min_x < (max_x - plot_controls->history_start * 60)) ? */
  /*   (max_x - plot_controls->history_start * 60) : min_x; */
  MINASSIGN(max_x, (min_x + plot_controls->history_length * 60));
  /* max_x = (max_x > (min_x + plot_controls->history_length * 60)) ? */
  /*   (min_x + plot_controls->history_length * 60) : max_x; */
  
  // Always keep another 5% on the right side.
  max_x += (max_x - min_x) * 0.05;
  
  for (i = 0; i < plot_controls->num_panels; i++) {
    // Make the lines.
    MALLOC(plot_lines[i], n_vis_lines);
    MALLOC(n_plot_lines[i], n_vis_lines);

    min_y = INFINITY;
    max_y = -INFINITY;

    // Now go through the data and make our arrays.
    // The third dimension of the plot lines will be the X and Y
    // coordinates, then the cycle time in seconds.
    for (j = 0; j < n_vis_lines; j++) {
      n_plot_lines[i][j] = 0;
      MALLOC(plot_lines[i][j], 3);
      plot_lines[i][j][0] = NULL;
      plot_lines[i][j][1] = NULL;
      plot_lines[i][j][2] = NULL;
      // The fourth dimension is the points to plot in the line.
      // We accumulate these now.
      for (k = 0; k < ncycles; k++) {
        /* printf(" cycle %d has %d IFs, type %s\n", k, cycle_numifs[k], */
        /*        cycle_vis_quantities[k][0][0]->scantype); */
        for (l = 0; l < cycle_numifs[k]; l++) {
          for (m = 0; m < npols; m++) {
            // Exclude data outside our history range.
            if ((cycle_vis_quantities[k][l][m]->ut_seconds < min_x) ||
                (cycle_vis_quantities[k][l][m]->ut_seconds > max_x)) {
              break;
            }
            /* printf("comparing pols %d to %d, window %d\n", */
            /*        cycle_vis_quantities[k][l][m]->pol, vis_lines[j]->pol, */
            /*        cycle_vis_quantities[k][l][m]->window); */
            if ((cycle_vis_quantities[k][l][m]->pol ==
                 vis_lines[j]->pol) &&
                (cycle_vis_quantities[k][l][m]->window ==
                 find_if_name(header_data[k], vis_lines[j]->if_label))) {
              /* printf("   found appropriate vis quantity!\n"); */
              // Find the correct baseline.
              for (n = 0; n < cycle_vis_quantities[k][l][m]->nbaselines; n++) {
                if (cycle_vis_quantities[k][l][m]->baseline[n] ==
                    ants_to_base(vis_lines[j]->ant1, vis_lines[j]->ant2)) {
                  /* printf("  IF %d, pol %d, baseline %d, flagged = %d\n", */
                  /* 	 l, m, n, cycle_vis_quantities[k][l][m]->flagged_bad[n]); */
                  if (cycle_vis_quantities[k][l][m]->flagged_bad[n] > 0) {
                    continue;
                  }
                  // Found it.
                  n_plot_lines[i][j] += 1;
                  REALLOC(plot_lines[i][j][0], n_plot_lines[i][j]);
                  REALLOC(plot_lines[i][j][1], n_plot_lines[i][j]);
                  REALLOC(plot_lines[i][j][2], n_plot_lines[i][j]);
                  plot_lines[i][j][0][n_plot_lines[i][j] - 1] =
                    cycle_vis_quantities[k][l][m]->ut_seconds;
                  if (header_data != NULL) {
                    // Get the cycle time from this cycle.
                    plot_lines[i][j][2][n_plot_lines[i][j] - 1] =
                      header_data[k]->cycle_time;
                  } else {
                    // Get the cycle time from the plot options.
                    plot_lines[i][j][2][n_plot_lines[i][j] - 1] =
                      plot_controls->cycletime;
                  }
                  /* printf("  number of points is now %d, time %.3f\n", */
                  /* 	 n_plot_lines[i][j], */
                  /* 	 plot_lines[i][j][0][n_plot_lines[i][j] - 1]); */
                  // Need to fix for bin.
                  if (plot_controls->panel_type[i] == PLOT_AMPLITUDE) {
                    plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                      cycle_vis_quantities[k][l][m]->amplitude[n][0];
                  } else if (plot_controls->panel_type[i] == PLOT_PHASE) {
                    plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                      cycle_vis_quantities[k][l][m]->phase[n][0];
                  } else if (plot_controls->panel_type[i] == PLOT_DELAY) {
                    plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                      cycle_vis_quantities[k][l][m]->delay[n][0];
                  }
                  MINASSIGN(min_y, plot_lines[i][j][1][n_plot_lines[i][j] - 1]);
                  MAXASSIGN(max_y, plot_lines[i][j][1][n_plot_lines[i][j] - 1]);
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
    
    /* printf("plotting panel %d with %.2f <= x <= %.2f, %.2f <= y <= %.2f\n", */
    /* 	   i, min_x, max_x, min_y, max_y); */
    // Extend the y-range slightly.
    dy = 0.05 * (max_y - min_y);
    min_y -= dy;
    max_y += dy;
    if ((plot_controls->panel_type[i] == PLOT_AMPLITUDE) && (min_y < 0)) {
      // Amplitude cannot be below 0.
      min_y = 0;
    }
    
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
    } else if (plot_controls->panel_type[i] == PLOT_DELAY) {
      (void)strcpy(panellabel, "Delay");
      (void)strcpy(panelunits, "(ns)");
    }
    cpgmtxt("L", 2.2, 0.5, 0.5, panellabel);
    cpgmtxt("R", 2.2, 0.5, 0.5, panelunits);
    if (i == (plot_controls->num_panels - 1)) {
      cpgmtxt("B", 3, 0.5, 0.5, "UT");
      // Print the baselines on the bottom.
      labtotalwidth = 0;
      for (j = 0; j < n_vis_lines; j++) {
        labtotalwidth += fracwidth(panelspec, min_x, max_x, 0, 1,
                                   vis_lines[j]->label);
      }
      // Work out the extra spacing to make the labels go all
      // the way across the bottom.
      labspacing = (1 - labtotalwidth) /
        (float)(n_vis_lines - 1);
      cxpos = 0;
      for (j = 0; j < n_vis_lines; j++) {
        cpgsci(vis_lines[j]->pgplot_colour);
        dxpos = fracwidth(panelspec, min_x, max_x, 0, i,
                          vis_lines[j]->label);
        cpgmtxt("B", 4, cxpos, 0, vis_lines[j]->label);
        //printf("printing label %s\n", vis_lines[j]->label);
        cxpos += dxpos + labspacing;
      }
    } else if (i == 0) {
      // Print the array antennas at the top left.
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
      // Then print the frequencies of the bands at the top right.
      
    }
    cpgsci(1);
    for (j = 0; j < n_vis_lines; j++) {
      cpgsci(vis_lines[j]->pgplot_colour);
      // Plot the line in segments connected in time.
      for (k = 0, connidx = 0; k < (n_plot_lines[i][j] - 1); k++) {
        if ((plot_lines[i][j][0][k + 1] >
             (plot_lines[i][j][0][k] + (1.5 * plot_lines[i][j][2][k])))) {
          // Disconnect.
          cpgline((k - connidx), plot_lines[i][j][0] + connidx,
                  plot_lines[i][j][1] + connidx);
          connidx = (k + 1);
        }
      }
      cpgline((n_plot_lines[i][j] - connidx),
              plot_lines[i][j][0] + connidx, plot_lines[i][j][1] + connidx);
    }
  }
  
  // Free our memory.
  for (i = 0; i < n_vis_lines; i++) {
    FREE(vis_lines[i]);
  }
  FREE(vis_lines);
  for (i = 0; i < plot_controls->num_panels; i++) {
    for (j = 0; j < n_vis_lines; j++) {
      for (k = 0; k < 3; k++) {
        FREE(plot_lines[i][j][k]);
      }
      FREE(plot_lines[i][j]);
    }
    FREE(n_plot_lines[i]);
    FREE(plot_lines[i]);
  }
  FREE(n_plot_lines);
  FREE(plot_lines);
}

// Take the dB log of value v, compared with max value m, store
// the log value in s.
#define LOGAMP(v, m, s)                               \
  if (v < (m / 1.0E12)) {                             \
    s = -120.0;                                       \
  } else {                                            \
    s = (float)(10.0 * log10((double)v / (double)m)); \
  }

void make_spd_plot(struct ampphase ***cycle_ampphase, struct panelspec *panelspec,
                   struct spd_plotcontrols *plot_controls, bool all_data_present) {
  int i, ant1, ant2, nants = 0, px, py, iauto = 0, icross = 0, isauto = NO;
  int npols = 0, *polidx = NULL, poli, num_ifs = 0, panels_per_if = 0;
  int idxif, ni, ri, rj, rp, bi, bn, pc, inverted = NO, plot_started = NO;
  float **panel_plotted = NULL, information_x_pos = 0.01, information_text_width;
  float information_text_height, xaxis_min, xaxis_max, yaxis_min, yaxis_max, theight;
  float ylog_min, ylog_max, pollab_height, pollab_xlen, pollab_ylen, pollab_padding;
  float *plot_xvalues = NULL, *plot_yvalues = NULL;
  char ptitle[BIGBUFSIZE], ptype[BUFSIZE], ftype[BUFSIZE], poltitle[BUFSIZE];
  char information_text[BUFSIZE];
  struct ampphase **ampphase_if = NULL;

  // Definitions of some magic numbers that we use.
  // The height above the top axis for the plot title.
  theight = 0.4;
  // The height below the bottom axis for the polarisation labels.
  pollab_height = 2.2;
  // The padding fraction width for the polarisation labels.
  pollab_padding = 1.2;
  
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
  if (plot_controls->plot_flags & PLOT_FLAG_AUTOCORRELATIONS) {
    panels_per_if += nants;
  }
  if (plot_controls->plot_flags & PLOT_FLAG_CROSSCORRELATIONS) {
    panels_per_if += (nants * (nants - 1)) / 2;
  }

  // Select the PGPLOT device.
  cpgslct(plot_controls->pgplot_device);

  // We need to keep track of how many plots have been made on each
  // panel, for labelling.
  MALLOC(panel_plotted, panelspec->nx);
  for (i = 0; i < panelspec->nx; i++) {
    CALLOC(panel_plotted[i], panelspec->ny);
  }
  
  /* printf("time of cycle = %s %.6f\n", cycle_ampphase[0][0]->obsdate, */
  /* 	 cycle_ampphase[0][0]->ut_seconds); */
  //changepanel(-1, -1, panelspec);

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
              (plot_controls->plot_flags & PLOT_FLAG_AUTOCORRELATIONS)) {
            px = (num_ifs * panels_per_if + iauto) % panelspec->nx;
            py = (int)((num_ifs * panels_per_if + iauto - px) / panelspec->nx);
            iauto++;
            bn = 2;
            isauto = YES;
          } else if ((ant1 != ant2) &&
                     (plot_controls->plot_flags & PLOT_FLAG_CROSSCORRELATIONS)) {
            if (plot_controls->plot_flags & PLOT_FLAG_AUTOCORRELATIONS) {
              px = (num_ifs * panels_per_if + nants + icross) % panelspec->nx;
              py = (int)((num_ifs * panels_per_if + nants + icross - px) /
                         panelspec->nx);
            } else {
              px = (num_ifs * panels_per_if + icross) % panelspec->nx;
              py = (int)((num_ifs * panels_per_if + icross - px) / panelspec->nx);
            }
            icross++;
            bn = 1;
            isauto = NO;
          } else {
            // We're not plotting this product.
            continue;
          }
          // Check if we've exceeded the space for this plot.
          if (py >= panelspec->ny) {
            continue;
          }
          // Set the panel.
          if (plot_started == NO) {
            if (plot_controls->interactive == NO) {
              cpgask(0);
            }
            cpgpage();
            plot_started = YES;
            // Put some information in the information area.
            changepanel(PANEL_INFORMATION, PANEL_INFORMATION, panelspec);
            cpgswin(0, 1, 0, 1);
            cpgsci(1);
            cpgbox("BC", 0, 0, "BC", 0, 0);
            cpgptxt(information_x_pos, 0.5, 0, 0, cycle_ampphase[0][0]->obsdate);
            cpglen(4, cycle_ampphase[0][0]->obsdate, &information_text_width,
                   &information_text_height);
            information_x_pos += information_text_width + 0.02;
            seconds_to_hourlabel(cycle_ampphase[0][0]->ut_seconds,
                                 information_text);
            cpgptxt(information_x_pos, 0.5, 0, 0, information_text);
            cpglen(4, information_text, &information_text_width, &information_text_height);
            information_x_pos += information_text_width + 0.02;
          }
          changepanel(px, py, panelspec);
          // Set the title for the plot.
          if (plot_controls->plot_options & PLOT_AMPLITUDE) {
            if (plot_controls->plot_options & PLOT_AMPLITUDE_LOG) {
              snprintf(ptype, BUFSIZE, "LOG(dB) AMPL.");
            } else {
              snprintf(ptype, BUFSIZE, "AMPL.");
            }
          } else if (plot_controls->plot_options & PLOT_PHASE) {
            snprintf(ptype, BUFSIZE, "PHASE");
          }
          if (ampphase_if[0]->window_name[0] == 'f') {
            snprintf(ftype, BUFSIZE, "FQ:%s", ampphase_if[0]->window_name + 1);
          } else if (ampphase_if[0]->window_name[0] == 'z') {
            snprintf(ftype, BUFSIZE, "ZM:%s", ampphase_if[0]->window_name + 1);
          }
          snprintf(ptitle, BIGBUFSIZE, "%s: %s BSL%d%d",
                   ptype, ftype, ant1, ant2);
          
          plotpanel_minmax(ampphase_if, plot_controls, i, npols, polidx,
                           &xaxis_min, &xaxis_max, &yaxis_min, &yaxis_max);
          /* printf("max/max x = %.6f / %.6f, y = %.6f / %.6f\n", */
          /* 	 xaxis_min, xaxis_max, yaxis_min, yaxis_max); */

          if ((plot_controls->plot_options & PLOT_AMPLITUDE) &&
              (plot_controls->plot_options & PLOT_AMPLITUDE_LOG)) {
            ylog_min = yaxis_min;
            ylog_max = yaxis_max;
            yaxis_max = 1.0;
            LOGAMP(ylog_min, ylog_max, yaxis_min);
          }

          if (panel_plotted[px][py] == 0) {
            // Only plot the borders and title once.
            cpgsci(1);
            cpgswin(xaxis_min, xaxis_max, yaxis_min, yaxis_max);
            cpgbox("BCNTS1", 0, 0, "BCNTS", 0, 0);
            cpgmtxt("T", theight, 0.5, 0.5, ptitle);
          }
          
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
              inverted = YES;
            }
          }

          // Allocate the output arrays.
          MALLOC(plot_xvalues, ampphase_if[0]->f_nchannels[i][0]);
          MALLOC(plot_yvalues, ampphase_if[0]->f_nchannels[i][0]);
          
          pc = 1;
          for (rp = 0; rp < npols; rp++) {
            for (bi = 0; bi < bn; bi++) {
              // Check if we actually proceed for this polarisation and bin
              // combination.
              if ((isauto == YES) && (bi > 0) &&
                  ((ampphase_if[polidx[rp]]->pol == POL_XY) ||
                   (ampphase_if[polidx[rp]]->pol == POL_YX))) {
                // Don't plot the second bins for the cross-pols.
                continue;
              }
              if ((isauto == YES) && (ampphase_if[polidx[rp]]->pol == POL_YX) &&
                  (!(plot_controls->plot_flags & PLOT_FLAG_POL_YX))) {
                // Don't plot the YX bin in the autos without explicit instruction.
                // We include only this one here since we do allow XX, YY, XY plotting in the
                // autos without instruction.
                continue;
              }
              if ((isauto == NO) && (((ampphase_if[polidx[rp]]->pol == POL_XY) &&
                                      (!(plot_controls->plot_flags & PLOT_FLAG_POL_XY))) ||
                                     ((ampphase_if[polidx[rp]]->pol == POL_YX) &&
                                      (!(plot_controls->plot_flags & PLOT_FLAG_POL_YX))))) {
                // Don't show the cross-pols in the cross-correlations without
                // explicit instruction.
                continue;
              }
              for (ri = 0, rj = ampphase_if[polidx[rp]]->f_nchannels[i][bi] - 1;
                   ri < ampphase_if[polidx[rp]]->f_nchannels[i][bi];
                   ri++, rj--) {
                if (inverted == YES) {
                  // Swap the frequencies.
                  if (rp == 0) {
                    plot_xvalues[ri] =
                      ampphase_if[polidx[rp]]->f_frequency[i][bi][rj];
                  }
                  if (plot_controls->plot_options & PLOT_AMPLITUDE) {
                    if (plot_controls->plot_options & PLOT_AMPLITUDE_LOG) {
                      LOGAMP(ampphase_if[polidx[rp]]->f_amplitude[i][bi][rj], ylog_max,
                             plot_yvalues[ri]);
                    } else {
                      plot_yvalues[ri] = 
                        ampphase_if[polidx[rp]]->f_amplitude[i][bi][rj];
                    }
                  } else {
                    plot_yvalues[ri] =
                      ampphase_if[polidx[rp]]->f_phase[i][bi][rj];
                  }
                } else {
                  if (plot_controls->plot_options & PLOT_FREQUENCY) {
                    plot_xvalues[ri] = ampphase_if[polidx[rp]]->f_frequency[i][bi][ri];
                  } else if (plot_controls->plot_options & PLOT_CHANNEL) {
                    plot_xvalues[ri] = ampphase_if[polidx[rp]]->f_channel[i][bi][ri];
                  }
                  if (plot_controls->plot_options & PLOT_AMPLITUDE) {
                    if (plot_controls->plot_options & PLOT_AMPLITUDE_LOG) {
                      LOGAMP(ampphase_if[polidx[rp]]->f_amplitude[i][bi][ri], ylog_max,
                             plot_yvalues[ri]);
                    } else {
                      plot_yvalues[ri] = ampphase_if[polidx[rp]]->f_amplitude[i][bi][ri];
                    }
                  } else if (plot_controls->plot_options & PLOT_PHASE) {
                    plot_yvalues[ri] = ampphase_if[polidx[rp]]->f_phase[i][bi][ri];
                  }
                }
              }
              cpgsci(pc);
              cpgline(ampphase_if[polidx[rp]]->f_nchannels[i][bi],
                      plot_xvalues, plot_yvalues);
              // Add the polarisation label.
              switch (ampphase_if[polidx[rp]]->pol) {
              case POL_XX:
                strcpy(poltitle, "AA");
                if ((isauto) && (bi > 0)) {
                  strcpy(poltitle, "aa");
                }
                break;
              case POL_YY:
                strcpy(poltitle, "BB");
                if ((isauto) && (bi > 0)) {
                  strcpy(poltitle, "bb");
                }
                break;
              case POL_XY:
                strcpy(poltitle, "AB");
                break;
              case POL_YX:
                strcpy(poltitle, "BA");
                break;
              }
              //cpgmtxt("B", pollab_height, (panel_plotted[px][py] * pollab_width), 0.0, poltitle);
              cpgmtxt("B", pollab_height, panel_plotted[px][py], 0.0, poltitle);
              cpglen(5, poltitle, &pollab_xlen, &pollab_ylen);
              panel_plotted[px][py] += pollab_xlen * pollab_padding;
              
              pc++;
            }
          }
          
          FREE(plot_xvalues);
          FREE(plot_yvalues);
        }
      }
      num_ifs++;
      ni++;
    } else if (all_data_present == true) {
      ni++;
    }
  }
  FREE(polidx);
  for (i = 0; i < panelspec->nx; i++) {
    FREE(panel_plotted[i]);
  }
  FREE(panel_plotted);
  
}

void seconds_to_hourlabel(float seconds, char *hourlabel) {
  // Take the number of seconds and give it back as
  // HH:MM:SS (assuming there are 86400 seconds in a day).
  // If greater than 86400, add "xd" at the start.
  int d, h, m, s;
  float dayseconds;
  
  dayseconds = (int)seconds % 86400;
  d = (int)((seconds - dayseconds) / 86400);

  h = (int)floorf(dayseconds / 3600);
  seconds -= h * 3600;
  m = (int)floorf(seconds / 60);
  seconds -= m * 60;
  s = (int)seconds;

  if (d > 0) {
    (void)sprintf(hourlabel, "%dd %02d:%02d:%02d", d, h, m, s);
  } else {
    (void)sprintf(hourlabel, "%02d:%02d:%02d", h, m, s);
  }
}

bool minmatch(char *ref, char *chk, int minlength) {
  // This routine compares the string chk to the string
  // ref. If the entire string chk represents ref up the
  // the length of chk or ref, it returns true.
  // eg. if ref = "select", and chk = "sel", true
  //        ref = "select", and chk = "s", minlength = 3, false
  //        ref = "select", and chk = "selects", false
  int chklen, reflen;

  reflen = strlen(ref);
  chklen = strlen(chk);
  if ((minlength > chklen) || (minlength > reflen)) {
    return false;
  }

  if (chklen > reflen) {
    return false;
  }

  if (strncasecmp(chk, ref, chklen) == 0) {
    return true;
  }

  return false;
}

int split_string(char *s, char *delim, char ***elements) {
  int i = 0;
  char *token = NULL, *saveptr = NULL;

  // Skip any leading delimiters.
  while (*s == *delim) {
    s++;
  }
  while ((token = strtok_r(s, delim, &saveptr))) {
    s = NULL;
    REALLOC(*elements, (i + 1));
    (*elements)[i] = token;
    i++;
  }

  return i;
}

void minutes_representation(float minutes, char *representation) {
  // We look at the number of minutes, and output the best human representation
  // of that duration.
  if (minutes < 1) {
    // Output in seconds.
    sprintf(representation, "%.1f sec", (minutes * 60));
  } else if (minutes > 2800) {
    // Output in days.
    sprintf(representation, "%.2f days", (minutes / 1440));
  } else if (minutes > 120) {
    // Output in hours.
    sprintf(representation, "%.2f hours", (minutes / 60));
  } else {
    // Output in minutes.
    sprintf(representation, "%.1f min", minutes);
  }
}

float string_to_minutes(char *s) {
  // Parse a string like 2.1m or 1.3h or 30s or 1h30m into the
  // number of minutes that string represents.
  float nmin = 0, num;
  regex_t regex;
  regmatch_t matches[4];
  int reti;
  char *cursor, numstr[100], *eptr = NULL;

  reti = regcomp(&regex, "([0-9]+)([dhms])", REG_EXTENDED);

  if (reti) {
    fprintf(stderr, "[string_to_minutes] Unable to compile regex\n");
    return nmin;
  }
  cursor = s;
  while (!(regexec(&regex, cursor, 4, matches, 0))) {
    strncpy(numstr, cursor + matches[1].rm_so,
            (1 + matches[1].rm_eo - matches[1].rm_so));
    num = strtof(numstr, &eptr);
    if (eptr != numstr) {
      // We got a conversion.
      switch ((cursor + matches[2].rm_so)[0]) {
      case 'd':
        nmin += num * 1440;
        break;
      case 'h':
        nmin += num * 60;
        break;
      case 'm':
        nmin += num;
        break;
      case 's':
        nmin += num / 60;
        break;
      }
    }
    
    cursor += matches[0].rm_eo;
  }

  regfree(&regex);
  
  return nmin;
}

void readline_print_messages(int nmesg, char **mesg) {
  // This routine allows us to print a bunch of messages while using
  // a readline interface, and have the prompt reappear after the
  // output.
  int i, saved_point;
  char *saved_line = NULL;

  saved_point = rl_point;
  saved_line = rl_copy_text(0, rl_end);
  rl_save_prompt();
  rl_replace_line("", 0);
  rl_redisplay();

  for (i = 0; i < nmesg; i++) {
    printf("%s", mesg[i]);
  }

  rl_restore_prompt();
  rl_replace_line(saved_line, 0);
  rl_point = saved_point;
  rl_redisplay();

  FREE(saved_line);
}
