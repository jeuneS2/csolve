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
#include "parser.h"
#include "version.h"

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

// macros to convert defines to strings
#define STR(X) #X
#define STRVAL(X) STR(X)

// definitions of suffixes
#define KILO 1024
#define MEGA (KILO * KILO)
#define GIGA (KILO * KILO * KILO)

// declarations for functions provided by the lexer
void yyset_in(FILE *);
FILE *yyget_in(void);
int yylex_destroy(void);

// maximum number of options, enough to accept one byte
#define OPTIONS_MAX 0x100

// list of all options
#define OPTION_LIST(F)                                                  \
  F('b', "binds", required_argument, "b:",                              \
    { bind_init(parse_size(optarg)); },                                 \
    { bind_init(BIND_STACK_SIZE_DEFAULT); },                            \
    "-b --binds <size>           maximum number of binds (default: %d)\n", \
    BIND_STACK_SIZE_DEFAULT)                                            \
                                                                        \
  F('c', "conflicts", required_argument, "c:",                          \
    { strategy_create_conflicts_init(parse_bool(optarg)); },            \
    { strategy_create_conflicts_init(STRATEGY_CREATE_CONFLICTS_DEFAULT); }, \
    "-c --conflicts <bool>       create conflict clauses (default: %s)\n", \
    STRATEGY_CREATE_CONFLICTS_DEFAULT ? STR(true) : STR(false))         \
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
  F('M', "confl-memory", required_argument, "M:",                       \
    { conflict_alloc_init(parse_size(optarg)); },                       \
    { conflict_alloc_init(CONFLICT_ALLOC_STACK_SIZE_DEFAULT); },        \
    "-M --confl-memory <size>    conflict allocation stack size in bytes (default: %d)\n", \
    CONFLICT_ALLOC_STACK_SIZE_DEFAULT)                                           \
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
  F('s', "stats-freq", required_argument, "s:",                         \
    { stats_frequency_init(parse_int(optarg)); },                       \
    { stats_frequency_init(STATS_FREQUENCY_DEFAULT); },                 \
    "-s --stats-freq <int>       statistics printing frequency (default: %u), set to 0 to disable\n", \
    STATS_FREQUENCY_DEFAULT)                                            \
                                                                        \
  F('t', "time", required_argument, "t:",                               \
    { timeout_init(parse_int(optarg)); },                               \
    { timeout_init(TIME_MAX_DEFAULT); },                                \
    "-t --time <int>             maximum solving time in seconds (default: %u), set to 0 to disable\n", \
    TIME_MAX_DEFAULT)                                                   \
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

// return the name of the program
const char *main_name() {
  return _main_name;
}

