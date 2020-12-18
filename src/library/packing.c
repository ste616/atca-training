/** \file packing.c
 *  \brief Routines necessary to pack and unpack data structures
 *
 * ATCA Training Library
 * (C) Jamie Stevens CSIRO 2020
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "packing.h"
#include "memory.h"
#include "atnetworking.h"

// Some definitions for the packing routines.
/*!
 *  \brief Produce an error message and exit the process
 *  \param msg the error message to output
 *
 * This routine is used in the packing routines to emit useful error messages.
 */
void error_and_exit(const char *msg) {
  fprintf(stderr, "PACKING ERROR %s\n\n", msg);
  exit(EXIT_FAILURE);
}

/*!
 *  \brief Read a number of bytes from a file handle
 *  \param data a pointer to the variable in which to store the data
 *  \param sz the number of bytes to read
 *  \param fh the file handle to read from
 *  \return true, if the number of bytes requested were able to be read from
 *          \a fh, or false otherwise
 */
bool read_bytes(void *data, size_t sz, FILE *fh) {
  return fread(data, sizeof(uint8_t), sz, fh) == (sz * sizeof(uint8_t));
}

/*!
 *  \brief Read a number of bytes from the CMP file
 *  \param ctx the CMP stream, which should have been initialised to read from
 *             a file
 *  \param data a pointer to the variable in which to store the data
 *  \param limit the maximum number of bytes that can be read into \a data
 */
bool file_reader(cmp_ctx_t *ctx, void *data, size_t limit) {
  return read_bytes(data, limit, (FILE *)ctx->buf);
}

/*!
 *  \brief Skip some number of bytes in the CMP file
 *  \param ctx the CMP stream, which should have been initialised to read from
 *             a file
 *  \param count the number of bytes to skip forward from the current position
 */
bool file_skipper(cmp_ctx_t *ctx, size_t count) {
  return fseek((FILE *)ctx->buf, count, SEEK_CUR);
}

/*!
 *  \brief Write a number of bytes to a file handle
 *  \param ctx the CMP stream, which should have been initialised to read from
 *             a file
 *  \param data a pointer to the variable containing the data to write
 *  \param count the number of bytes to write from \a data
 */
size_t file_writer(cmp_ctx_t *ctx, const void *data, size_t count) {
  return fwrite(data, sizeof(uint8_t), count, (FILE *)ctx->buf);
}

// Routines that wrap around the fundamental serializer routines, to
// make them more useful for our purposes, and to encapsulate error
// handling.

// Boolean.
// Reader.
/*!
 *  \brief Read a boolean value from the data stream
 *  \param cmp the CMP stream
 *  \param value a pointer to the variable in which the value read from the
 *               stream will be stored
 */
void pack_read_bool(cmp_ctx_t *cmp, bool *value) {
  if (!cmp_read_bool(cmp, value)) CMPERROR(cmp);
}
// Writer.
/*!
 *  \brief Write a boolean value into the data stream
 *  \param cmp the CMP stream
 *  \param value the value to encode into the stream
 */
void pack_write_bool(cmp_ctx_t *cmp, bool value) {
  if (!cmp_write_bool(cmp, value)) CMPERROR(cmp);
}

// Signed integer.
// Reader.
/*!
 *  \brief Read a signed integer value from the data stream
 *  \param cmp the CMP stream
 *  \param value a pointer to the variable in which the value read from the
 *               stream will be stored
 */
void pack_read_sint(cmp_ctx_t *cmp, int *value) {
  int64_t v;
  if (!cmp_read_sinteger(cmp, &v)) CMPERROR(cmp);
  *value = (int)v;
}
// Writer.
/*!
 *  \brief Write a signed integer value into the data stream
 *  \param cmp the CMP stream
 *  \param value the value to encode into the stream
 */
void pack_write_sint(cmp_ctx_t *cmp, int value) {
  if (!cmp_write_sint(cmp, value)) CMPERROR(cmp);
}

// Unsigned integer.
// Reader.
/*!
 *  \brief Read an unsigned integer value from the data stream
 *  \param cmp the CMP stream
 *  \param value a pointer to the variable in which the value read from the
 *               stream will be stored
 */
void pack_read_uint(cmp_ctx_t *cmp, unsigned int *value) {
  uint64_t v;
  if (!cmp_read_uinteger(cmp, &v)) CMPERROR(cmp);
  *value = (unsigned int)v;
}
// Writer.
/*!
 *  \brief Write an unsigned integer value into the data stream
 *  \param cmp the CMP stream
 *  \param value the value to encode into the stream
 */
void pack_write_uint(cmp_ctx_t *cmp, unsigned int value) {
  if (!cmp_write_uint(cmp, value)) CMPERROR(cmp);
}

// Float.
// Reader.
/*!
 *  \brief Read a floating-point value from the data stream
 *  \param cmp the CMP stream
 *  \param value a pointer to the variable in which the value read from the
 *               stream will be stored
 */
void pack_read_float(cmp_ctx_t *cmp, float *value) {
  if (!cmp_read_float(cmp, value)) CMPERROR(cmp);
}
// Writer.
/*!
 *  \brief Write a floating-point value into the data stream
 *  \param cmp the CMP stream
 *  \param value the value to encode into the stream
 */
void pack_write_float(cmp_ctx_t *cmp, float value) {
  if (!cmp_write_float(cmp, value)) CMPERROR(cmp);
}

// Double.
// Reader.
/*!
 *  \brief Read a double-precision floating-point value from the data stream
 *  \param cmp the CMP stream
 *  \param value a pointer to the variable in which the value read from the
 *               stream will be stored
 */
void pack_read_double(cmp_ctx_t *cmp, double *value) {
  if (!cmp_read_double(cmp, value)) CMPERROR(cmp);
}
// Writer.
/*!
 *  \brief Write a double-precision floating-point value into the data stream
 *  \param cmp the CMP stream
 *  \param value the value to encode into the stream
 */
void pack_write_double(cmp_ctx_t *cmp, double value) {
  if (!cmp_write_double(cmp, value)) CMPERROR(cmp);
}

// String.
// Reader.
/*!
 *  \brief Read a string value from the data stream
 *  \param cmp the CMP stream
 *  \param value a pointer to the variable in which the value read from the
 *               stream will be stored
 *  \param maxlength the maximum length of string to read, in bytes
 */
void pack_read_string(cmp_ctx_t *cmp, char *value, int maxlength) {
  uint32_t string_size = (uint32_t)maxlength;
  if (!cmp_read_str(cmp, value, &string_size)) CMPERROR(cmp);
}
// Writer.
/*!
 *  \brief Write a string value into the data stream
 *  \param cmp the CMP stream
 *  \param value the value to encode into the stream
 *  \param maxlength the maximum length of the string to write, in bytes;
 *                   this does not have to be the actual length of the
 *                   string, which will be determined by this routine
 */
