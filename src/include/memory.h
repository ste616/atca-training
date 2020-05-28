#include <stdlib.h>
#include <stdio.h>

#pragma once

#define ERROR_MALLOC_MESSAGE "Insufficient memory"
#define ERROR_REALLOC_MESSAGE "Reallocation failed"

#define MALLOC(p, n)                                \
  do                                                \
    {                                               \
      if ( !( (p) = malloc(sizeof(*(p)) * (n)) ) )  \
        {                                           \
          perror(ERROR_MALLOC_MESSAGE);             \
          exit(EXIT_FAILURE);                       \
        }                                           \
    }                                               \
  while(0)

#define CALLOC(p, n)                              \
  do                                              \
    {                                             \
      if ( !( (p) = calloc((n), sizeof(*(p))) ) ) \
        {                                         \
          perror(ERROR_MALLOC_MESSAGE);           \
          exit(EXIT_FAILURE);                     \
        }                                         \
    }                                             \
  while(0)

#define REALLOC(p, n) \
  do                                                      \
    {                                                     \
      if ( !( (p) = realloc((p), sizeof(*(p)) * (n)) ) )  \
        {                                                 \
          perror(ERROR_REALLOC_MESSAGE);                  \
          exit(EXIT_FAILURE);                             \
        }                                                 \
    }                                                     \
  while(0)

#define FREE(p) \
do \
{ \
  free(p); \
  p = NULL; \
} \
while(0)

#define ARRAY_APPEND(p, n, v) REALLOC(p, n); p[n - 1] = v

#define STRUCTCOPY(s, d, p) d->p = s->p

