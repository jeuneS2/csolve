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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include "csolve.h"
#include "parser.h"
#include "version.h"

void yyset_in(FILE *);

const char *_main_name;

const char *main_name() {
  return _main_name;
}

void print_version(FILE *file) {
  fprintf(file, "%s\n", VERSION);
  fprintf(file, "%s\n", COPYRIGHT);
  fprintf(file, "This is free software; see the source for copying conditions.  There is NO\n"
                "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
}

void print_usage(FILE *file) {
  fprintf(file, "Usage: %s [-v] [-h] [-b <binds>] [-m <size>] [<file>]\n", _main_name);
}

void print_help(FILE *file) {
  print_usage(file);
  fprintf(file, "Options:\n");
  fprintf(file, "  -b --binds <binds>   maximum number of binds (default: %d)\n", BIND_STACK_SIZE_DEFAULT);
  fprintf(file, "  -m --memory <size>   allocation stack size (default: %d)\n", ALLOC_STACK_SIZE_DEFAULT);
  fprintf(file, "  -h --help            show this message and exit\n");
  fprintf(file, "  -v --version         print version and exit\n");
}

size_t parse_size(const char *str) {
  char *endptr;
  size_t size = strtoll(str, &endptr, 0);
  if (endptr[0] == '\0') {
    return size;
  } else if ((endptr[0] == 'k' || endptr[0] == 'K') && endptr[1] == '\0') {
    return size * 1024;
  } else if ((endptr[0] == 'm' || endptr[0] == 'M') && endptr[1] == '\0') {
    return size * 1024 * 1024;
  } else if ((endptr[0] == 'g' || endptr[0] == 'G') && endptr[1] == '\0') {
    return size * 1024 * 1024 * 1024;
  } else {
    print_error(ERROR_MSG_INVALID_SIZE_ARG, str);
    exit(EXIT_FAILURE);
  }
}

void parse_options(int argc, char **argv) {

  static const char *short_options = "b:m:hv";
  static struct option long_options[] = {
    {"binds",   required_argument, 0, 'b' },
    {"memory",  required_argument, 0, 'm' },
    {"help",    no_argument,       0, 'h' },
    {"version", no_argument,       0, 'v' },
    {0,         0,                 0, 0   }
  };

  bool seen_b = false;
  bool seen_m = false;

  int c;
  while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
    switch(c) {
    case 'b':
      if (seen_b) {
        print_usage(stderr);
        exit(EXIT_SUCCESS);
      } else {
        size_t max_binds = parse_size(optarg);
        bind_init(max_binds);
        seen_b = true;
        break;
      }
    case 'm':
      if (seen_m) {
        print_usage(stderr);
        exit(EXIT_SUCCESS);
      } else {
        size_t max_memory = parse_size(optarg);
        alloc_init(max_memory);
        seen_m = true;
        break;
      }
    case 'h':
      print_help(stdout);
      exit(EXIT_SUCCESS);
    case 'v':
      print_version(stdout);
      exit(EXIT_SUCCESS);
    default:
      print_usage(stderr);
      exit(EXIT_FAILURE);
    }
  }

  if (optind < argc-1) {
    print_usage(stderr);
    exit(EXIT_FAILURE);
  }

  if (!seen_b) {
    bind_init(BIND_STACK_SIZE_DEFAULT);
  }
  if (!seen_m) {
    alloc_init(ALLOC_STACK_SIZE_DEFAULT);
  }

  FILE *in = stdin;

  if (optind == argc-1 && strcmp(argv[optind], "-") != 0) {
    in = fopen(argv[optind], "r");
    if (in == NULL) {
      print_error("%s: %s", argv[optind], strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  yyset_in(in);
}


int main(int argc, char **argv) {
  _main_name = argv[0];

  parse_options(argc, argv);

  yyparse();

  return EXIT_SUCCESS;
}
