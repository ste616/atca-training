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
#include "atrpfits.h"

/**
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

/**
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