// print the program version
static void print_version(FILE *file) {
  fprintf(file, "%s\n", VERSION);
  fprintf(file, "%s\n", COPYRIGHT);
  fprintf(file, "This is free software; see the source for copying conditions.  There is NO\n"
                "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
}

// print the basic program usage
static void print_usage(FILE *file) {
  fprintf(file, "Usage: %s [<options>] [<file>]\n", _main_name);
}

// macro to print the help text from an entry in the option list
#define OPTION_HELP(VAL, LONG, ARG, SHORT, MATCH, DEFAULT, ...) \
  fprintf(file, "  " __VA_ARGS__);

// print the full help text
static void print_help(FILE *file) {
  print_usage(file);
  fprintf(file, "Options:\n");
  OPTION_LIST(OPTION_HELP)
}

// parse a string to a boolean
static bool parse_bool(const char *str) {
  if (strcmp(str, STR(true)) == 0) {
    return true;
  }
  if (strcmp(str, STR(false)) == 0) {
    return false;
  }

  // die if the string could not be parsed
  print_fatal(ERROR_MSG_INVALID_BOOL_ARG, str);
  return false;
}

// parse a string to an integer
static int32_t parse_int(const char *str) {
  char *endptr;
  int32_t size = strtol(str, &endptr, 0);

  if (endptr[0] == '\0') {
    return size;
  }

  // die if the string could not be parsed
  print_fatal(ERROR_MSG_INVALID_INT_ARG, str);
  return 0;
}

// parse a string to a variable ordering
static enum order_t parse_order(const char *str) {
  if (strcmp(str, "none") == 0) {
    return ORDER_NONE;
  }
  if (strcmp(str, "smallest-domain") == 0) {
    return ORDER_SMALLEST_DOMAIN;
  }
  if (strcmp(str, "largest-domain") == 0) {
    return ORDER_LARGEST_DOMAIN;
  }
  if (strcmp(str, "smallest-value") == 0) {
    return ORDER_SMALLEST_VALUE;
  }
  if (strcmp(str, "largest-value") == 0) {
    return ORDER_LARGEST_VALUE;
  }

  // die if the string could not be parsed
  print_fatal(ERROR_MSG_INVALID_ORDER_ARG, str);
  return ORDER_NONE;
}

// parse a string to a size (accepting an integer with a possible k/M/G suffix)
static size_t parse_size(const char *str) {
  char *endptr;
  size_t size = strtoll(str, &endptr, 0);

  // check if plain integer
  if (endptr[0] == '\0') {
    return size;
  }
  // check if integer wit k/M/G suffix, ignoring case
  if (toupper(endptr[0]) == 'K' && endptr[1] == '\0') {
    return size * KILO;
  }
  if (toupper(endptr[0]) == 'M' && endptr[1] == '\0') {
    return size * MEGA;
  }
  if (toupper(endptr[0]) == 'G' && endptr[1] == '\0') {
    return size * GIGA;
  }

  // die if the string could not be parsed
  print_fatal(ERROR_MSG_INVALID_SIZE_ARG, str);
  return 0;
}

// macro to extract the short option from an entry in the option list
#define OPTION_SHORT(VAL, LONG, ARG, SHORT, MATCH, DEFAULT, ...)       \
  SHORT

// macro to extract the long option from an entry in the option list
#define OPTION_LONG(VAL, LONG, ARG, SHORT, MATCH, DEFAULT, ...)        \
  { LONG, ARG, 0, VAL },

// macro to perform the matching action for an option
#define OPTION_MATCH(VAL, LONG, ARG, SHORT, MATCH, DEFAULT, ...)       \
  case VAL: { MATCH; } break;

// macro to perform the default action for an option
#define OPTION_DEFAULT(VAL, LONG, ARG, SHORT, MATCH, DEFAULT, ...)     \
  if (!seen[VAL]) { DEFAULT; }

// parse all program options
static void parse_options(int argc, char **argv) {

  // option arrays for getopt
  static const char *short_options = OPTION_LIST(OPTION_SHORT);
  static struct option long_options[] = {
    OPTION_LIST(OPTION_LONG)
    {0, 0, 0, 0 }
  };

  // initialize array of previously seen options
  bool seen [OPTIONS_MAX];
  for (size_t i = 0; i < sizeof(seen)/sizeof(seen[0]); i++) {
    seen[i] = false;
  }

  // iterate over all command-line options
  int c;
  while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
    // allow options only once
    if (seen[c % OPTIONS_MAX]) {
      print_usage(stderr);
      exit(EXIT_FAILURE);
    }
    seen[c % OPTIONS_MAX] = true;

    // perform actions when matching an option
    switch(c) {
      OPTION_LIST(OPTION_MATCH);
    default:
      print_usage(stderr);
      exit(EXIT_FAILURE);
    }
  }

  // check that there is at most one additional argument
  if (optind < argc-1) {
    print_usage(stderr);
    exit(EXIT_FAILURE);
  }

  // perform default actions for options not seen previously
  OPTION_LIST(OPTION_DEFAULT);

  FILE *in = stdin;

  // open file as needed, otherwise keep stdin
  if (optind == argc-1 && strcmp(argv[optind], "-") != 0) {
    in = fopen(argv[optind], "r");
    if (in == NULL) {
      print_fatal("%s: %s", argv[optind], strerror(errno));
    }
  }

  yyset_in(in);
}

// clean up resource allocations
static void cleanup() {
  bind_free();
  patch_free();
  alloc_free();
  conflict_alloc_free();
  strategy_var_order_free();
  fclose(yyget_in());
  yylex_destroy();
}

// the main function
int main(int argc, char **argv) {
  _main_name = argv[0];

  parse_options(argc, argv);

  yyparse();

  cleanup();

  return EXIT_SUCCESS;
}