void pack_write_string(cmp_ctx_t *cmp, char *value, long unsigned int maxlength) {
  int write_length = (strlen(value) < maxlength) ? strlen(value) : maxlength;
  if (!cmp_write_str(cmp, value, write_length)) CMPERROR(cmp);
}

// Arrays.
// Support checker.
/*!
 *  \brief Check that the proposed array read will succeed because the length
 *         recorded in the stream agrees with how many elements we want to read
 *  \param cmp the CMP stream
 *  \param expected_length the expected number of elements that will be coming from
 *         the array that should be the next thing in the stream
 *  \return if the number of elements recorded as being in the array matches
 *          \a expected_length, then this routine will return the same number as
 *          \a expected_length, but from a different source; if there is no match
 *          an error will be generated and execution will stop
 */
unsigned int pack_readarray_checksize(cmp_ctx_t *cmp, unsigned int expected_length) {
  unsigned int size_read;
  char emsg[1024];
  // Get the array size.
  CMPR_ARRAYSIZE(cmp, size_read);

  if (expected_length != size_read) {
    sprintf(emsg, "Read length of %d is different to expected %d",
            size_read, expected_length);
    error_and_exit(emsg);
  }
  return size_read;
}
// Float.
// Reader.
void pack_readarray_float(cmp_ctx_t *cmp, unsigned int expected_length,
                          float *array) {
  unsigned int i;
  pack_readarray_checksize(cmp, expected_length);
  
  for (i = 0; i < expected_length; i++) {
    pack_read_float(cmp, &(array[i]));
  }
}
// Writer.
void pack_writearray_float(cmp_ctx_t *cmp, unsigned int length,
                           float *array) {
  unsigned int i;
  CMPW_ARRAYINIT(cmp, length);
  for (i = 0; i < length; i++) {
    pack_write_float(cmp, array[i]);
  }
}

// Double.
// Reader.
void pack_readarray_double(cmp_ctx_t *cmp, unsigned int expected_length,
                           double *array) {
  unsigned int i;
  pack_readarray_checksize(cmp, expected_length);
  for (i = 0; i < expected_length; i++) {
    pack_read_double(cmp, &(array[i]));
  }
}
// Writer.
void pack_writearray_double(cmp_ctx_t *cmp, unsigned int length,
                            double *array) {
  unsigned int i;
  CMPW_ARRAYINIT(cmp, length);
  for (i = 0; i < length; i++) {
    pack_write_double(cmp, array[i]);
  }
}

// Float complex.
// Reader.
void pack_readarray_floatcomplex(cmp_ctx_t *cmp, unsigned int expected_length,
                                 float complex *array) {
  unsigned int i;
  float fr, fi;
  pack_readarray_checksize(cmp, 2 * expected_length);

  for (i = 0; i < expected_length; i++) {
    pack_read_float(cmp, &fr);
    pack_read_float(cmp, &fi);
    array[i] = fr + fi * I;
  }
}
// Writer.
void pack_writearray_floatcomplex(cmp_ctx_t *cmp, unsigned int length,
                                  float complex *array) {
  unsigned int i;
  CMPW_ARRAYINIT(cmp, 2 * length);

  for (i = 0; i < length; i++) {
    pack_write_float(cmp, creal(array[i]));
    pack_write_float(cmp, cimag(array[i]));
  }
}

// Signed integer.
// Reader.
void pack_readarray_sint(cmp_ctx_t *cmp, unsigned int expected_length,
                         int *array) {
  unsigned int i;
  pack_readarray_checksize(cmp, expected_length);

  for (i = 0; i < expected_length; i++) {
    pack_read_sint(cmp, &(array[i]));
  }
}
// Writer.
void pack_writearray_sint(cmp_ctx_t *cmp, unsigned int length,
                          int *array) {
  unsigned int i;
  CMPW_ARRAYINIT(cmp, length);

  for (i = 0; i < length; i++) {
    pack_write_sint(cmp, array[i]);
  }
}

// String.
// Reader.
void pack_readarray_string(cmp_ctx_t *cmp, unsigned int expected_length,
                           char **array, long unsigned int maxlength) {
  unsigned int i;
  pack_readarray_checksize(cmp, expected_length);

  for (i = 0; i < expected_length; i++) {
    pack_read_string(cmp, array[i], maxlength);
  }
}
// Writer.
void pack_writearray_string(cmp_ctx_t *cmp, unsigned int length,
                            char **array, long unsigned int maxlength) {
  unsigned int i;
  CMPW_ARRAYINIT(cmp, length);

  for (i = 0; i < length; i++) {
    pack_write_string(cmp, array[i], maxlength);
  }
}

void pack_ampphase_options(cmp_ctx_t *cmp, struct ampphase_options *a) {
  // This routine takes an ampphase_options structure and packs it for
  // transport.
  pack_write_bool(cmp, a->phase_in_degrees);
  pack_write_sint(cmp, a->include_flagged_data);
  pack_write_sint(cmp, a->num_ifs);
  pack_writearray_float(cmp, a->num_ifs, a->if_centre_freq);
  pack_writearray_float(cmp, a->num_ifs, a->if_bandwidth);
  pack_writearray_sint(cmp, a->num_ifs, a->if_nchannels);
  pack_writearray_sint(cmp, a->num_ifs, a->min_tvchannel);
  pack_writearray_sint(cmp, a->num_ifs, a->max_tvchannel);
  pack_writearray_sint(cmp, a->num_ifs, a->delay_averaging);
  pack_writearray_sint(cmp, a->num_ifs, a->averaging_method);
  pack_write_bool(cmp, a->systemp_reverse_online);
  pack_write_bool(cmp, a->systemp_apply_computed);
}

void unpack_ampphase_options(cmp_ctx_t *cmp, struct ampphase_options *a) {
  // This routine unpacks an ampphase_options structure from the
  // serializer, and stores it in the passed structure.
  pack_read_bool(cmp, &(a->phase_in_degrees));
  pack_read_sint(cmp, &(a->include_flagged_data));
  pack_read_sint(cmp, &(a->num_ifs));
  MALLOC(a->if_centre_freq, a->num_ifs);
  MALLOC(a->if_bandwidth, a->num_ifs);
  MALLOC(a->if_nchannels, a->num_ifs);
  MALLOC(a->min_tvchannel, a->num_ifs);
  MALLOC(a->max_tvchannel, a->num_ifs);
  MALLOC(a->delay_averaging, a->num_ifs);
  MALLOC(a->averaging_method, a->num_ifs);
  pack_readarray_float(cmp, a->num_ifs, a->if_centre_freq);
  pack_readarray_float(cmp, a->num_ifs, a->if_bandwidth);
  pack_readarray_sint(cmp, a->num_ifs, a->if_nchannels);
  pack_readarray_sint(cmp, a->num_ifs, a->min_tvchannel);
  pack_readarray_sint(cmp, a->num_ifs, a->max_tvchannel);
  pack_readarray_sint(cmp, a->num_ifs, a->delay_averaging);
  pack_readarray_sint(cmp, a->num_ifs, a->averaging_method);
  pack_read_bool(cmp, &(a->systemp_reverse_online));
  pack_read_bool(cmp, &(a->systemp_apply_computed));
}

