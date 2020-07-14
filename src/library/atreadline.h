/**
 * ATCA Training Library: atreadline.c
 * (C) Jamie Stevens CSIRO 2020
 *
 * This module contains functions that interface with readline, that
 * are used throughout the code.
 */

#pragma once

// This is the maximum number of messages that can be passed to
// the readline message routine (simply because we have a statically allocated
// array to shove the messages in).
#define MAX_N_MESSAGES 100

void readline_print_messages(int nmesg, char **mesg);
