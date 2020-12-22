/** \file plotting.c
 *  \brief Routines used to make plots, and supporting routines
 *
 * ATCA Training Library
 * (C) Jamie Stevens CSIRO 2020
 *
 * This module contains functions that need to plot things with
 * PGPLOT.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "memory.h"
#include "cpgplot.h"
#include "plotting.h"
#include "compute.h"

/*!
 *  \brief Work out how many polarisations are needed for a plot
 *  \param plotcontrols a structure set by the user to control how a plot should
 *                      look and what information it should contain
 *
 * This routine is necessary since the `plot_options` variable in the
 * spd_plotcontrols structure is a bitwise OR combination, so it not super
 * easy to work out how many options have been included. This routine doesn't
 * have a return value because it sets a value within the structure it is
 * called with.
 */
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

/*!
 *  \brief Sorting routine for vis_line structures by increasing baseline length
 *  \param a a vis_line structure
 *  \param b another vis_line structure
 *  \return - -1 if the baseline length of a < b
 *          - 1 if the baseline length of a > b
 *          - 0 if the two baseline lengths are identical
 */
int cmpfunc_baseline_length(const void *a, const void *b) {
  // This routine is responsible for sorting the vis_line structures
  // in increasing baseline length.
  double ba, bb;
  ba = (*(struct vis_line**)a)->baseline_length;
  bb = (*(struct vis_line**)b)->baseline_length;
  if (ba < bb) return -1;
  if (ba > bb) return 1;
  return 0;
}

/*!
 *  \brief Transform routine which takes some line colour and returns a colour
 *         that should be used to overplot the averaged version of that line
 *  \param plot_colour the PGPLOT colour index used to plot the normal line
 *  \return the PGPLOT colour index which should be used to overplot the
 *          averaged data
 */
int plot_colour_averaging(int plot_colour) {
  return (plot_colour + 5);
}