void pack_ampphase(cmp_ctx_t *cmp, struct ampphase *a) {
  // This routine takes an ampphase structure and packs it for transport.
  int i, j;
  // The number of quantities in each array.
  pack_write_sint(cmp, a->nchannels);
  pack_write_sint(cmp, a->nbaselines);

  // Now the arrays storing the labels for each quantity.
  pack_writearray_float(cmp, a->nchannels, a->channel);
  pack_writearray_float(cmp, a->nchannels, a->frequency);
  pack_writearray_sint(cmp, a->nbaselines, a->baseline);

  // The static quantities.
  pack_write_sint(cmp, a->pol);
  pack_write_sint(cmp, a->window);
  pack_write_string(cmp, a->window_name, 8);
  pack_write_string(cmp, a->obsdate, OBSDATE_LENGTH);
  pack_write_float(cmp, a->ut_seconds);
  pack_write_string(cmp, a->scantype, OBSTYPE_LENGTH);

  // The bin arrays have one element per baseline.
  pack_writearray_sint(cmp, a->nbaselines, a->nbins);

  // The flag array has the indexing:
  // array[baseline][bin]
  for (i = 0; i < a->nbaselines; i++) {
    pack_writearray_sint(cmp, a->nbins[i], a->flagged_bad[i]);
  }

  // The arrays here have the following indexing.
  // array[baseline][bin][channel]
  for (i = 0; i < a->nbaselines; i++) {
    for (j = 0; j < a->nbins[i]; j++) {
      pack_writearray_float(cmp, a->nchannels, a->weight[i][j]);
      pack_writearray_float(cmp, a->nchannels, a->amplitude[i][j]);
      pack_writearray_float(cmp, a->nchannels, a->phase[i][j]);
      pack_writearray_floatcomplex(cmp, a->nchannels, a->raw[i][j]);
    }
  }

  // These next arrays contain the same data as above, but
  // do not include the flagged channels.
  for (i = 0; i < a->nbaselines; i++) {
    for (j = 0; j < a->nbins[i]; j++) {
      pack_write_sint(cmp, a->f_nchannels[i][j]);
      pack_writearray_float(cmp, a->f_nchannels[i][j], a->f_channel[i][j]);
      pack_writearray_float(cmp, a->f_nchannels[i][j], a->f_frequency[i][j]);
      pack_writearray_float(cmp, a->f_nchannels[i][j], a->f_weight[i][j]);
      pack_writearray_float(cmp, a->f_nchannels[i][j], a->f_amplitude[i][j]);
      pack_writearray_float(cmp, a->f_nchannels[i][j], a->f_phase[i][j]);
      pack_writearray_floatcomplex(cmp, a->f_nchannels[i][j], a->f_raw[i][j]);
    }
  }

  // Some metadata.
  pack_write_float(cmp, a->min_amplitude_global);
  pack_write_float(cmp, a->max_amplitude_global);
  pack_write_float(cmp, a->min_phase_global);
  pack_write_float(cmp, a->max_phase_global);
  pack_writearray_float(cmp, a->nbaselines, a->min_amplitude);
  pack_writearray_float(cmp, a->nbaselines, a->max_amplitude);
  pack_writearray_float(cmp, a->nbaselines, a->min_phase);
  pack_writearray_float(cmp, a->nbaselines, a->max_phase);
  pack_writearray_float(cmp, a->nbaselines, a->min_real);
  pack_writearray_float(cmp, a->nbaselines, a->max_real);
  pack_writearray_float(cmp, a->nbaselines, a->min_imag);
  pack_writearray_float(cmp, a->nbaselines, a->max_imag);
  
  pack_ampphase_options(cmp, a->options);
  pack_metinfo(cmp, &(a->metinfo));
  pack_syscal_data(cmp, a->syscal_data);
}

