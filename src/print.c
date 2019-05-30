/* Copyright 2018-2019 Wolfgang Puffitsch

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

#include "csolve.h"

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// print value
void print_val(FILE *file, const struct val_t val) {
  if (is_value(val)) {
    fprintf(file, " %d", get_lo(val));
  } else {
    fprintf(file, " [%d;%d]", get_lo(val), get_hi(val));
  }
}

// print expression
void print_constr(FILE *file, const struct constr_t *constr) {
  if (IS_TYPE(TERM, constr)) {
    print_val(file, constr->constr.term.val);
  } else if (IS_TYPE(WAND, constr)) {
    for (size_t i = 0; i < constr->constr.wand.length; i++) {
      print_constr(file, constr->constr.wand.elems[i].constr);
      fprintf(file, ";");
    }
  } else {
    fprintf(file, " (%c", constr->type->op);
    print_constr(file, constr->constr.expr.l);
    if (constr->constr.expr.r != NULL) {
      print_constr(file, constr->constr.expr.r);
    }
    fprintf(file, ")");
  }
}

// print variable environment
void print_env(FILE *file, size_t size, struct env_t *env) {
  for (size_t i = 0; i < size; i++) {
    fprintf(file, "%s =", env[i].key);
    print_val(file, env[i].val->constr.term.val);
    fprintf(file, ", ");
  }
}

//print solution
void print_solution(FILE *file, size_t size, struct env_t *env) {
  fprintf(file, "SOLUTION: ");
  print_env(file, size, env);
  fprintf(file, "BEST: %d\n", objective_best());
}

// helper function to print errors
static void vprint_error(const char *fmt, va_list ap) {
    fprintf(stderr, "%s: error: ", main_name());
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

// print error message
void print_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprint_error(fmt, args);
    va_end(args);
}

// print error message and die
void print_fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprint_error(fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}
