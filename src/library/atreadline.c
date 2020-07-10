/**
 * ATCA Training Library: atreadline.c
 * (C) Jamie Stevens CSIRO 2020
 *
 * This module contains functions that interface with readline, that
 * are used throughout the code.
 */

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "atreadline.h"
#include "memory.h"

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