void unpack_ampphase(cmp_ctx_t *cmp, struct ampphase *a) {
  // This routine unpacks an ampphase structure from the
  // serializer and stores it in the passed structure.
  int i, j;
  // The number of quantities in each array.
  pack_read_sint(cmp, &(a->nchannels));
  pack_read_sint(cmp, &(a->nbaselines));

  // Now the arrays storing the labels for each quantity.
  MALLOC(a->channel, a->nchannels);
  pack_readarray_float(cmp, a->nchannels, a->channel);
  MALLOC(a->frequency, a->nchannels);
  pack_readarray_float(cmp, a->nchannels, a->frequency);
  MALLOC(a->baseline, a->nbaselines);
  pack_readarray_sint(cmp, a->nbaselines, a->baseline);

  // The static quantities.
  pack_read_sint(cmp, &(a->pol));
  pack_read_sint(cmp, &(a->window));
  pack_read_string(cmp, a->window_name, 8);
  pack_read_string(cmp, a->obsdate, OBSDATE_LENGTH);
  pack_read_float(cmp, &(a->ut_seconds));
  pack_read_string(cmp, a->scantype, OBSTYPE_LENGTH);

  // The bin arrays have one element per baseline.
  MALLOC(a->nbins, a->nbaselines);
  pack_readarray_sint(cmp, a->nbaselines, a->nbins);

  // The flag array has the indexing:
  // array[baseline][bin]
  MALLOC(a->flagged_bad, a->nbaselines);
  for (i = 0; i < a->nbaselines; i++) {
    MALLOC(a->flagged_bad[i], a->nbins[i]);
    pack_readarray_sint(cmp, a->nbins[i], a->flagged_bad[i]);
  }

  // The arrays here have the following indexing.
  // array[baseline][bin][channel]
  MALLOC(a->weight, a->nbaselines);
  MALLOC(a->amplitude, a->nbaselines);
  MALLOC(a->phase, a->nbaselines);
  MALLOC(a->raw, a->nbaselines);
  for (i = 0; i < a->nbaselines; i++) {
    MALLOC(a->weight[i], a->nbins[i]);
    MALLOC(a->amplitude[i], a->nbins[i]);
    MALLOC(a->phase[i], a->nbins[i]);
    MALLOC(a->raw[i], a->nbins[i]);
    for (j = 0; j < a->nbins[i]; j++) {
      MALLOC(a->weight[i][j], a->nchannels);
      pack_readarray_float(cmp, a->nchannels, a->weight[i][j]);
      MALLOC(a->amplitude[i][j], a->nchannels);
      pack_readarray_float(cmp, a->nchannels, a->amplitude[i][j]);
      MALLOC(a->phase[i][j], a->nchannels);
      pack_readarray_float(cmp, a->nchannels, a->phase[i][j]);
      MALLOC(a->raw[i][j], a->nchannels);
      pack_readarray_floatcomplex(cmp, a->nchannels, a->raw[i][j]);
    }
  }

  
  // These next arrays contain the same data as above, but
  // do not include the flagged channels.
  MALLOC(a->f_nchannels, a->nbaselines);
  MALLOC(a->f_channel, a->nbaselines);
  MALLOC(a->f_frequency, a->nbaselines);
  MALLOC(a->f_weight, a->nbaselines);
  MALLOC(a->f_amplitude, a->nbaselines);
  MALLOC(a->f_phase, a->nbaselines);
  MALLOC(a->f_raw, a->nbaselines);
  for (i = 0; i < a->nbaselines; i++) {
    MALLOC(a->f_nchannels[i], a->nbins[i]);
    MALLOC(a->f_channel[i], a->nbins[i]);
    MALLOC(a->f_frequency[i], a->nbins[i]);
    MALLOC(a->f_weight[i], a->nbins[i]);
    MALLOC(a->f_amplitude[i], a->nbins[i]);
    MALLOC(a->f_phase[i], a->nbins[i]);
    MALLOC(a->f_raw[i], a->nbins[i]);
    for (j = 0; j < a->nbins[i]; j++) {
      pack_read_sint(cmp, &(a->f_nchannels[i][j]));
      MALLOC(a->f_channel[i][j], a->f_nchannels[i][j]);
      pack_readarray_float(cmp, a->f_nchannels[i][j], a->f_channel[i][j]);
      MALLOC(a->f_frequency[i][j], a->f_nchannels[i][j]);
      pack_readarray_float(cmp, a->f_nchannels[i][j], a->f_frequency[i][j]);
      MALLOC(a->f_weight[i][j], a->f_nchannels[i][j]);
      pack_readarray_float(cmp, a->f_nchannels[i][j], a->f_weight[i][j]);
      MALLOC(a->f_amplitude[i][j], a->f_nchannels[i][j]);
      pack_readarray_float(cmp, a->f_nchannels[i][j], a->f_amplitude[i][j]);
      MALLOC(a->f_phase[i][j], a->f_nchannels[i][j]);
      pack_readarray_float(cmp, a->f_nchannels[i][j], a->f_phase[i][j]);
      MALLOC(a->f_raw[i][j], a->f_nchannels[i][j]);
      pack_readarray_floatcomplex(cmp, a->f_nchannels[i][j], a->f_raw[i][j]);
    }
  }

  // Some metadata.
  pack_read_float(cmp, &(a->min_amplitude_global));
  pack_read_float(cmp, &(a->max_amplitude_global));
  pack_read_float(cmp, &(a->min_phase_global));
  pack_read_float(cmp, &(a->max_phase_global));
  MALLOC(a->min_amplitude, a->nbaselines);
  pack_readarray_float(cmp, a->nbaselines, a->min_amplitude);
  MALLOC(a->max_amplitude, a->nbaselines);
  pack_readarray_float(cmp, a->nbaselines, a->max_amplitude);
  MALLOC(a->min_phase, a->nbaselines);
  pack_readarray_float(cmp, a->nbaselines, a->min_phase);
  MALLOC(a->max_phase, a->nbaselines);
  pack_readarray_float(cmp, a->nbaselines, a->max_phase);
  MALLOC(a->min_real, a->nbaselines);
  pack_readarray_float(cmp, a->nbaselines, a->min_real);
  MALLOC(a->max_real, a->nbaselines);
  pack_readarray_float(cmp, a->nbaselines, a->max_real);
  MALLOC(a->min_imag, a->nbaselines);
  pack_readarray_float(cmp, a->nbaselines, a->min_imag);
  MALLOC(a->max_imag, a->nbaselines);
  pack_readarray_float(cmp, a->nbaselines, a->max_imag);

  MALLOC(a->options, 1);
  unpack_ampphase_options(cmp, a->options);
  unpack_metinfo(cmp, &(a->metinfo));
  MALLOC(a->syscal_data, 1);
  unpack_syscal_data(cmp, a->syscal_data);
}

void pack_spectrum_data(cmp_ctx_t *cmp, struct spectrum_data *a) {
  int i, j;

  // The spectrum header.
  pack_scan_header_data(cmp, a->header_data);

  // The number of IFs in this spectra set.
  pack_write_sint(cmp, a->num_ifs);
  // The number of polarisations.
  pack_write_sint(cmp, a->num_pols);

  // The ampphase structures.
  for (i = 0; i < a->num_ifs; i++) {
    for (j = 0; j < a->num_pols; j++) {
      pack_ampphase(cmp, a->spectrum[i][j]);
    }
  }
}

void unpack_spectrum_data(cmp_ctx_t *cmp, struct spectrum_data *a) {
  int i, j;
  // The spectrum header.
  MALLOC(a->header_data, 1);
  unpack_scan_header_data(cmp, a->header_data);
  
  // The number of IFs in this spectra set.
  pack_read_sint(cmp, &(a->num_ifs));
  // The number of polarisations.
  pack_read_sint(cmp, &(a->num_pols));

  // Allocate some memory.
  MALLOC(a->spectrum, a->num_ifs);
  for (i = 0; i < a->num_ifs; i++) {
    MALLOC(a->spectrum[i], a->num_pols);
    for (j = 0; j < a->num_pols; j++) {
      MALLOC(a->spectrum[i][j], 1);
      unpack_ampphase(cmp, a->spectrum[i][j]);
    }
  }
}

void free_spectrum_data(struct spectrum_data *spectrum_data) {
  int i, j;
  for (i = 0; i < spectrum_data->num_ifs; i++) {
    for (j = 0; j < spectrum_data->num_pols; j++) {
      free_ampphase(&(spectrum_data->spectrum[i][j]));
    }
    FREE(spectrum_data->spectrum[i]);
  }
  FREE(spectrum_data->spectrum);
  /* if (spectrum_data->header_data != NULL) { */
  /*   free_scan_header_data(spectrum_data->header_data); */
  /*   FREE(spectrum_data->header_data); */
  /* } */
}

