/* Copyright 2018 Wolfgang Puffitsch

This file is part of CSolve.

CSolve is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

CSolve is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with CSolve.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "csolve.h"

#define STAT_VAR(NAME, TYPE, RESET_VAL, ...)    \
  TYPE NAME;

STAT_LIST(STAT_VAR)

#define STAT_RESET(NAME, TYPE, RESET_VAL, ...)  \
  stat_reset_ ## NAME();

void stats_init(void) {
  STAT_LIST(STAT_RESET)
}

#define STAT_PRINT(NAME, TYPE, RESET_VAL, ...)  \
  fprintf(file, __VA_ARGS__, NAME);

void stats_print(FILE *file) {
  STAT_LIST(STAT_PRINT)
}
