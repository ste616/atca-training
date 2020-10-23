/** \file memory.h
 *  \brief Macros for allocating, deallocating and copying memory and structures.
 *
 * ATCA Training Library
 * (C) Jamie Stevens CSIRO 2020
 *
 * This module has some macros that are used all over the codebase to allocate,
 * reallocate, free and copy memory.
 */
#include <stdlib.h>
#include <stdio.h>

#pragma once

/*! \def ERROR_MALLOC_MESSAGE
 *  \brief This is the error message that will be emitted if MALLOC or CALLOC
 *         fails.
 */
#define ERROR_MALLOC_MESSAGE "Insufficient memory"
/*! \def ERROR_REALLOC_MESSAGE
 *  \brief This is the error message that will be emitted if REALLOC fails.
 */
#define ERROR_REALLOC_MESSAGE "Reallocation failed"

/*! \def MALLOC
 *  \brief A convenience wrapper around malloc with error checking and a
 *         simpler calling method.
 *  \param p the pointer to the memory after allocation
 *  \param n the number of elements to allocate memory for, where each element
 *           is the size of the dereferenced type of \a p
 *
 * Examples:
 *
 * `float *var = malloc(10 * sizeof(float));` is equivalent to
 * `float *var; MALLOC(var, 10);`
 *
 * `int **var = malloc(25 * sizeof(int *));` is equivalent to
 * `int **var; MALLOC(var, 25);`
 */
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

/*! \def CALLOC
 *  \brief A convenience wrapper around calloc with error checking and a
 *         simpler calling method.
 *  \param p the pointer to the memory after allocation
 *  \param n the number of elements to allocate memory for, where each element
 *           is the size of the dereferenced type of \a p
 *
 * Examples:
 *
 * `float *var = calloc(10 * sizeof(float));` is equivalent to
 * `float *var; CALLOC(var, 10);`
 *
 * `double **var = calloc(25 * sizeof(double *));` is equivalent to
 * `double **var; CALLOC(var, 25);`
 */
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

/*! \def REALLOC
 *  \brief A convenience wrapper around realloc with error checking
 *         and a simpler calling method
 *  \param p the pointer to the memory that is to be reallocated
 *  \param n the new number of elements to allocate memory for, where each
 *           element is the size of the dereferenced type of \a p
 *
 * Examples:
 *
 * `float *var = malloc(10 * sizeof(float)); var = realloc(var, 20 * sizeof(float));`
 * is equivalent to `float *var; MALLOC(var, 10); REALLOC(var, 20);`
 */
#define REALLOC(p, n)					  \
  do                                                      \
    {                                                     \
      if ( !( (p) = realloc((p), sizeof(*(p)) * (n)) ) )  \
        {                                                 \
          perror(ERROR_REALLOC_MESSAGE);                  \
          exit(EXIT_FAILURE);                             \
        }                                                 \
    }                                                     \
  while(0)

/*! \def FREE
 *  \brief A convenience wrapper around free which resets the memory pointer
 *         to NULL after memory is released
 *  \param p the pointer to the memory that is to be freed; will be NULL after
 *           this call
 */
#define FREE(p)					\
  do						\
    {						\
      free(p);					\
      p = NULL;					\
    }						\
while(0)

/*! \def ARRAY_APPEND
 *  \brief Change the size of an array and set the last value in the new
 *         array
 *  \param p the pointer to the memory representing the array
 *  \param n the number of new elements that the array will have after this call
 *  \param v the value of the last element in the array
 *
 * This convenience macro is designed to be used like the following block:
 *
 *     float *array = NULL; int n_array = 0;
 *     ARRAY_APPEND(array, ++n, 2); // array will be [ 2 ], n = 1
 *     ARRAY_APPEND(array, ++n, 16); // array will be [ 2, 16 ], n = 2
 */
#define ARRAY_APPEND(p, n, v) REALLOC(p, n); p[n - 1] = v

/*! \def STRUCTCOPY
 *  \brief Copy the value of some structure member from one instance
 *         to another
 *  \param s the source structure instance, the parameter value will come
 *           from here; must be a pointer
 *  \param d the destination structure instance; must be a pointer
 *  \param p the parameter to copy from the source to the destination
 *
 * This convenience macro is designed to make the code more readable,
 * as seen in the following example.
 *
 *     struct mystruct { int a; float b; double c; };
 *     struct mystruct *m1, *m2;
 *     MALLOC(m1, 1); MALLOC(m2, 1);
 *     m1->a = 1; m1->b = 2.0; m1->c = 3.0;
 *     STRUCTCOPY(m1, m2, a); STRUCTCOPY(m1, m2, b); STRUCTCOPY(m1, m2, c);
 *     // m2 is now { a = 1; b = 2.0; c = 3.0; }
 */
#define STRUCTCOPY(s, d, p) d->p = s->p