void pack_vis_quantities(cmp_ctx_t *cmp, struct vis_quantities *a) {
  int i;
  
  // The options that were used.
  pack_ampphase_options(cmp, a->options);

  // Number of quantities in the array.
  pack_write_sint(cmp, a->nbaselines);

  // The time.
  pack_write_string(cmp, a->obsdate, OBSDATE_LENGTH);
  pack_write_float(cmp, a->ut_seconds);

  // Labels.
  pack_write_sint(cmp, a->pol);
  pack_write_sint(cmp, a->window);
  pack_writearray_sint(cmp, a->nbaselines, a->nbins);
  pack_writearray_sint(cmp, a->nbaselines, a->baseline);
  pack_writearray_sint(cmp, a->nbaselines, a->flagged_bad);
  pack_write_string(cmp, a->scantype, OBSTYPE_LENGTH);

  // The arrays.
  for (i = 0; i < a->nbaselines; i++) {
    pack_writearray_float(cmp, a->nbins[i], a->amplitude[i]);
    pack_writearray_float(cmp, a->nbins[i], a->phase[i]);
    pack_writearray_float(cmp, a->nbins[i], a->delay[i]);
  }

  // Metadata.
  pack_write_float(cmp, a->min_amplitude);
  pack_write_float(cmp, a->max_amplitude);
  pack_write_float(cmp, a->min_phase);
  pack_write_float(cmp, a->max_phase);
  pack_write_float(cmp, a->min_delay);
  pack_write_float(cmp, a->max_delay);
}

void unpack_vis_quantities(cmp_ctx_t *cmp, struct vis_quantities *a) {
  int i;

  // The options that were used.
  MALLOC(a->options, 1);
  unpack_ampphase_options(cmp, a->options);

  // Number of quantities in the array.
  pack_read_sint(cmp, &(a->nbaselines));

  // The time.
  pack_read_string(cmp, a->obsdate, OBSDATE_LENGTH);
  pack_read_float(cmp, &(a->ut_seconds));
  
  // Labels.
  pack_read_sint(cmp, &(a->pol));
  pack_read_sint(cmp, &(a->window));
  MALLOC(a->nbins, a->nbaselines);
  pack_readarray_sint(cmp, a->nbaselines, a->nbins);
  MALLOC(a->baseline, a->nbaselines);
  pack_readarray_sint(cmp, a->nbaselines, a->baseline);
  MALLOC(a->flagged_bad, a->nbaselines);
  pack_readarray_sint(cmp, a->nbaselines, a->flagged_bad);
  pack_read_string(cmp, a->scantype, OBSTYPE_LENGTH);

  // The arrays.
  MALLOC(a->amplitude, a->nbaselines);
  MALLOC(a->phase, a->nbaselines);
  MALLOC(a->delay, a->nbaselines);
  for (i = 0; i < a->nbaselines; i++) {
    MALLOC(a->amplitude[i], a->nbins[i]);
    pack_readarray_float(cmp, a->nbins[i], a->amplitude[i]);
    MALLOC(a->phase[i], a->nbins[i]);
    pack_readarray_float(cmp, a->nbins[i], a->phase[i]);
    MALLOC(a->delay[i], a->nbins[i]);
    pack_readarray_float(cmp, a->nbins[i], a->delay[i]);
  }

  // Metadata.
  pack_read_float(cmp, &(a->min_amplitude));
  pack_read_float(cmp, &(a->max_amplitude));
  pack_read_float(cmp, &(a->min_phase));
  pack_read_float(cmp, &(a->max_phase));
  pack_read_float(cmp, &(a->min_delay));
  pack_read_float(cmp, &(a->max_delay));
}

void copy_spectrum_data(struct spectrum_data *dest,
			struct spectrum_data *src) {
  // Copy one spd_data structure to another.
  // The spectrum header.
  dest->header_data = src->header_data;

  // The number of IFs in this spectra set.
  dest->num_ifs = src->num_ifs;

  // The number of polarisations.
  dest->num_pols = src->num_pols;

  // The ampphase structures.
  dest->spectrum = src->spectrum;
}

void copy_vis_data(struct vis_data *dest,
                   struct vis_data *src) {
  // Copy one vis_data structure to another.
  // The number of cycles contained here.
  dest->nviscycles = src->nviscycles;

  // The times.
  dest->mjd_low = src->mjd_low;
  dest->mjd_high = src->mjd_high;
  
  // The header data for each cycle.
  dest->header_data = src->header_data;

  // The number of IFs per cycle.
  dest->num_ifs = src->num_ifs;

  // The number of pols per cycle per IF.
  dest->num_pols = src->num_pols;

  // The vis_quantities structures.
  dest->vis_quantities = src->vis_quantities;

  // The metinfo for each cycle.
  dest->metinfo = src->metinfo;

  // And the calibration parameters.
  dest->syscal_data = src->syscal_data;

  // And the options.
  dest->num_options = src->num_options;
  dest->options = src->options;
}

void pack_vis_data(cmp_ctx_t *cmp, struct vis_data *a) {
  int i, j, k;
  // The number of cycles contained here.
  pack_write_sint(cmp, a->nviscycles);

  // The range of MJDs that were allowed when these data
  // were compiled.
  pack_write_double(cmp, a->mjd_low);
  pack_write_double(cmp, a->mjd_high);
  
  // The header data for each cycle.
  for (i = 0; i < a->nviscycles; i++) {
    pack_scan_header_data(cmp, a->header_data[i]);
  }

  // The number of IFs per cycle.
  pack_writearray_sint(cmp, a->nviscycles, a->num_ifs);

  // The number of pols per cycle per IF.
  for (i = 0; i < a->nviscycles; i++) {
    pack_writearray_sint(cmp, a->num_ifs[i], a->num_pols[i]);
  }

  // The vis_quantities structures.
  for (i = 0; i < a->nviscycles; i++) {
    for (j = 0; j < a->num_ifs[i]; j++) {
      for (k = 0; k < a->num_pols[i][j]; k++) {
        pack_vis_quantities(cmp, a->vis_quantities[i][j][k]);
      }
    }
  }

  // The metinfo for each cycle.
  for (i = 0; i < a->nviscycles; i++) {
    pack_metinfo(cmp, a->metinfo[i]);
  }
  // And the calibration parameters.
  for (i = 0; i < a->nviscycles; i++) {
    pack_syscal_data(cmp, a->syscal_data[i]);
  }
  // And the options.
  pack_write_sint(cmp, a->num_options);
  for (i = 0; i < a->num_options; i++) {
    pack_ampphase_options(cmp, a->options[i]);
  }
}