void change_spd_plotcontrols(struct spd_plotcontrols *plotcontrols,
                             int *xaxis_type, int *yaxis_type, int *pols,
			     int *decorations) {
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
    if (plotcontrols->plot_options & PLOT_REAL) {
      plotcontrols->plot_options -= PLOT_REAL;
    }
    if (plotcontrols->plot_options & PLOT_IMAG) {
      plotcontrols->plot_options -= PLOT_IMAG;
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

  if (decorations != NULL) {
    if (plotcontrols->plot_options & PLOT_TVCHANNELS) {
      plotcontrols->plot_options -= PLOT_TVCHANNELS;
    }
    plotcontrols->plot_options |= *decorations;
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
			   int decorations, int pgplot_device) {
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

  // Set our decorations.
  if (decorations == DEFAULT) {
    decorations = PLOT_TVCHANNELS;
  }
  plotcontrols->plot_options |= decorations;
  
  // Keep the PGPLOT device.
  plotcontrols->pgplot_device = pgplot_device;
}

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

void change_vis_plotcontrols_limits(struct vis_plotcontrols *plotcontrols,
                                    int paneltype, bool use_limit,
                                    float limit_min, float limit_max) {
  int i;
  // First, find the panel matching paneltype. If paneltype is -1, change
  // all the panels.
  for (i = 0; i < plotcontrols->num_panels; i++) {
    if ((paneltype == VIS_PLOTPANEL_ALL) ||
        (plotcontrols->panel_type[i] == paneltype)) {
      plotcontrols->use_panel_limits[i] = use_limit;
      if (use_limit) {
        // Change the limit settings.
        plotcontrols->panel_limits_min[i] = limit_min;
        plotcontrols->panel_limits_max[i] = limit_max;
      }
    }
  }
}

#define NAVAILABLE_PANELS 16
const int available_panels[NAVAILABLE_PANELS] = { VIS_PLOTPANEL_AMPLITUDE,
                                                  VIS_PLOTPANEL_PHASE,
                                                  VIS_PLOTPANEL_DELAY,
                                                  VIS_PLOTPANEL_TEMPERATURE,
                                                  VIS_PLOTPANEL_PRESSURE,
                                                  VIS_PLOTPANEL_HUMIDITY,
                                                  VIS_PLOTPANEL_SYSTEMP,
                                                  VIS_PLOTPANEL_WINDSPEED,
                                                  VIS_PLOTPANEL_WINDDIR,
                                                  VIS_PLOTPANEL_RAINGAUGE,
                                                  VIS_PLOTPANEL_SEEMONPHASE,
                                                  VIS_PLOTPANEL_SEEMONRMS,
                                                  VIS_PLOTPANEL_SYSTEMP_COMPUTED,
                                                  VIS_PLOTPANEL_GTP,
                                                  VIS_PLOTPANEL_SDO,
                                                  VIS_PLOTPANEL_CALJY };

bool product_can_be_x(int product) {
  switch (product) {
  case PLOT_TIME:
    return true;
  }
  return false;
}


void change_vis_plotcontrols_panels(struct vis_plotcontrols *plotcontrols,
                                    int xaxis_type, int num_panels,
                                    int *paneltypes, struct panelspec *panelspec) {
  int cnpanels = 0, i, j;
  bool *climits = NULL;
  float *climits_min = NULL, *climits_max = NULL;
  
  // We change which panels are displayed, and the axis types.
  if (xaxis_type == DEFAULT) {
    xaxis_type = PLOT_TIME;
  }
  // Count the number of panels we need.
  for (i = 0; i < num_panels; i++) {
    for (j = 0; j < NAVAILABLE_PANELS; j++) {
      if (paneltypes[i] == available_panels[j]) {
        cnpanels += 1;
        break;
      }
    }
  }
  // Copy it to the actual structure if we get complete success.
  if ((cnpanels == num_panels) && (product_can_be_x(xaxis_type))) {
    // We don't want to reset any currently set axis limits, so it
    // gets complicated.
    CALLOC(climits, num_panels);
    CALLOC(climits_min, num_panels);
    CALLOC(climits_max, num_panels);
    for (i = 0; i < num_panels; i++) {
      for (j = 0; j < plotcontrols->num_panels; j++) {
        if (paneltypes[i] == plotcontrols->panel_type[j]) {
          climits[i] = plotcontrols->use_panel_limits[j];
          climits_min[i] = plotcontrols->panel_limits_min[j];
          climits_max[i] = plotcontrols->panel_limits_max[j];
          break;
        }
      }
    }
    // Make the changes now.
    if (plotcontrols->num_panels != num_panels) {
      // Change the PGPLOT splitting.
      splitpanels(1, num_panels, plotcontrols->pgplot_device, 1, 1, 0, panelspec);
    }
    plotcontrols->x_axis_type = xaxis_type;
    plotcontrols->num_panels = num_panels;
    REALLOC(plotcontrols->panel_type, plotcontrols->num_panels);
    REALLOC(plotcontrols->use_panel_limits, plotcontrols->num_panels);
    REALLOC(plotcontrols->panel_limits_min, plotcontrols->num_panels);
    REALLOC(plotcontrols->panel_limits_max, plotcontrols->num_panels);
    for (i = 0; i < num_panels; i++) {
      plotcontrols->panel_type[i] = paneltypes[i];
      plotcontrols->use_panel_limits[i] = climits[i];
      plotcontrols->panel_limits_min[i] = climits_min[i];
      plotcontrols->panel_limits_max[i] = climits_max[i];
    }
    // Free our memory.
    FREE(climits);
    FREE(climits_min);
    FREE(climits_max);
  }
  
}

void init_vis_plotcontrols(struct vis_plotcontrols *plotcontrols,
                           int xaxis_type, int num_panels,
                           int *paneltypes, int nvisbands, char **visbands,
                           int pgplot_device, struct panelspec *panelspec) {
  int i, j;
  
  // Initialise the plotcontrols structure and form the panelspec
  // structure using the same information.
  if (xaxis_type == DEFAULT) {
    xaxis_type = PLOT_TIME;
  }

  if (!product_can_be_x(xaxis_type)) {
    // We can't continue;
    return;
  }
  plotcontrols->plot_options = 0;
  plotcontrols->x_axis_type = xaxis_type;

  // Count the number of panels we need.
  plotcontrols->num_panels = 0;
  plotcontrols->panel_type = NULL;
  for (i = 0; i < num_panels; i++) {
    for (j = 0; j < NAVAILABLE_PANELS; j++) {
      if (paneltypes[i] == available_panels[j]) {
        plotcontrols->num_panels += 1;
        REALLOC(plotcontrols->panel_type, plotcontrols->num_panels);
        plotcontrols->panel_type[plotcontrols->num_panels - 1] = paneltypes[i];
        break;
      }
    }
  }

  // Initialise the axis limits.
  MALLOC(plotcontrols->use_panel_limits, plotcontrols->num_panels);
  MALLOC(plotcontrols->panel_limits_min, plotcontrols->num_panels);
  MALLOC(plotcontrols->panel_limits_max, plotcontrols->num_panels);
  for (i = 0; i < plotcontrols->num_panels; i++) {
    plotcontrols->use_panel_limits[i] = false;
  }

  // Keep the PGPLOT device.
  plotcontrols->pgplot_device = pgplot_device;
  
  // Set up the panelspec structure for this.
  splitpanels(1, plotcontrols->num_panels, pgplot_device, 1, 1, 0, panelspec);

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
  FREE(plotcontrols->use_panel_limits);
  FREE(plotcontrols->panel_limits_min);
  FREE(plotcontrols->panel_limits_max);
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
                 float margin_reduction, int info_area_num_lines,
                 struct panelspec *panelspec) {
  int i, j;
  float panel_width, panel_height, panel_px_width, panel_px_height;
  float padding_fraction = 1.8, padding_x, padding_y;
  float padding_px_x, padding_px_y, dpx_x, dpx_y;
  float orig_charheight, charheight, meas_charwidth, meas_charheight;
  
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

    // Change the character height.
    cpgqch(&orig_charheight);
    charheight = orig_charheight / 2;
    cpgsch(charheight);

    // Don't do this stuff again if the user changes the number of
    // panels.
    panelspec->measured = YES;
    
    // Reduce the margins.
    panelspec->orig_x1 /= margin_reduction;
    dpx_x = panelspec->orig_px_x1 - (panelspec->orig_px_x1 / margin_reduction);
    dpx_y = panelspec->orig_px_y1 - (panelspec->orig_px_y1 / margin_reduction);
    panelspec->orig_px_x1 /= margin_reduction;
    panelspec->orig_x2 = 1 - panelspec->orig_x1;
    panelspec->orig_px_x2 += dpx_x;
    panelspec->orig_y1 /= 0.7 * margin_reduction;
    panelspec->orig_px_y1 /= 0.7 * margin_reduction;
    panelspec->num_information_lines = (info_area_num_lines - 1);
    if (info_area_num_lines > 0) {
      cpglen(0, "L", &meas_charwidth, &meas_charheight);
      panelspec->orig_y2 = 1 - 2 * panelspec->orig_y1 -
        (float)panelspec->num_information_lines * meas_charheight;
    } else {
      panelspec->orig_y2 = 1 - panelspec->orig_y1;
    }
    panelspec->orig_px_y2 += dpx_y;

    if (info_area_num_lines > 0) {
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
  } else if ((x >= 0) && (x < panelspec->nx) && (y < 0)) {
    // Set the viewport to be all the space above the top y panel.
    cpgsvp(panelspec->x1[x][0], panelspec->x2[x][0],
           panelspec->y2[x][0], 1.0);
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
  } else if (plot_controls->plot_options & PLOT_REAL) {
    *plotmin_y = plot_ampphase[polidx[0]]->min_real[plot_baseline_idx];
    *plotmax_y = plot_ampphase[polidx[0]]->max_real[plot_baseline_idx];
  } else if (plot_controls->plot_options & PLOT_IMAG) {
    *plotmin_y = plot_ampphase[polidx[0]]->min_imag[plot_baseline_idx];
    *plotmax_y = plot_ampphase[polidx[0]]->max_imag[plot_baseline_idx];
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
    } else if (plot_controls->plot_options & PLOT_REAL) {
      MINASSIGN(*plotmin_y,
                plot_ampphase[polidx[i]]->min_real[plot_baseline_idx]);
      MAXASSIGN(*plotmax_y,
                plot_ampphase[polidx[i]]->max_real[plot_baseline_idx]);
    } else if (plot_controls->plot_options & PLOT_IMAG) {
      MINASSIGN(*plotmin_y,
                plot_ampphase[polidx[i]]->min_imag[plot_baseline_idx]);
      MAXASSIGN(*plotmax_y,
                plot_ampphase[polidx[i]]->max_imag[plot_baseline_idx]);
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
          } else if (plot_controls->plot_options & PLOT_REAL) {
            MINASSIGN(*plotmin_y,
                      plot_ampphase[polidx[j]]->min_real[i]);
            MAXASSIGN(*plotmax_y,
                      plot_ampphase[polidx[j]]->max_real[i]);
          } else if (plot_controls->plot_options & PLOT_IMAG) {
            MINASSIGN(*plotmin_y,
                      plot_ampphase[polidx[j]]->min_imag[i]);
            MAXASSIGN(*plotmax_y,
                      plot_ampphase[polidx[j]]->max_imag[i]);
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
                  int ant1, int ant2, int ifnum, char *if_label,
                  int pol, int colour, int line_style,
                  struct scan_header_data *header_data) {
  struct vis_line *new_line = NULL;
  double dx, dy, dz;
  char polname[8];
  // Add a potential vis line.
  *n_vis_lines += 1;
  REALLOC(*vis_lines, *n_vis_lines);
  MALLOC(new_line, 1);
  new_line->ant1 = ant1;
  new_line->ant2 = ant2;
  strncpy(new_line->if_label, if_label, BUFSIZE);
  new_line->pol = pol;
  pol_to_vis_name(new_line->pol, ifnum, polname);
  snprintf(new_line->label, BUFSIZE, "%d%d%s",
	   new_line->ant1, new_line->ant2, polname);
  dx = header_data->ant_cartesian[ant2 - 1][0] -
    header_data->ant_cartesian[ant1 - 1][0];
  dy = header_data->ant_cartesian[ant2 - 1][1] -
    header_data->ant_cartesian[ant1 - 1][1];
  dz = header_data->ant_cartesian[ant2 - 1][2] -
    header_data->ant_cartesian[ant1 - 1][2];
  new_line->baseline_length = (float)sqrt(dx * dx + dy * dy + dz * dz);

  new_line->pgplot_colour = colour;
  if (line_style == DEFAULT) {
    new_line->line_style = 1;
  } else {
    new_line->line_style = line_style;
  }
  
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

/*!
 *  \brief Make a VIS-type plot, including all annotations
 *  \param cycle_vis_quantities the data to put on the plot; this should be a 3-D array
 *                              of structure pointers, where the first index is over
 *                              all the cycles, the second index is over the windows
 *                              in each cycle, and the third index is over the polarisations
 *  \param ncycles the number of cycles (the first index) in \a cycle_vis_quantities
 *  \param cycle_numifs an array specifying how many windows are in each cycle; should have
 *                      length \a ncycles; each element should correspond to the length of
 *                      the second index of \a cycle_vis_quantities
 *  \param npols the number of polarisations (the third index) in \a cycle_vis_quantities
 *  \param sort_baselines an indicator of whether to sort the baselines according to length
 *                        (set to true), or not (set to false)
 *  \param panelspec a structure that specifies how to divide up the PGPLOT view surface
 *  \param plot_controls specification of the plots to make and how to plot them
 *  \param header_data an array of scan header structure pointers, to specify the scan
 *                     metadata for each cycle; this should have length \a ncycles
 *  \param metinfo an array of `metinfo` structure pointers, to specify the weather metadata
 *                 for each cycle; this should have length \a ncycles
 *  \param syscal_data an array of `syscal_data` structure pointers, to specify the
 *                     system calibration metadata for each cycle; this should have length
 *                     \a ncycles
 *  \param num_times the number of time indicator lines to plot
 *  \param times an array of times to indicate on the plot by vertical dashed lines;
 *               this should have length \a num_times
 */
void make_vis_plot(struct vis_quantities ****cycle_vis_quantities,
                   int ncycles, int *cycle_numifs, int npols,
                   bool sort_baselines,
                   struct panelspec *panelspec,
                   struct vis_plotcontrols *plot_controls,
                   struct scan_header_data **header_data,
                   struct metinfo **metinfo, struct syscal_data **syscal_data,
		   int num_times, float *times) {
  bool show_tsys_legend = false;
  int nants = 0, i = 0, n_vis_lines = 0, j = 0, k = 0, p = 0;
  int singleant = 0, l = 0, m = 0, n = 0, ii, connidx = 0;
  int sysantidx = -1, sysifidx = -1, syspolidx = -1, ap;
  int **n_plot_lines = NULL, ipos = -1, *panel_n_vis_lines = NULL, n_active_ants = 0;
  int n_tsys_vis_lines = 0, n_meta_vis_lines = 0, dbrk, *antprod = NULL, nls = 1;
  float ****plot_lines = NULL, min_x, max_x, min_y, max_y;
  float cxpos, dxpos, labtotalwidth, labspacing, dy, maxwidth, twidth;
  float padlabel = 0.01, cch, timeline_x[2], timeline_y[2];
  float ***antlines = NULL;
  char xopts[BUFSIZE], yopts[BUFSIZE], panellabel[BUFSIZE], panelunits[BUFSIZE];
  char antstring[BUFSIZE], bandstring[BUFSIZE];
  struct vis_line **vis_lines = NULL, **tsys_vis_lines = NULL, **meta_vis_line = NULL;
  struct vis_line ***plot_vis_lines = NULL;
  struct scan_header_data *vlh = NULL;

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

  // Select a representative scan header.
  vlh = header_data[ncycles - 1];

  // Work out how many active antennas there are.
  for (i = 1; i <= MAXANTS; i++) {
    if ((1 << i) & plot_controls->array_spec) {
      n_active_ants += 1;
    }
  }

  // Work out some global needs.
  for (i = 0; i < plot_controls->num_panels; i++) {
    if ((plot_controls->panel_type[i] == VIS_PLOTPANEL_SYSTEMP) ||
        (plot_controls->panel_type[i] == VIS_PLOTPANEL_SYSTEMP_COMPUTED)) {
      show_tsys_legend = true;
    }
  }
  
  // We make up to 16 lines per panel.
  // The first dimension will be the panel number.
  MALLOC(plot_lines, plot_controls->num_panels);
  MALLOC(n_plot_lines, plot_controls->num_panels);
  // We need to ensure we can plot antenna values in unambiguous
  // fashion. We keep track of 2 pols in 2 IFs (this second number
  // will change eventually). This number will be the line style,
  // since the colour is determined by the antenna.
  CALLOC(antprod, 2 * 2);
  CALLOC(antlines, 2 * 2);
  for (i = 0; i < (2 * 2); i++) {
    CALLOC(antlines[i], 2);
    for (j = 0; j < 2; j++) {
      CALLOC(antlines[i][j], 2);
    }
  }
  // The second dimension will be the line number.
  // We need to work out how to allocate up to 16 lines.
  for (p = 0, n_vis_lines = 0, n_tsys_vis_lines = 0;
       p < plot_controls->nproducts; p++) {
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
                               i, j, 1, plot_controls->visbands[0], 
                               POL_XX, DEFAULT, DEFAULT, vlh);
                }
                if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF2) {
                  add_vis_line(&vis_lines, &n_vis_lines,
                               i, j, 2, plot_controls->visbands[1], POL_XX,
                               DEFAULT, DEFAULT, vlh);
                }
              }
              if (plot_controls->vis_products[p]->pol_spec & PLOT_POL_YY) {
                if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF1) {
                  add_vis_line(&vis_lines, &n_vis_lines,
                               i, j, 1, plot_controls->visbands[0], POL_YY,
                               DEFAULT, DEFAULT, vlh);
                }
                if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF2) {
                  add_vis_line(&vis_lines, &n_vis_lines,
                               i, j, 2, plot_controls->visbands[1], POL_YY,
                               DEFAULT, DEFAULT, vlh);
                }
              }
            } else {
              // Auto-correlations allow XY and YX.
              if (plot_controls->vis_products[p]->pol_spec & PLOT_POL_XY) {
                if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF1) {
                  add_vis_line(&vis_lines, &n_vis_lines,
                               i, j, 1, plot_controls->visbands[0], POL_XY,
                               DEFAULT, DEFAULT, vlh);
                }
                if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF2) {
                  add_vis_line(&vis_lines, &n_vis_lines,
                               i, j, 2, plot_controls->visbands[1], POL_XY,
                               DEFAULT, DEFAULT, vlh);
                }
              }
              // Prepare if we're plotting Tsys.
              if (plot_controls->vis_products[p]->pol_spec & PLOT_POL_XX) {
                ap = 0;
                if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF1) {
                  ap += 0 * 2; // For algorithmic purposes later.
                  if (antprod[ap] == 0) {
                    // We fill this product with the next line style and
                    // increment the next line style.
                    antprod[ap] = nls++;
                  }
                  add_vis_line(&tsys_vis_lines, &n_tsys_vis_lines,
                               i, j, 1, plot_controls->visbands[0], POL_XX,
                               (i + 3), antprod[ap], vlh);
                }
                if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF2) {
                  ap += 1 * 2;
                  if (antprod[ap] == 0) {
                    antprod[ap] = nls++;
                  }
                  add_vis_line(&tsys_vis_lines, &n_tsys_vis_lines,
                               i, j, 1, plot_controls->visbands[1], POL_XX,
                               (i + 3), antprod[ap], vlh);
                }
              }
              if (plot_controls->vis_products[p]->pol_spec & PLOT_POL_YY) {
                ap = 1;
                if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF1) {
                  ap += 0 * 2;
                  if (antprod[ap] == 0) {
                    antprod[ap] = nls++;
                  }
                  add_vis_line(&tsys_vis_lines, &n_tsys_vis_lines,
                               i, j, 1, plot_controls->visbands[0], POL_YY,
                               (i + 3), antprod[ap], vlh);
                }
                if (plot_controls->vis_products[p]->if_spec & VIS_PLOT_IF2) {
                  ap += 1 * 2;
                  if (antprod[ap] == 0) {
                    antprod[ap] = nls++;
                  }
                  add_vis_line(&tsys_vis_lines, &n_tsys_vis_lines,
                               i, j, 1, plot_controls->visbands[1], POL_YY,
                               (i + 3), antprod[ap], vlh);
                }
              }
            }
          }
        }
      }
    }
  }
  // Prepare if we're plotting site metadata.
  add_vis_line(&meta_vis_line, &n_meta_vis_lines,
               1, 1, 1, plot_controls->visbands[0], POL_XX, 1, 1, vlh);
  //printf("generating %d vis lines\n", n_vis_lines);
  if ((n_vis_lines == 0) && (n_tsys_vis_lines == 0) && (n_meta_vis_lines == 0)) {
    // Nothing to plot!
    return;
  }

  if (sort_baselines) {
    // Sort the lines into baseline length order.
    qsort(vis_lines, n_vis_lines, sizeof(struct vis_line *), cmpfunc_baseline_length);
  }

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
  MINASSIGN(max_x, (min_x + plot_controls->history_length * 60));
  
  // Always keep another 5% on the right side.
  max_x += (max_x - min_x) * 0.05;

  CALLOC(panel_n_vis_lines, plot_controls->num_panels);
  CALLOC(plot_vis_lines, plot_controls->num_panels);
  for (i = 0; i < plot_controls->num_panels; i++) {
    min_y = INFINITY;
    max_y = -INFINITY;

    if ((plot_controls->panel_type[i] == VIS_PLOTPANEL_AMPLITUDE) ||
        (plot_controls->panel_type[i] == VIS_PLOTPANEL_PHASE) ||
        (plot_controls->panel_type[i] == VIS_PLOTPANEL_DELAY)) {

      // Make the lines.
      panel_n_vis_lines[i] = n_vis_lines;
      MALLOC(plot_lines[i], n_vis_lines);
      MALLOC(n_plot_lines[i], n_vis_lines);
      CALLOC(plot_vis_lines[i], n_vis_lines);
      
      // Now go through the data and make our arrays.
      // The third dimension of the plot lines will be the X and Y
      // coordinates, then the cycle time in seconds.
      // We plot per baseline (or autocorrelation).
      for (j = 0; j < n_vis_lines; j++) {
        n_plot_lines[i][j] = 0;
        MALLOC(plot_lines[i][j], 3);
        plot_lines[i][j][0] = NULL;
        plot_lines[i][j][1] = NULL;
        plot_lines[i][j][2] = NULL;
        plot_vis_lines[i][j] = vis_lines[j];
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
                    if (plot_controls->panel_type[i] == VIS_PLOTPANEL_AMPLITUDE) {
                      plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                        cycle_vis_quantities[k][l][m]->amplitude[n][0];
                    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_PHASE) {
                      plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                        cycle_vis_quantities[k][l][m]->phase[n][0];
                    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_DELAY) {
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
    } else if ((plot_controls->panel_type[i] == VIS_PLOTPANEL_SYSTEMP) ||
               (plot_controls->panel_type[i] == VIS_PLOTPANEL_SYSTEMP_COMPUTED) ||
               (plot_controls->panel_type[i] == VIS_PLOTPANEL_GTP) ||
               (plot_controls->panel_type[i] == VIS_PLOTPANEL_SDO) ||
               (plot_controls->panel_type[i] == VIS_PLOTPANEL_CALJY)) {
      // Make a metadata plot that only has antenna-based lines.
      panel_n_vis_lines[i] = n_tsys_vis_lines;
      MALLOC(plot_lines[i], n_tsys_vis_lines);
      MALLOC(n_plot_lines[i], n_tsys_vis_lines);
      CALLOC(plot_vis_lines[i], n_tsys_vis_lines);
      for (j = 0; j < n_tsys_vis_lines; j++) {
        n_plot_lines[i][j] = 0;
        CALLOC(plot_lines[i][j], 3);
        plot_vis_lines[i][j] = tsys_vis_lines[j];
        for (k = 0; k < ncycles; k++) {
          for (l = 0; l < cycle_numifs[k]; l++) {
            for (m = 0; m < npols; m++) {
              if ((cycle_vis_quantities[k][l][m]->ut_seconds < min_x) ||
                  (cycle_vis_quantities[k][l][m]->ut_seconds > max_x)) {
                break;
              }
              // We need to find the correct Tsys to show.
              // We choose to plot the Tsys of the first antenna of the baseline,
              // unless the second is antenna 6, at which point we plot it.
              sysantidx = sysifidx = syspolidx = -1;
              /* fprintf(stderr, "[make_vis_plot] search for ant %d, pol %d, IF %d\n", */
              /*         tsys_vis_lines[j]->ant1, cycle_vis_quantities[k][l][m]->pol, */
              /*         cycle_vis_quantities[k][l][m]->window); */
              for (ii = 0; ii < syscal_data[k]->num_ants; ii++) {
                if (syscal_data[k]->ant_num[ii] == tsys_vis_lines[j]->ant1) {
                  sysantidx = ii;
                  break;
                }
              }
              for (ii = 0; ii < syscal_data[k]->num_ifs; ii++) {
                if ((syscal_data[k]->if_num[ii] ==
                    cycle_vis_quantities[k][l][m]->window) &&
                    (cycle_vis_quantities[k][l][m]->window ==
                     find_if_name(header_data[k], tsys_vis_lines[j]->if_label))) {
                  sysifidx = ii;
                  break;
                }
              }
              for (ii = 0; ii < syscal_data[k]->num_pols; ii++) {
                if ((syscal_data[k]->pol[ii] == cycle_vis_quantities[k][l][m]->pol) &&
                    (tsys_vis_lines[j]->pol == syscal_data[k]->pol[ii])) {
                  syspolidx = ii;
                  break;
                }
              }
              if ((sysantidx > -1) && (sysifidx > -1) && (syspolidx > -1)) {
                n_plot_lines[i][j] += 1;
                REALLOC(plot_lines[i][j][0], n_plot_lines[i][j]);
                REALLOC(plot_lines[i][j][1], n_plot_lines[i][j]);
                REALLOC(plot_lines[i][j][2], n_plot_lines[i][j]);
                plot_lines[i][j][0][n_plot_lines[i][j] - 1] =
                  cycle_vis_quantities[k][l][m]->ut_seconds;
                if (header_data != NULL) {
                  plot_lines[i][j][2][n_plot_lines[i][j] - 1] =
                    header_data[k]->cycle_time;
                } else {
                  plot_lines[i][j][2][n_plot_lines[i][j] - 1] =
                    plot_controls->cycletime;
                }
                /* fprintf(stderr, "  adding plot line\n"); */
                if (plot_controls->panel_type[i] == VIS_PLOTPANEL_SYSTEMP) {
                  plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                    syscal_data[k]->online_tsys[sysantidx][sysifidx][syspolidx];
                } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_SYSTEMP_COMPUTED) {
                  plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                    syscal_data[k]->computed_tsys[sysantidx][sysifidx][syspolidx];
                } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_GTP) {
                  plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                    syscal_data[k]->gtp[sysantidx][sysifidx][syspolidx];
                } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_SDO) {
                  plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                    syscal_data[k]->sdo[sysantidx][sysifidx][syspolidx];
                } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_CALJY) {
                  plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                    syscal_data[k]->caljy[sysantidx][sysifidx][syspolidx];
                }
                MINASSIGN(min_y, plot_lines[i][j][1][n_plot_lines[i][j] - 1]);
                MAXASSIGN(max_y, plot_lines[i][j][1][n_plot_lines[i][j] - 1]);
                break;
              }
            }
          }
        }
      }
    } else if ((plot_controls->panel_type[i] == VIS_PLOTPANEL_TEMPERATURE) ||
               (plot_controls->panel_type[i] == VIS_PLOTPANEL_PRESSURE) ||
               (plot_controls->panel_type[i] == VIS_PLOTPANEL_HUMIDITY) ||
               (plot_controls->panel_type[i] == VIS_PLOTPANEL_WINDSPEED) ||
               (plot_controls->panel_type[i] == VIS_PLOTPANEL_WINDDIR) ||
               (plot_controls->panel_type[i] == VIS_PLOTPANEL_RAINGAUGE) ||
               (plot_controls->panel_type[i] == VIS_PLOTPANEL_SEEMONPHASE) ||
               (plot_controls->panel_type[i] == VIS_PLOTPANEL_SEEMONRMS)) {
      // Make a metadata plot that only has a single site-based line.
      panel_n_vis_lines[i] = 1;
      MALLOC(plot_lines[i], 1);
      MALLOC(n_plot_lines[i], 1);
      CALLOC(plot_vis_lines[i], 1);
      j = 0;
      n_plot_lines[i][j] = 0;
      CALLOC(plot_lines[i][j], 3);
      plot_vis_lines[i][j] = meta_vis_line[j];
      for (k = 0; k < ncycles; k++) {
        dbrk = 0;
        for (l = 0; l < cycle_numifs[k]; l++) {
          for (m = 0; m < npols; m++) {
            if ((cycle_vis_quantities[k][l][m]->ut_seconds < min_x) ||
                (cycle_vis_quantities[k][l][m]->ut_seconds > max_x)) {
              break;
            }
            n_plot_lines[i][j] += 1;
            REALLOC(plot_lines[i][j][0], n_plot_lines[i][j]);
            REALLOC(plot_lines[i][j][1], n_plot_lines[i][j]);
            REALLOC(plot_lines[i][j][2], n_plot_lines[i][j]);
            plot_lines[i][j][0][n_plot_lines[i][j] - 1] =
              cycle_vis_quantities[k][0][0]->ut_seconds;
            if (header_data != NULL) {
              plot_lines[i][j][2][n_plot_lines[i][j] - 1] =
                header_data[k]->cycle_time;
            } else {
              plot_lines[i][j][2][n_plot_lines[i][j] - 1] =
                plot_controls->cycletime;
            }
            if (plot_controls->panel_type[i] == VIS_PLOTPANEL_TEMPERATURE) {
              plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                metinfo[k]->temperature;
            } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_PRESSURE) {
              plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                metinfo[k]->air_pressure;
            } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_HUMIDITY) {
              plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                metinfo[k]->humidity;
            } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_WINDSPEED) {
              plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                metinfo[k]->wind_speed;
            } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_WINDDIR) {
              plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                metinfo[k]->wind_direction;
            } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_RAINGAUGE) {
              plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                metinfo[k]->rain_gauge;
            } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_SEEMONPHASE) {
              plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                metinfo[k]->seemon_phase;
            } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_SEEMONRMS) {
              plot_lines[i][j][1][n_plot_lines[i][j] - 1] =
                metinfo[k]->seemon_rms;
            }
            MINASSIGN(min_y, plot_lines[i][j][1][n_plot_lines[i][j] - 1]);
            MAXASSIGN(max_y, plot_lines[i][j][1][n_plot_lines[i][j] - 1]);
            dbrk = 1;
            break;
          }
          if (dbrk == 1) {
            break;
          }
        }
      }
    }

    // Make the panel.
    changepanel(0, i, panelspec);
    
    // Extend the y-range slightly.
    dy = 0.05 * (max_y - min_y);
    min_y -= dy;
    max_y += dy;
    if ((plot_controls->panel_type[i] == VIS_PLOTPANEL_AMPLITUDE) && (min_y < 0)) {
      // Amplitude cannot be below 0.
      min_y = 0;
    }

    // Finally, if the user has set the panel limits manually, use them
    // instead.
    if (plot_controls->use_panel_limits[i]) {
      min_y = plot_controls->panel_limits_min[i];
      max_y = plot_controls->panel_limits_max[i];
    }

    // Now check that min and max aren't the same, and put in a bit of padding
    // if they are.
    if (min_y == max_y) {
      min_y -= 0.1;
      max_y += 0.1;
    }

    /* printf("plotting panel %d with %.2f <= x <= %.2f, %.2f <= y <= %.2f\n", */
    /* 	   i, min_x, max_x, min_y, max_y); */
    
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
    cpgsls(1);
    cpgtbox(xopts, 0, 0, yopts, 0, 0);
    if (plot_controls->panel_type[i] == VIS_PLOTPANEL_AMPLITUDE) {
      (void)strcpy(panellabel, "Amplitude");
      (void)strcpy(panelunits, "(Pseudo-Jy)");
    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_PHASE) {
      (void)strcpy(panellabel, "Phase");
      (void)strcpy(panelunits, "(degrees)");
    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_DELAY) {
      (void)strcpy(panellabel, "Delay");
      (void)strcpy(panelunits, "(ns)");
    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_TEMPERATURE) {
      (void)strcpy(panellabel, "Air Temp.");
      (void)strcpy(panelunits, "(C)");
    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_PRESSURE) {
      (void)strcpy(panellabel, "Air Press.");
      (void)strcpy(panelunits, "(hPa)");
    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_HUMIDITY) {
      (void)strcpy(panellabel, "Rel. Humidity");
      (void)strcpy(panelunits, "(%)");
    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_WINDSPEED) {
      (void)strcpy(panellabel, "Wind Speed");
      (void)strcpy(panelunits, "(km/h)");
    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_WINDDIR) {
      (void)strcpy(panellabel, "Wind Dir.");
      (void)strcpy(panelunits, "(deg)");
    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_RAINGAUGE) {
      (void)strcpy(panellabel, "Rain");
      (void)strcpy(panelunits, "(mm)");
    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_SEEMONPHASE) {
      (void)strcpy(panellabel, "See. Mon. Phase");
      (void)strcpy(panelunits, "(deg)");
    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_SEEMONRMS) {
      (void)strcpy(panellabel, "See. Mon. RMS");
      (void)strcpy(panelunits, "(micron)");
    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_SYSTEMP) {
      (void)strcpy(panellabel, "Sys. Temp.");
      (void)strcpy(panelunits, "(K)");
    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_SYSTEMP_COMPUTED) {
      (void)strcpy(panellabel, "Comp. Sys. Temp.");
      (void)strcpy(panelunits, "(K)");
    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_GTP) {
      (void)strcpy(panellabel, "GTP");
      (void)strcpy(panelunits, "");
    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_SDO) {
      (void)strcpy(panellabel, "SDO");
      (void)strcpy(panelunits, "");
    } else if (plot_controls->panel_type[i] == VIS_PLOTPANEL_CALJY) {
      (void)strcpy(panellabel, "Noise Cal.");
      (void)strcpy(panelunits, "(Jy)");
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
    }
    if (i == 0) {
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
      // Print a legend of antenna based lines in the middle if
      // necessary.
      cxpos += dxpos;
      cpgqch(&cch);
      if (show_tsys_legend) {
        for (j = 0; j < (2 * 2); j++) {
          if (antprod[j] > 0) {
            snprintf(bandstring, BUFSIZE, "%d%c", (1 + (int)floorf((float)j / 2.0)),
                     'A' + (j % 2));
            twidth = fracwidth(panelspec, min_x, max_x, 0, i, bandstring);
            cpgmtxt("T", 0.5 + cch, cxpos, 0, bandstring);
            cpgsls(antprod[j]);
            antlines[j][0][0] = cxpos;
            antlines[j][0][1] = cxpos + twidth;
            antlines[j][1][0] = antlines[j][1][1] = 0.15;
            cxpos += twidth * 1.5;
          }
        }
      }
        
      // Then print the frequencies of the bands at the top right.
      maxwidth = 0;
      for (j = 0; j < plot_controls->nvisbands; j++) {
        snprintf(bandstring, BUFSIZE, "%c%c,%c%c = %s",
                 ('A' + (j * 2)), ('A' + (j * 2)),
                 ('B' + (j * 2)), ('B' + (j * 2)),
                 plot_controls->visbands[j]);
        twidth = fracwidth(panelspec, min_x, max_x, 0, i, bandstring);
        MAXASSIGN(maxwidth, twidth);
        ipos = find_if_name(vlh, plot_controls->visbands[j]) - 1;
        snprintf(bandstring, BUFSIZE, "%.0f", vlh->if_centre_freq[ipos]);
        twidth = fracwidth(panelspec, min_x, max_x, 0, i, bandstring);
        MAXASSIGN(maxwidth, twidth);
      }
      
      cxpos = 1 - (plot_controls->nvisbands * maxwidth + padlabel);
      for (j = 0; j < plot_controls->nvisbands; j++) {
        ipos = find_if_name(vlh, plot_controls->visbands[j]) - 1;
        snprintf(bandstring, BUFSIZE, "%.0f", vlh->if_centre_freq[ipos]);
        cpgmtxt("T", 0.5, cxpos, 0, bandstring);
        snprintf(bandstring, BUFSIZE, "%c%c,%c%c = %s",
                 ('A' + (j * 2)), ('A' + (j * 2)),
                 ('B' + (j * 2)), ('B' + (j * 2)),
                 plot_controls->visbands[j]);
        cpgmtxt("T", 0.5 + cch, cxpos, 0, bandstring);
        cxpos += maxwidth + (padlabel / (plot_controls->nvisbands - 1));
      }

      
    }
    cpgsci(1);
    for (j = 0; j < panel_n_vis_lines[i]; j++) {
      cpgsci(plot_vis_lines[i][j]->pgplot_colour);
      cpgsls(plot_vis_lines[i][j]->line_style);
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
    // Plot any time indicators.
    for (j = 0; j < num_times; j++) {
      cpgsci(4);
      cpgsls(2);

      if ((times[j] >= min_x) && (times[j] <= max_x)) {
	timeline_x[0] = timeline_x[1] = times[j];
	timeline_y[0] = min_y;
	timeline_y[1] = max_y;
	cpgline(2, timeline_x, timeline_y);
      }
    }
    if ((i == 0) && (show_tsys_legend)) {
      // Make some legend lines.
      changepanel(0, -1, panelspec);
      cpgswin(0, 1, 0, 1);
      cpgsci(3);
      for (j = 0; j < (2 * 2); j++) {
        if (antprod[j] > 0) {
          cpgsls(antprod[j]);
          cpgline(2, antlines[j][0], antlines[j][1]);
        }
      }
    }
  }
  
  // Free our memory.
  for (i = 0; i < n_vis_lines; i++) {
    FREE(vis_lines[i]);
  }
  FREE(vis_lines);
  for (i = 0; i < n_tsys_vis_lines; i++) {
    FREE(tsys_vis_lines[i]);
  }
  FREE(tsys_vis_lines);
  FREE(meta_vis_line[0]);
  FREE(meta_vis_line);
  for (i = 0; i < plot_controls->num_panels; i++) {
    for (j = 0; j < panel_n_vis_lines[i]; j++) {
      for (k = 0; k < 3; k++) {
        FREE(plot_lines[i][j][k]);
      }
      FREE(plot_lines[i][j]);
      /* FREE(plot_vis_lines[i][j]); */
    }
    FREE(n_plot_lines[i]);
    FREE(plot_lines[i]);
    FREE(plot_vis_lines[i]);
  }
  for (i = 0; i < (2 * 2); i++) {
    for (j = 0; j < 2; j++) {
      FREE(antlines[i][j]);
    }
    FREE(antlines[i]);
  }
  FREE(antlines);
  FREE(antprod);
  FREE(panel_n_vis_lines);
  FREE(n_plot_lines);
  FREE(plot_lines);
  FREE(plot_vis_lines);
}

