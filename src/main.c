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
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "csolve.h"
#include "parser.h"
#include "version.h"

#define STR(X) #X
#define STRVAL(X) STR(X)

#define KILO 1024
#define MEGA (KILO * KILO)
#define GIGA (KILO * KILO * KILO)

void yyset_in(FILE *);

#define OPTIONS_MAX 0x100

#define OPTION_LIST(F)                                                  \
  F('b', "binds", required_argument, "b:",                              \
    { bind_init(parse_size(optarg)); },                                 \
    { bind_init(BIND_STACK_SIZE_DEFAULT); },                            \
    "-b --binds <size>           maximum number of binds (default: %d)\n", \
    BIND_STACK_SIZE_DEFAULT)                                            \
                                                                        \
  F('f', "prefer-failing", required_argument, "f:",                     \
    { strategy_prefer_failing_init(parse_bool(optarg)); },              \
    { strategy_prefer_failing_init(STRATEGY_PREFER_FAILING_DEFAULT); }, \
    "-f --prefer-failing <bool>  prefer failing variables when ordering (default: %s)\n", \
    STRATEGY_PREFER_FAILING_DEFAULT ? STR(true) : STR(false))           \
                                                                        \
  F('h', "help", no_argument, "h",                                      \
    { print_help(stdout); exit(EXIT_SUCCESS); }, ,                      \
    "-h --help                   show this message and exit\n")         \
                                                                        \
  F('j', "jobs", required_argument, "j:",                               \
    { shared_init(parse_int(optarg)); },                                \
    { shared_init(WORKERS_MAX_DEFAULT); },                              \
    "-j --jobs <int>             number of jobs to run simultaneously (default: %u)\n", \
    WORKERS_MAX_DEFAULT)                                                \
                                                                        \
  F('m', "memory", required_argument, "m:",                             \
    { alloc_init(parse_size(optarg)); },                                \
    { alloc_init(ALLOC_STACK_SIZE_DEFAULT); },                          \
    "-m --memory <size>          allocation stack size in bytes (default: %d)\n", \
    ALLOC_STACK_SIZE_DEFAULT)                                           \
                                                                        \
  F('o', "order", required_argument, "o:",                              \
    { strategy_order_init(parse_order(optarg)); },                      \
    { strategy_order_init(STRATEGY_ORDER_DEFAULT); },                   \
    "-o --order <order>          how to order variables during solving (default: %s)\n", \
    STRVAL(STRATEGY_ORDER_DEFAULT))                                     \
                                                                        \
  F('p', "patches", required_argument, "p:",                            \
    { patch_init(parse_size(optarg)); },                                \
    { patch_init(PATCH_STACK_SIZE_DEFAULT); },                          \
    "-p --patches <size>         maximum number of patches (default: %d)\n", \
    PATCH_STACK_SIZE_DEFAULT)                                           \
                                                                        \
  F('r', "restart-freq", required_argument, "r:",                       \
    { strategy_restart_frequency_init(parse_int(optarg)); },            \
    { strategy_restart_frequency_init(STRATEGY_RESTART_FREQUENCY_DEFAULT); }, \
    "-r --restart-freq <int>     restart frequency when looking for any solution (default: %u), set to 0 to disable\n", \
    STRATEGY_RESTART_FREQUENCY_DEFAULT)                                 \
                                                                        \
  F('v', "version", no_argument, "v",                                   \
    { print_version(stdout); exit(EXIT_SUCCESS); }, ,                   \
    "-v --version                print version and exit\n")             \
                                                                        \
  F('w', "weighten", required_argument, "w:",                           \
    { strategy_compute_weights_init(parse_bool(optarg)); },             \
    { strategy_compute_weights_init(STRATEGY_COMPUTE_WEIGHTS_DEFAULT); }, \
    "-w --weighten <bool>        compute weights of variables for initial order (default: %s)\n", \
    STRATEGY_COMPUTE_WEIGHTS_DEFAULT ? STR(true) : STR(false))          \


static const char *_main_name;

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
  fprintf(file, "Usage: %s [<options>] [<file>]\n", _main_name);
}

#define OPTION_HELP(VAL, LONG, ARG, SHORT, MATCH, DEFAULT, ...) \
  fprintf(file, "  " __VA_ARGS__);
  