void unpack_vis_data(cmp_ctx_t *cmp, struct vis_data *a) {
  int i, j, k;

  // The number of cycles contained here.
  pack_read_sint(cmp, &(a->nviscycles));

  // The range of MJDs that were allowed when these data
  // were compiled.
  pack_read_double(cmp, &(a->mjd_low));
  pack_read_double(cmp, &(a->mjd_high));
  
  // The header data for each cycle.
  MALLOC(a->header_data, a->nviscycles);
  for (i = 0; i < a->nviscycles; i++) {
    MALLOC(a->header_data[i], 1);
    unpack_scan_header_data(cmp, a->header_data[i]);
  }
  
  // The number of IFs per cycle.
  MALLOC(a->num_ifs, a->nviscycles);
  pack_readarray_sint(cmp, a->nviscycles, a->num_ifs);

  // The number of pols per cycle per IF.
  MALLOC(a->num_pols, a->nviscycles);
  for (i = 0; i < a->nviscycles; i++) {
    MALLOC(a->num_pols[i], a->num_ifs[i]);
    pack_readarray_sint(cmp, a->num_ifs[i], a->num_pols[i]);
  }

  // The vis_quantities structures.
  MALLOC(a->vis_quantities, a->nviscycles);
  for (i = 0; i < a->nviscycles; i++) {
    MALLOC(a->vis_quantities[i], a->num_ifs[i]);
    for (j = 0; j < a->num_ifs[i]; j++) {
      MALLOC(a->vis_quantities[i][j], a->num_pols[i][j]);
      for (k = 0; k < a->num_pols[i][j]; k++) {
        MALLOC(a->vis_quantities[i][j][k], 1);
        unpack_vis_quantities(cmp, a->vis_quantities[i][j][k]);
      }
    }
  }

  // The metinfo for each cycle.
  MALLOC(a->metinfo, a->nviscycles);
  for (i = 0; i < a->nviscycles; i++) {
    MALLOC(a->metinfo[i], 1);
    unpack_metinfo(cmp, a->metinfo[i]);
  }

  // And the calibration parameters.
  MALLOC(a->syscal_data, a->nviscycles);
  for (i = 0; i < a->nviscycles; i++) {
    MALLOC(a->syscal_data[i], 1);
    unpack_syscal_data(cmp, a->syscal_data[i]);
  }
  // And the options.
  pack_read_sint(cmp, &(a->num_options));
  MALLOC(a->options, a->num_options);
  for (i = 0; i < a->num_options; i++) {
    MALLOC(a->options[i], 1);
    unpack_ampphase_options(cmp, a->options[i]);
  }
}

/*!
 *  \brief Free the memory within a vis_data structure, but not the
 *         structure allocation itself
 *  \param vis_data the structure to clear
 */
void free_vis_data(struct vis_data *vis_data) {
  int i, j, k;
  for (i = 0; i < vis_data->nviscycles; i++) {
    for (j = 0; j < vis_data->num_ifs[i]; j++) {
      for (k = 0; k < vis_data->num_pols[i][j]; k++) {
        free_vis_quantities(&(vis_data->vis_quantities[i][j][k]));
        //FREE(vis_data->vis_quantities[i][j][k]);
      }
      FREE(vis_data->vis_quantities[i][j]);
    }
    FREE(vis_data->vis_quantities[i]);
    // We don't free the header data, we assume that will be
    // freed somewhere else.
    /* free_scan_header_data(vis_data->header_data[i]); */
    /* FREE(vis_data->header_data[i]); */
    FREE(vis_data->num_pols[i]);
    // Free the metinfo and calibration data.
    FREE(vis_data->metinfo[i]);
    free_syscal_data(vis_data->syscal_data[i]);
    FREE(vis_data->syscal_data[i]);
  }
  FREE(vis_data->vis_quantities);
  FREE(vis_data->header_data);
  FREE(vis_data->num_ifs);
  FREE(vis_data->num_pols);
  FREE(vis_data->metinfo);
  FREE(vis_data->syscal_data);

  for (i = 0; i < vis_data->num_options; i++) {
    free_ampphase_options(vis_data->options[i]);
    FREE(vis_data->options[i]);
  }
  FREE(vis_data->options);
}

void pack_scan_header_data(cmp_ctx_t *cmp, struct scan_header_data *a) {
  int i;
  // Time variables.
  pack_write_string(cmp, a->obsdate, OBSDATE_LENGTH);
  pack_write_float(cmp, a->ut_seconds);

  // Details about the observation.
  pack_write_string(cmp, a->obstype, OBSTYPE_LENGTH);
  pack_write_string(cmp, a->calcode, CALCODE_LENGTH);
  pack_write_sint(cmp, a->cycle_time);

  // The sources.
  pack_write_sint(cmp, a->num_sources);
  for (i = 0; i < a->num_sources; i++) {
    pack_write_string(cmp, a->source_name[i], SOURCE_LENGTH);
  }

  // Source coordinates.
  pack_writearray_float(cmp, a->num_sources, a->rightascension_hours);
  pack_writearray_float(cmp, a->num_sources, a->declination_degrees);

  // Frequency configuration.
  pack_write_sint(cmp, a->num_ifs);
  pack_writearray_float(cmp, a->num_ifs, a->if_centre_freq);
  pack_writearray_float(cmp, a->num_ifs, a->if_bandwidth);
  pack_writearray_sint(cmp, a->num_ifs, a->if_num_channels);
  pack_writearray_sint(cmp, a->num_ifs, a->if_num_stokes);
  pack_writearray_sint(cmp, a->num_ifs, a->if_sideband);
  pack_writearray_sint(cmp, a->num_ifs, a->if_chain);
  pack_writearray_sint(cmp, a->num_ifs, a->if_label);
  for (i = 0; i < a->num_ifs; i++) {
    pack_writearray_string(cmp, 3, a->if_name[i], 8);
    pack_writearray_string(cmp, a->if_num_stokes[i], a->if_stokes_names[i], 3);
  }
  pack_write_sint(cmp, a->num_ants);
  pack_writearray_sint(cmp, a->num_ants, a->ant_label);
  pack_writearray_string(cmp, a->num_ants, a->ant_name, 9);
  for (i = 0; i < a->num_ants; i++) {
    pack_writearray_double(cmp, 3, a->ant_cartesian[i]);
  }

}