// Definition to align text on a certain line, for use in make_spd_plot.
// Line 0 is at the top of the information area.
#define YPOS_LINE(l) (1.0 - (float)(l + 1) / (float)(panelspec->num_information_lines))

void make_spd_plot(struct ampphase ***cycle_ampphase, struct panelspec *panelspec,
                   struct spd_plotcontrols *plot_controls,
                   struct scan_header_data *scan_header_data,
		   struct syscal_data *compiled_tsys_data,
                   int max_tsys_ifs, bool all_data_present) {
  int i, j, k, ant1, ant2, nants = 0, px, py, iauto = 0, icross = 0, isauto = NO;
  int npols = 0, *polidx = NULL, poli, num_ifs = 0, panels_per_if = 0;
  int idxif, ni, ri, rj, rp, bi, bn, pc, inverted = NO, plot_started = NO;
  int tsys_num_ifs;
  float **panel_plotted = NULL, information_x_pos = 0.01, information_text_width;
  float information_text_height, xaxis_min, xaxis_max, yaxis_min, yaxis_max, theight;
  float ylog_min, ylog_max, pollab_height, pollab_xlen, pollab_ylen, pollab_padding;
  float *plot_xvalues = NULL, *plot_yvalues = NULL, maxlen_tsys;
  float tvchan_yvals[2], tvchan_xvals[2], tvchans[2];
  char ptitle[BIGBUFSIZE], ptype[BUFSIZE], ftype[BUFSIZE], poltitle[BUFSIZE];
  char information_text[BUFSIZE], ***systemp_strings = NULL;
  struct ampphase **ampphase_if = NULL, *avg_ampphase;

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
            cpgptxt(information_x_pos, YPOS_LINE(0), 0, 0, cycle_ampphase[0][0]->obsdate);
            cpglen(4, cycle_ampphase[0][0]->obsdate, &information_text_width,
                   &information_text_height);
            information_x_pos += information_text_width + 0.02;
            seconds_to_hourlabel(cycle_ampphase[0][0]->ut_seconds,
                                 information_text);
            cpgptxt(information_x_pos, YPOS_LINE(0), 0, 0, information_text);
            cpglen(4, information_text, &information_text_width, &information_text_height);
            information_x_pos += information_text_width + 0.02;
            // The name of the source.
            cpgptxt(information_x_pos, YPOS_LINE(0), 0, 0, scan_header_data->source_name[0]);
            cpglen(4, scan_header_data->source_name[0], &information_text_width,
                   &information_text_height);
            information_x_pos += information_text_width + 0.02;
            // Antennas on source.
            information_text[0] = 0;
            for (j = 0; j < cycle_ampphase[0][0]->syscal_data->num_ants; j++) {
              if (cycle_ampphase[0][0]->syscal_data->flagging[j] & 1) {
                information_text[j] = '-';
              } else {
                information_text[j] = '0' + cycle_ampphase[0][0]->syscal_data->ant_num[j];
              }
            }
            information_text[j] = 0;
            cpgptxt(information_x_pos, YPOS_LINE(0), 0, 0, information_text);
            cpglen(4, information_text, &information_text_width, &information_text_height);
            information_x_pos += information_text_width + 0.02;
            // The system temperatures.
            CALLOC(systemp_strings, compiled_tsys_data->num_ants);
            maxlen_tsys = 0;
            // We may only display the Tsys for a maximum number of IFs.
            tsys_num_ifs = ((compiled_tsys_data->num_ifs > max_tsys_ifs) &&
                            (max_tsys_ifs > 0)) ?
              max_tsys_ifs : compiled_tsys_data->num_ifs;
            for (j = 0; j < compiled_tsys_data->num_ants; j++) {
              CALLOC(systemp_strings[j], tsys_num_ifs);
              for (k = 0; k < tsys_num_ifs; k++) {
                CALLOC(systemp_strings[j][k], BUFSIZE);
                snprintf(systemp_strings[j][k], BUFSIZE, "%.1f / %.1f",
                         compiled_tsys_data->online_tsys[j][k][CAL_XX],
                         compiled_tsys_data->online_tsys[j][k][CAL_YY]);
                cpglen(4, systemp_strings[j][k], &information_text_width,
                       &information_text_height);
                MAXASSIGN(maxlen_tsys, information_text_width);
              }
            }
            for (j = 0; j <= tsys_num_ifs; j++) {
              for (k = 0; k <= compiled_tsys_data->num_ants; k++) {
                if ((j == 0) && (k > 0)) {
                  snprintf(information_text, BUFSIZE, "CA0%d",
                           compiled_tsys_data->ant_num[k - 1]);
                  cpgptxt((information_x_pos + maxlen_tsys +
                           ((float)(k - 1) * (maxlen_tsys + 0.02))),
                          YPOS_LINE(0), 0, 0.5, information_text);
                } else if ((j > 0) && (k == 0)) {
                  snprintf(information_text, BUFSIZE, "IF%d",
                           compiled_tsys_data->if_num[j - 1]);
                  cpgptxt(information_x_pos, YPOS_LINE(j), 0, 0, information_text);
                } else if ((j == 0) && (k == 0)) {
                  cpgptxt(information_x_pos, YPOS_LINE(0), 0, 0, "TSYS");
                } else {
                  cpgptxt((information_x_pos + (maxlen_tsys / 2.0) +
                           ((float)(k - 1) * (maxlen_tsys + 0.02))),
                          YPOS_LINE(j), 0, 0, systemp_strings[k - 1][j - 1]);
                }
              }
            }
	    // Free the memory from the systemp strings.
	    for (j = 0; j < compiled_tsys_data->num_ants; j++) {
	      for (k = 0; k < tsys_num_ifs; k++) {
		FREE(systemp_strings[j][k]);
	      }
	      FREE(systemp_strings[j]);
	    }
	    FREE(systemp_strings);
            // Reset back to the left and plot some weather parameters.
            information_x_pos = 0.01;
            snprintf(information_text, BUFSIZE, "T=%.1f C P=%.1f hPa H=%.1f %%",
                     cycle_ampphase[0][0]->metinfo.temperature,
                     cycle_ampphase[0][0]->metinfo.air_pressure,
                     cycle_ampphase[0][0]->metinfo.humidity);
            cpgptxt(information_x_pos, YPOS_LINE(1), 0, 0, information_text);
            
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
          } else if (plot_controls->plot_options & PLOT_REAL) {
            if (plot_controls->plot_options & PLOT_AMPLITUDE_LOG) {
              snprintf(ptype, BUFSIZE, "LOG(dB) REAL");
            } else {
              snprintf(ptype, BUFSIZE, "REAL");
            }
          } else if (plot_controls->plot_options & PLOT_IMAG) {
            if (plot_controls->plot_options & PLOT_AMPLITUDE_LOG) {
              snprintf(ptype, BUFSIZE, "LOG(dB) IMAG");
            } else {
              snprintf(ptype, BUFSIZE, "IMAG");
            }
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
	    // Do some averaging if the user wants us to plot the averaged data.
	    if (plot_controls->plot_options & PLOT_AVERAGED_DATA) {
	      avg_ampphase = prepare_ampphase();
	      chanaverage_ampphase(ampphase_if[polidx[rp]], avg_ampphase,
				   ampphase_if[polidx[rp]]->options->delay_averaging[idxif + 1],
				   ampphase_if[polidx[rp]]->options->averaging_method[idxif + 1]);
	    }
	    
            for (bi = 0; bi < bn; bi++) {
              // Check if we actually proceed for this polarisation and bin
              // combination.
              if ((isauto == NO) && (bi > 0) &&
                  ((ampphase_if[polidx[rp]]->pol == POL_XY) ||
                   (ampphase_if[polidx[rp]]->pol == POL_YX))) {
                // Don't plot the second bins for the cross-pols.
                continue;
              }
	      if ((isauto == YES) && (bi == 0) &&
		  (ampphase_if[polidx[rp]]->pol == POL_XY)) {
		// XY pol in the autos comes from when the noise diode is on, not off.
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
                  } else if (plot_controls->plot_options & PLOT_PHASE) {
                    plot_yvalues[ri] =
                      ampphase_if[polidx[rp]]->f_phase[i][bi][rj];
                  } else if (plot_controls->plot_options & PLOT_REAL) {
                    plot_yvalues[ri] = crealf(ampphase_if[polidx[rp]]->f_raw[i][bi][rj]);
                  } else if (plot_controls->plot_options & PLOT_IMAG) {
                    plot_yvalues[ri] = cimagf(ampphase_if[polidx[rp]]->f_raw[i][bi][rj]);
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
                  } else if (plot_controls->plot_options & PLOT_REAL) {
                    plot_yvalues[ri] = crealf(ampphase_if[polidx[rp]]->f_raw[i][bi][ri]);
                  } else if (plot_controls->plot_options & PLOT_IMAG) {
                    plot_yvalues[ri] = cimagf(ampphase_if[polidx[rp]]->f_raw[i][bi][ri]);
                  }
                }
              }
              cpgsci(pc);
              cpgline(ampphase_if[polidx[rp]]->f_nchannels[i][bi],
                      plot_xvalues, plot_yvalues);
	      // Check if the user wants to display the averaged data.
	      if (plot_controls->plot_options & PLOT_AVERAGED_DATA) {
		// Remake the plot values again with the averaged data.
		for (ri = 0, rj = avg_ampphase->f_nchannels[i][bi] - 1;
		     ri < avg_ampphase->f_nchannels[i][bi]; ri++, rj--) {
		  if (inverted == YES) {
		    // Swap the frequencies.
		    if (rp == 0) {
		      plot_xvalues[ri] = avg_ampphase->f_frequency[i][bi][rj];
		    }
		    if (plot_controls->plot_options & PLOT_AMPLITUDE) {
		      if (plot_controls->plot_options & PLOT_AMPLITUDE_LOG) {
			LOGAMP(avg_ampphase->f_amplitude[i][bi][rj], ylog_max,
			       plot_yvalues[ri]);
		      } else {
			plot_yvalues[ri] = avg_ampphase->f_amplitude[i][bi][rj];
		      }
		    } else if (plot_controls->plot_options & PLOT_PHASE) {
		      plot_yvalues[ri] = avg_ampphase->f_phase[i][bi][rj];
		    } else if (plot_controls->plot_options & PLOT_REAL) {
		      plot_yvalues[ri] = crealf(avg_ampphase->f_raw[i][bi][rj]);
		    } else if (plot_controls->plot_options & PLOT_IMAG) {
		      plot_yvalues[ri] = cimagf(avg_ampphase->f_raw[i][bi][rj]);
		    }
		  } else {
		    if (plot_controls->plot_options & PLOT_FREQUENCY) {
		      plot_xvalues[ri] = avg_ampphase->f_frequency[i][bi][ri];
		    } else if (plot_controls->plot_options & PLOT_CHANNEL) {
		      plot_xvalues[ri] = avg_ampphase->f_channel[i][bi][ri];
		    }
		    if (plot_controls->plot_options & PLOT_AMPLITUDE) {
		      if (plot_controls->plot_options & PLOT_AMPLITUDE_LOG) {
			LOGAMP(avg_ampphase->f_amplitude[i][bi][ri], ylog_max,
			       plot_yvalues[ri]);
		      } else {
			plot_yvalues[ri] = avg_ampphase->f_amplitude[i][bi][ri];
		      }
		    } else if (plot_controls->plot_options & PLOT_PHASE) {
		      plot_yvalues[ri] = avg_ampphase->f_phase[i][bi][ri];
		    } else if (plot_controls->plot_options & PLOT_REAL) {
		      plot_yvalues[ri] = crealf(avg_ampphase->f_raw[i][bi][ri]);
		    } else if (plot_controls->plot_options & PLOT_IMAG) {
		      plot_yvalues[ri] = cimagf(avg_ampphase->f_raw[i][bi][ri]);
		    }
		  }
		}
		cpgsci(plot_colour_averaging(pc));
		cpgline(avg_ampphase->f_nchannels[i][bi], plot_xvalues, plot_yvalues);
		cpgsci(pc);
	      }

	      // Store the tvchannel ranges here while we know we're accessing valid data.
	      if (plot_controls->plot_options & PLOT_TVCHANNELS) {
		if (plot_controls->plot_options & PLOT_CHANNEL) {
		  tvchans[0] = (float)ampphase_if[polidx[rp]]->options->min_tvchannel[idxif + 1];
		  tvchans[1] = (float)ampphase_if[polidx[rp]]->options->max_tvchannel[idxif + 1];
		} else if (plot_controls->plot_options & PLOT_FREQUENCY) {
		  // Have to find the frequency of the channels.
		  for (j = ampphase_if[polidx[rp]]->options->min_tvchannel[idxif + 1] - 1;
		       ((j < ampphase_if[polidx[rp]]->nchannels) &&
			(j <= ampphase_if[polidx[rp]]->options->min_tvchannel[idxif + 1] + 1));
		       j++) {
		    if ((int)ampphase_if[polidx[rp]]->channel[j] ==
			ampphase_if[polidx[rp]]->options->min_tvchannel[idxif + 1]) {
		      tvchans[0] = ampphase_if[polidx[rp]]->frequency[j];
		      break;
		    }
		  }
		  for (j = ampphase_if[polidx[rp]]->options->max_tvchannel[idxif + 1] - 1;
		       ((j < ampphase_if[polidx[rp]]->nchannels) &&
			(j <= ampphase_if[polidx[rp]]->options->max_tvchannel[idxif + 1] + 1));
		       j++) {
		    if ((int)ampphase_if[polidx[rp]]->channel[j] ==
			ampphase_if[polidx[rp]]->options->max_tvchannel[idxif + 1]) {
		      tvchans[1] = ampphase_if[polidx[rp]]->frequency[j];
		      break;
		    }
		  }
		}
	      }

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

	      // Add another polarisation label if we've overplotted the
	      // averaged line.
	      if (plot_controls->plot_options & PLOT_AVERAGED_DATA) {
		switch(ampphase_if[polidx[rp]]->pol) {
		case POL_XX:
		  strcpy(poltitle, "AAv");
		  if ((isauto) && (bi > 0)) {
		    strcpy(poltitle, "aav");
		  }
		  break;
		case POL_YY:
		  strcpy(poltitle, "BBv");
		  if ((isauto) && (bi > 0)) {
		    strcpy(poltitle, "bbv");
		  }
		  break;
		case POL_XY:
		  strcpy(poltitle, "ABv");
		  break;
		case POL_YX:
		  strcpy(poltitle, "BAv");
		  break;
		}
		cpgsci(plot_colour_averaging(pc));
		cpgmtxt("B", pollab_height, panel_plotted[px][py], 0.0, poltitle);
		cpgsci(pc);
		cpglen(5, poltitle, &pollab_xlen, &pollab_ylen);
		panel_plotted[px][py] += pollab_xlen * pollab_padding;
	      }
	      
              pc++;
            }
	    // Free memory if required.
	    if (plot_controls->plot_options & PLOT_AVERAGED_DATA) {
	      free_ampphase(&avg_ampphase);
	    }
          }
	  if (plot_controls->plot_options & PLOT_TVCHANNELS) {
	    // Show the tvchannel range.
	    cpgsls(2);
	    cpgsci(7);
	    tvchan_yvals[0] = yaxis_min;
	    tvchan_yvals[1] = yaxis_max;
	    for (j = 0; j < 2; j++) {
	      tvchan_xvals[0] = tvchan_xvals[1] = tvchans[j];
	      cpgline(2, tvchan_xvals, tvchan_yvals);
	    }
	    cpgsls(1);
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