void print_help(FILE *file) {
  print_usage(file);
  fprintf(file, "Options:\n");
  OPTION_LIST(OPTION_HELP)
}

bool parse_bool(const char *str) {
  if (strcmp(str, STR(true)) == 0) {
    return true;
  } else if (strcmp(str, STR(false)) == 0) {
    return false;
  } else {
    print_fatal(ERROR_MSG_INVALID_BOOL_ARG, str);
  }
  return false;
}

int32_t parse_int(const char *str) {
  char *endptr;
  int32_t size = strtol(str, &endptr, 0);
  if (endptr[0] == '\0') {
    return size;
  } else {
    print_fatal(ERROR_MSG_INVALID_INT_ARG, str);
  }
  return 0;
}

enum order_t parse_order(const char *str) {
  if (strcmp(str, "none") == 0) {
    return ORDER_NONE;
  } else if (strcmp(str, "smallest-domain") == 0) {
    return ORDER_SMALLEST_DOMAIN;
  } else if (strcmp(str, "largest-domain") == 0) {
    return ORDER_LARGEST_DOMAIN;
  } else if (strcmp(str, "smallest-value") == 0) {
    return ORDER_SMALLEST_VALUE;
  } else if (strcmp(str, "largest-value") == 0) {
    return ORDER_LARGEST_VALUE;
  } else {
    print_fatal(ERROR_MSG_INVALID_ORDER_ARG, str);
  }
  return ORDER_NONE;
}

size_t parse_size(const char *str) {
  char *endptr;
  size_t size = strtoll(str, &endptr, 0);
  if (endptr[0] == '\0') {
    return size;
  } else if ((endptr[0] == 'k' || endptr[0] == 'K') && endptr[1] == '\0') {
    return size * KILO;
  } else if ((endptr[0] == 'm' || endptr[0] == 'M') && endptr[1] == '\0') {
    return size * MEGA;
  } else if ((endptr[0] == 'g' || endptr[0] == 'G') && endptr[1] == '\0') {
    return size * GIGA;
  } else {
    print_fatal(ERROR_MSG_INVALID_SIZE_ARG, str);
  }
  return 0;
}


#define OPTION_SHORT(VAL, LONG, ARG, SHORT, MATCH, DEFAULT, ...)       \
  SHORT

#define OPTION_LONG(VAL, LONG, ARG, SHORT, MATCH, DEFAULT, ...)        \
  { LONG, ARG, 0, VAL },

#define OPTION_MATCH(VAL, LONG, ARG, SHORT, MATCH, DEFAULT, ...)       \
  case VAL: { MATCH; } break;

#define OPTION_DEFAULT(VAL, LONG, ARG, SHORT, MATCH, DEFAULT, ...)     \
  if (!seen[VAL]) { DEFAULT; }

void parse_options(int argc, char **argv) {

  static const char *short_options = OPTION_LIST(OPTION_SHORT);
  static struct option long_options[] = {
    OPTION_LIST(OPTION_LONG)
    {0, 0, 0, 0 }
  };
  
  bool seen [OPTIONS_MAX];
  for (size_t i = 0; i < sizeof(seen)/sizeof(seen[0]); i++) {
    seen[i] = false;
  }

  int c;
  while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
    if (seen[c % OPTIONS_MAX]) {
      print_usage(stderr);
      exit(EXIT_FAILURE);
    }
    seen[c % OPTIONS_MAX] = true;

    switch(c) {
      OPTION_LIST(OPTION_MATCH);
    default:
      print_usage(stderr);
      exit(EXIT_FAILURE);
    }
  }

  if (optind < argc-1) {
    print_usage(stderr);
    exit(EXIT_FAILURE);
  }

  OPTION_LIST(OPTION_DEFAULT);

  FILE *in = stdin;

  if (optind == argc-1 && strcmp(argv[optind], "-") != 0) {
    in = fopen(argv[optind], "r");
    if (in == NULL) {
      print_fatal("%s: %s", argv[optind], strerror(errno));
    }
  }

  yyset_in(in);
}

void cleanup() {
  bind_free();
  patch_free();
  alloc_free();
}

int main(int argc, char **argv) {
  _main_name = argv[0];

  parse_options(argc, argv);

  yyparse();

  cleanup();

  return EXIT_SUCCESS;
}