void unpack_scan_header_data(cmp_ctx_t *cmp, struct scan_header_data *a) {
  int i, j;
  // Time variables.
  pack_read_string(cmp, a->obsdate, OBSDATE_LENGTH);
  pack_read_float(cmp, &(a->ut_seconds));

  // Details about the observation.
  pack_read_string(cmp, a->obstype, OBSTYPE_LENGTH);
  pack_read_string(cmp, a->calcode, CALCODE_LENGTH);
  pack_read_sint(cmp, &(a->cycle_time));

  // The sources.
  pack_read_sint(cmp, &(a->num_sources));
  MALLOC(a->source_name, a->num_sources);
  for (i = 0; i < a->num_sources; i++) {
    MALLOC(a->source_name[i], SOURCE_LENGTH);
    pack_read_string(cmp, a->source_name[i], SOURCE_LENGTH);
  }
  
  // Source coordinates.
  MALLOC(a->rightascension_hours, a->num_sources);
  pack_readarray_float(cmp, a->num_sources, a->rightascension_hours);
  MALLOC(a->declination_degrees, a->num_sources);
  pack_readarray_float(cmp, a->num_sources, a->declination_degrees);
  
  // Frequency configuration.
  pack_read_sint(cmp, &(a->num_ifs));
  MALLOC(a->if_centre_freq, a->num_ifs);
  pack_readarray_float(cmp, a->num_ifs, a->if_centre_freq);
  MALLOC(a->if_bandwidth, a->num_ifs);
  pack_readarray_float(cmp, a->num_ifs, a->if_bandwidth);
  MALLOC(a->if_num_channels, a->num_ifs);
  pack_readarray_sint(cmp, a->num_ifs, a->if_num_channels);
  
  MALLOC(a->if_num_stokes, a->num_ifs);
  pack_readarray_sint(cmp, a->num_ifs, a->if_num_stokes);
  MALLOC(a->if_sideband, a->num_ifs);
  pack_readarray_sint(cmp, a->num_ifs, a->if_sideband);
  MALLOC(a->if_chain, a->num_ifs);
  pack_readarray_sint(cmp, a->num_ifs, a->if_chain);
  MALLOC(a->if_label, a->num_ifs);
  pack_readarray_sint(cmp, a->num_ifs, a->if_label);
  MALLOC(a->if_name, a->num_ifs);
  MALLOC(a->if_stokes_names, a->num_ifs);
  for (i = 0; i < a->num_ifs; i++) {
    MALLOC(a->if_name[i], 3);
    for (j = 0; j < 3; j++) {
      MALLOC(a->if_name[i][j], 8);
    }
    MALLOC(a->if_stokes_names[i], a->if_num_stokes[i]);
    for (j = 0; j < a->if_num_stokes[i]; j++) {
      MALLOC(a->if_stokes_names[i][j], 3);
    }
    pack_readarray_string(cmp, 3, a->if_name[i], 8);
    pack_readarray_string(cmp, a->if_num_stokes[i], a->if_stokes_names[i], 3);
  }
  pack_read_sint(cmp, &(a->num_ants));
  MALLOC(a->ant_label, a->num_ants);
  pack_readarray_sint(cmp, a->num_ants, a->ant_label);
  MALLOC(a->ant_name, a->num_ants);
  for (i = 0; i < a->num_ants; i++) {
    MALLOC(a->ant_name[i], 9);
  }
  pack_readarray_string(cmp, a->num_ants, a->ant_name, 9);
  MALLOC(a->ant_cartesian, a->num_ants);
  for (i = 0; i < a->num_ants; i++) {
    MALLOC(a->ant_cartesian[i], 3);
    pack_readarray_double(cmp, 3, a->ant_cartesian[i]);
  }
  
}

/*!
 *  \brief Pack a requests structure into the CMP buffer
 *  \param cmp the CMP buffer object
 *  \param a the requests structure
 */
void pack_requests(cmp_ctx_t *cmp, struct requests *a) {
  // The request type.
  pack_write_sint(cmp, a->request_type);
  // The ID of the client.
  pack_write_string(cmp, a->client_id, CLIENTIDLENGTH);
  // The username of the client.
  pack_write_string(cmp, a->client_username, CLIENTIDLENGTH);
  // The type of client.
  pack_write_sint(cmp, a->client_type);
}

/*!
 *  \brief Read a requests structure from the CMP buffer
 *  \param cmp the CMP buffer object
 *  \param a the requests structure, which must already be allocated
 *           before passing it
 */
void unpack_requests(cmp_ctx_t *cmp, struct requests *a) {
  // The request type
  pack_read_sint(cmp, &(a->request_type));
  // The ID of the client.
  pack_read_string(cmp, a->client_id, CLIENTIDLENGTH);
  // The username of the client.
  pack_read_string(cmp, a->client_username, CLIENTIDLENGTH);
  // The type of client.
  pack_read_sint(cmp, &(a->client_type));
}

void pack_responses(cmp_ctx_t *cmp, struct responses *a) {
  // The response type.
  pack_write_sint(cmp, a->response_type);
  // The ID of the client that requested the new data, or
  // all 0s if it's just new data.
  pack_write_string(cmp, a->client_id, CLIENTIDLENGTH);
}

void unpack_responses(cmp_ctx_t *cmp, struct responses *a) {
  // The response type.
  pack_read_sint(cmp, &(a->response_type));
  // The ID of the client that requested the new data, or
  // all 0s if it's just new data.
  pack_read_string(cmp, a->client_id, CLIENTIDLENGTH);
}

void pack_metinfo(cmp_ctx_t *cmp, struct metinfo *a) {
  // Labelling.
  pack_write_string(cmp, a->obsdate, OBSDATE_LENGTH);
  pack_write_float(cmp, a->ut_seconds);

  // Quantities from the weather station.
  pack_write_float(cmp, a->temperature);
  pack_write_float(cmp, a->air_pressure);
  pack_write_float(cmp, a->humidity);
  pack_write_float(cmp, a->wind_speed);
  pack_write_float(cmp, a->wind_direction);
  pack_write_float(cmp, a->rain_gauge);
  pack_write_bool(cmp, a->weather_valid);

  // Quantities from the seeing monitor.
  pack_write_float(cmp, a->seemon_phase);
  pack_write_float(cmp, a->seemon_rms);
  pack_write_bool(cmp, a->seemon_valid);
}

void unpack_metinfo(cmp_ctx_t *cmp, struct metinfo *a) {
  // Labelling.
  pack_read_string(cmp, a->obsdate, OBSDATE_LENGTH);
  pack_read_float(cmp, &(a->ut_seconds));

  // Quantities from the weather station.
  pack_read_float(cmp, &(a->temperature));
  pack_read_float(cmp, &(a->air_pressure));
  pack_read_float(cmp, &(a->humidity));
  pack_read_float(cmp, &(a->wind_speed));
  pack_read_float(cmp, &(a->wind_direction));
  pack_read_float(cmp, &(a->rain_gauge));
  pack_read_bool(cmp, &(a->weather_valid));

  // Quantities from the seeing monitor.
  pack_read_float(cmp, &(a->seemon_phase));
  pack_read_float(cmp, &(a->seemon_rms));
  pack_read_bool(cmp, &(a->seemon_valid));
}

