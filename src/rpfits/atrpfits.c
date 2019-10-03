/**
 * ATCA Training Library: reader.h
 * (C) Jamie Stevens CSIRO 2019
 *
 * This library is designed to read an RPFITS file and for each cycle make available
 * the products which would be available online: spectra, amp, phase, delay,
 * u, v, w, etc.
 * This is so we can make tools which will let observers muck around with what they 
 * would see online in all sorts of real situations, as represented by real data 
 * files.
 * 
 * This module contains the structures and common-use functions necessary for all 
 * the other routines.
 */

#include <stdio.h>
#include <stdlib.h>
#include "atrpfits.h"

/**
 * Routine to convert an RPFITS baseline number into the component antennas.
 */
void base_to_ants(int baseline,int *ant1,int *ant2){
  /* the baseline number is just 256*a1 + a2, where
     a1 is antenna 1, and a2 is antenna 2, and a1<=a2 */

  *ant2 = baseline % 256;
  *ant1 = (baseline - *ant2) / 256;
  
}

/**
 * Routine to convert two antenna numbers to a baseline.
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


