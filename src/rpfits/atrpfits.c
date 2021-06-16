/** \file atrpfits.c
 *  \brief Convenience functions relevant to RPFITS conventions
 *
 * ATCA Training Library
 * (C) Jamie Stevens CSIRO 2020
 *
 * This module contains functions that can interpret the conventions present in
 * the RPFITS standard, for use in all the other routines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "atrpfits.h"
#include "common.h"
#include "memory.h"

/*!
 *  \brief Routine to convert an RPFITS baseline number into the component antennas
 *  \param baseline the baseline number from an RPFITS file
 *  \param ant1 pointer to a variable that upon exit will contain the lower-numbered
 *              antenna of the baseline pair
 *  \param ant2 pointer to a variable that upon exit will contain the higher-numbered
 *              antenna of the baseline pair.
 *
 * The RPFITS baseline number can be uniquely resolved into its component antennas
 * using the rule that baseline number is just 256 * a1 + a2, where a1 is antenna 1,
 * a2 is antenna 2, and a1 <= a2.
 */
void base_to_ants(int baseline,int *ant1,int *ant2){
  *ant2 = baseline % 256;
  *ant1 = (baseline - *ant2) / 256;
}

/*!
 *  \brief Routine to convert two antenna numbers to a baseline.
 *  \param ant1 the first antenna in the baseline pair
 *  \param ant2 the second antenna in the baseline pair
 *  \return the baseline number
 *
 * The RPFITS baseline number is a unique number generated from its component
 * antennas using the rule that baseline number (returned from this routine)
 * is just 256 * a1 + a2, where a1 is antenna 1, a2 is antenna 2 and a1 <= a2.
 * If the antenna 2 number given to this routine is lower than the antenna 1
 * number, this routine will automatically swap them to ensure the proper
 * calculation of the baseline number.
 */
int ants_to_base(int ant1, int ant2) {
  int swapper = -1;

  if (ant1 > ant2) {
    swapper = ant1;
    ant1 = ant2;
    ant2 = swapper;
  }

  return (256 * ant1 + ant2);
}

/*!
 *  \brief Produce a description of the array configuration
 *  \param header_data the header data for the scan
 *  \param o if not set to NULL, put the output in this string instead of
 *           printing it to screen
 *  \param o_len the maximum length of the string that can fit in \a o
 */
void print_array_configuration(struct scan_header_data *header_data,
			       char *o, int o_len) {
  int i, j, r;
  double **baseline_lengths = NULL, diff_x, diff_y, diff_z;
  char **station_names = NULL, array_configuration[ARRAY_NAME_LENGTH];

  // Initialise the strings.
  if (o != NULL) {
    o[0] = 0;
  }

  if (header_data == NULL) {
    info_print(o, o_len, "[print_array_configuration] NULL passed as header\n");
    return;
  }

  MALLOC(station_names, header_data->num_ants);
  MALLOC(baseline_lengths, header_data->num_ants);
  info_print(o, o_len, " ANTENNAS ");
  for (i = 0; i < header_data->num_ants; i++) {
    info_print(o, o_len, "    %1d", header_data->ant_label[i]);
  }
  info_print(o, o_len, "\n     NAME ");
  for (i = 0; i < header_data->num_ants; i++) {
    info_print(o, o_len, " %.4s", header_data->ant_name[i]);
  }
  info_print(o, o_len, "\n  STATION ");
  for (i = 0; i < header_data->num_ants; i++) {
    CALLOC(station_names[i], STATION_NAME_LENGTH);
    CALLOC(baseline_lengths[i], header_data->num_ants);
    baseline_lengths[i][i] = 0;
    r = find_station(header_data->ant_cartesian[i][0],
		     header_data->ant_cartesian[i][1],
		     header_data->ant_cartesian[i][2],
		     station_names[i]);
    if (r == FINDSTATION_NOT_FOUND) {
      strcpy(station_names[i], "???");
    }
    info_print(o, o_len, " %4s", station_names[i]);
    for (j = 0; j < header_data->num_ants; j++) {
      if (j == i) {
	continue;
      }
      diff_x = header_data->ant_cartesian[i][0] - header_data->ant_cartesian[j][0];
      diff_y = header_data->ant_cartesian[i][1] - header_data->ant_cartesian[j][1];
      diff_z = header_data->ant_cartesian[i][2] - header_data->ant_cartesian[j][2];
      baseline_lengths[i][j] = sqrt(diff_x * diff_x + diff_y * diff_y + diff_z * diff_z);
    }
  }
  r = find_array_configuration(station_names, array_configuration);
  if (r == FINDARRAYCONFIG_NOT_FOUND) {
    strcpy(array_configuration, "???");
  }
  info_print(o, o_len, "\n    ARRAY %s\n", array_configuration);
  info_print(o, o_len, " BASELINE LENGTHS (m):\n");
  info_print(o, o_len, "   ");
  for (i = 0; i < header_data->num_ants; i++) {
    info_print(o, o_len, "        %1d", header_data->ant_label[i]);
  }
  info_print(o, o_len, "\n");
  for (i = 0; i < header_data->num_ants; i++) {
    info_print(o, o_len, "  %1d", header_data->ant_label[i]);
    for (j = 0; j < header_data->num_ants; j++) {
      info_print(o, o_len, "   %6.1f", baseline_lengths[i][j]);
    }
    info_print(o, o_len, "\n");
  }
  
  // Free our memory.
  for (i = 0; i < header_data->num_ants; i++) {
    FREE(station_names[i]);
    FREE(baseline_lengths[i]);
  }
  FREE(station_names);
  FREE(baseline_lengths);
}