void pack_syscal_data(cmp_ctx_t *cmp, struct syscal_data *a) {
  int i, j;
  
  // Labelling.
  pack_write_string(cmp, a->obsdate, OBSDATE_LENGTH);
  pack_write_float(cmp, a->utseconds);

  // Array sizes.
  pack_write_sint(cmp, a->num_ifs);
  pack_write_sint(cmp, a->num_ants);
  pack_write_sint(cmp, a->num_pols);

  // Array descriptors.
  if (a->num_ifs > 0) {
    pack_writearray_sint(cmp, a->num_ifs, a->if_num);
  }
  pack_writearray_sint(cmp, a->num_ants, a->ant_num);
  if (a->num_pols > 0) {
    pack_writearray_sint(cmp, a->num_pols, a->pol);
  }

  // Parameters that only depend on antenna.
  pack_writearray_float(cmp, a->num_ants, a->parangle);
  pack_writearray_float(cmp, a->num_ants, a->tracking_error_max);
  pack_writearray_float(cmp, a->num_ants, a->tracking_error_rms);
  pack_writearray_sint(cmp, a->num_ants, a->flagging);

  // Parameters that vary for each antenna and IF.
  if (a->num_ifs > 0) {
    for (i = 0; i < a->num_ants; i++) {
      pack_writearray_float(cmp, a->num_ifs, a->xyphase[i]);
      pack_writearray_float(cmp, a->num_ifs, a->xyamp[i]);
    }
  }

  // The system temperatures (varies by antenna, IF and pol).
  if ((a->num_ifs > 0) && (a->num_pols > 0)) {
    for (i = 0; i < a->num_ants; i++) {
      for (j = 0; j < a->num_ifs; j++) {
        pack_writearray_float(cmp, a->num_pols, a->online_tsys[i][j]);
        pack_writearray_sint(cmp, a->num_pols, a->online_tsys_applied[i][j]);
        pack_writearray_float(cmp, a->num_pols, a->computed_tsys[i][j]);
        pack_writearray_sint(cmp, a->num_pols, a->computed_tsys_applied[i][j]);
      }
    }
  }

  // Paramters measured by the correlator (varies by antenna, IF and pol).
  if ((a->num_ifs > 0) && (a->num_pols > 0)) {
    for (i = 0; i < a->num_ants; i++) {
      for (j = 0; j < a->num_ifs; j++) {
        pack_writearray_float(cmp, a->num_pols, a->gtp[i][j]);
        pack_writearray_float(cmp, a->num_pols, a->sdo[i][j]);
        pack_writearray_float(cmp, a->num_pols, a->caljy[i][j]);
      }
    }
  }
}

void unpack_syscal_data(cmp_ctx_t *cmp, struct syscal_data *a) {
  int i, j;

  // Labelling.
  pack_read_string(cmp, a->obsdate, OBSDATE_LENGTH);
  pack_read_float(cmp, &(a->utseconds));

  // Array sizes.
  pack_read_sint(cmp, &(a->num_ifs));
  pack_read_sint(cmp, &(a->num_ants));
  pack_read_sint(cmp, &(a->num_pols));

  // Array descriptors.
  if (a->num_ifs > 0) {
    MALLOC(a->if_num, a->num_ifs);
    pack_readarray_sint(cmp, a->num_ifs, a->if_num);
  } else {
    a->if_num = NULL;
  }
  MALLOC(a->ant_num, a->num_ants);
  pack_readarray_sint(cmp, a->num_ants, a->ant_num);
  if (a->num_pols > 0) {
    MALLOC(a->pol, a->num_pols);
    pack_readarray_sint(cmp, a->num_pols, a->pol);
  } else {
    a->pol = NULL;
  }

  // Parameters that only depend on antenna.
  MALLOC(a->parangle, a->num_ants);
  pack_readarray_float(cmp, a->num_ants, a->parangle);
  MALLOC(a->tracking_error_max, a->num_ants);
  pack_readarray_float(cmp, a->num_ants, a->tracking_error_max);
  MALLOC(a->tracking_error_rms, a->num_ants);
  pack_readarray_float(cmp, a->num_ants, a->tracking_error_rms);
  MALLOC(a->flagging, a->num_ants);
  pack_readarray_sint(cmp, a->num_ants, a->flagging);

  // Parameters that vary for each antenna and IF.
  if (a->num_ifs > 0) {
    MALLOC(a->xyphase, a->num_ants);
    MALLOC(a->xyamp, a->num_ants);
    for (i = 0; i < a->num_ants; i++) {
      MALLOC(a->xyphase[i], a->num_ifs);
      pack_readarray_float(cmp, a->num_ifs, a->xyphase[i]);
      MALLOC(a->xyamp[i], a->num_ifs);
      pack_readarray_float(cmp, a->num_ifs, a->xyamp[i]);
    }
  } else {
    a->xyphase = NULL;
    a->xyamp = NULL;
  }

  // The system temperatures (varies by antenna, IF and pol).
  if ((a->num_ifs > 0) && (a->num_pols > 0)) {
    MALLOC(a->online_tsys, a->num_ants);
    MALLOC(a->online_tsys_applied, a->num_ants);
    MALLOC(a->computed_tsys, a->num_ants);
    MALLOC(a->computed_tsys_applied, a->num_ants);
    for (i = 0; i < a->num_ants; i++) {
      MALLOC(a->online_tsys[i], a->num_ifs);
      MALLOC(a->online_tsys_applied[i], a->num_ifs);
      MALLOC(a->computed_tsys[i], a->num_ifs);
      MALLOC(a->computed_tsys_applied[i], a->num_ifs);
      for (j = 0; j < a->num_ifs; j++) {
        MALLOC(a->online_tsys[i][j], a->num_pols);
        pack_readarray_float(cmp, a->num_pols, a->online_tsys[i][j]);
        MALLOC(a->online_tsys_applied[i][j], a->num_pols);
        pack_readarray_sint(cmp, a->num_pols, a->online_tsys_applied[i][j]);
        MALLOC(a->computed_tsys[i][j], a->num_pols);
        pack_readarray_float(cmp, a->num_pols, a->computed_tsys[i][j]);
        MALLOC(a->computed_tsys_applied[i][j], a->num_pols);
        pack_readarray_sint(cmp, a->num_pols, a->computed_tsys_applied[i][j]);
      }
    }
  } else {
    a->online_tsys = NULL;
    a->online_tsys_applied = NULL;
    a->computed_tsys = NULL;
    a->computed_tsys_applied = NULL;
  }
  
  // Parameters measured by the correlator (varies by antenna, IF and pol).
  if ((a->num_ifs > 0) && (a->num_pols > 0)) {
    MALLOC(a->gtp, a->num_ants);
    MALLOC(a->sdo, a->num_ants);
    MALLOC(a->caljy, a->num_ants);
    for (i = 0; i < a->num_ants; i++) {
      MALLOC(a->gtp[i], a->num_ifs);
      MALLOC(a->sdo[i], a->num_ifs);
      MALLOC(a->caljy[i], a->num_ifs);
      for (j = 0; j < a->num_ifs; j++) {
        MALLOC(a->gtp[i][j], a->num_pols);
        pack_readarray_float(cmp, a->num_pols, a->gtp[i][j]);
        MALLOC(a->sdo[i][j], a->num_pols);
        pack_readarray_float(cmp, a->num_pols, a->sdo[i][j]);
        MALLOC(a->caljy[i][j], a->num_pols);
        pack_readarray_float(cmp, a->num_pols, a->caljy[i][j]);
      }
    }
  } else {
    a->gtp = NULL;
    a->sdo = NULL;
    a->caljy = NULL;
  }
  
}

void init_cmp_memory_buffer(cmp_ctx_t *cmp, cmp_mem_access_t *mem, void *buffer,
                            size_t buffer_len) {
  cmp_mem_access_init(cmp, mem, buffer, buffer_len);
}
