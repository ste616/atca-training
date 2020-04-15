/**
 * ATCA Training Library: nvis.c
 * (C) Jamie Stevens CSIRO 2020
 *
 * This is a new version of VIS, which can work with both a
 * correlator or the simulator.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <argp.h>
#include <stdbool.h>
#include "memory.h"
#include "packing.h"

const char *argp_program_version = "nvis 1.0";
const char *argp_program_bug_address = "<Jamie.Stevens@csiro.au>";

// Program documentation.
static char doc[] = "new/network VIS";

// Argument description.
static char args_doc[] = "[options]";

// Our options.
static struct argp_option options[] = {
  { "file", 'f', "FILE", 0, "Use an output file as the input" },
  { 0 }
};

#define VISBUFSIZE 1024

// The arguments structure.
struct arguments {
  bool use_file;
  char input_file[VISBUFSIZE];
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch(key) {
  case 'f':
    arguments->use_file = true;
    strncpy(arguments->input_file, arg, VISBUFSIZE);
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

void read_data_from_file(char *filename, struct vis_data *vis_data) {
  FILE *fh = NULL;
  cmp_ctx_t cmp;

  fh = fopen(filename, "rb");
  if (fh == NULL) {
    error_and_exit("Error opening input file");
  }
  cmp_init(&cmp, fh, file_reader, file_skipper, file_writer);
  unpack_vis_data(&cmp, vis_data);
  fclose(fh);
}

int main(int argc, char *argv[]) {
  struct arguments arguments;
  struct vis_data vis_data;
  int i, j;

  // Set the default for the arguments.
  arguments.use_file = false;
  arguments.input_file[0] = 0;

  // Parse the arguments.
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // At the moment we must have a file.
  if (arguments.use_file == false) {
    fprintf(stderr, "NVIS CURRENTLY ONLY SUPPORTS FILE INPUT!\n");
    exit(1);
  }

  // Open and unpack the file if we have one.
  read_data_from_file(arguments.input_file, &vis_data);

  // Let's print something to check.
  printf("VIS data has %d cycles:\n", vis_data.nviscycles);
  for (i = 0; i < vis_data.nviscycles; i++) {
    printf(" Cycle %03d: NUM IFS = %d\n", i, vis_data.num_ifs[i]);
    for (j = 0; j < vis_data.num_ifs[i]; j++) {
      printf("  IF %02d: NUM POLS = %d\n", j, vis_data.num_pols[i][j]);
    }
  }

}
