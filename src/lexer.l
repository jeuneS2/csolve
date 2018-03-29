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

%{

#include "csolve.h"
#include "parser.h"

int fileno(FILE *);
static int input(void) __attribute__((unused));
static void yyunput(int c, char *buf_ptr) __attribute__((unused));

uint32_t line_count = 1;

#define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno;

%}

%option yylineno

BNUM (0b[0-1]+)
ONUM (0[0-7]*)
DNUM ([1-9][0-9]*)
XNUM (0x[0-9a-fA-F]+)
SYM  ([_a-zA-Z@][_a-zA-Z0-9@]*)

%%

"ANY"  return ANY;
"ALL"  return ALL;
"MAX"  return MAX;
"MIN"  return MIN;

"="    return '=';
"!="   return NEQ;
"<"    return '<';
"<="   return LEQ;
">"    return '>';
">="   return GEQ;
"-"    return '-';
"+"    return '+';
"*"    return '*';
"!"    return '!';
"&"    return '&';
"|"    return '|';

"("    return '(';
")"    return ')';

";"    return ';';

{BNUM} {
          yylval.intval = strtol(yytext+2, NULL, 2);
          return NUM;
       }

{ONUM} {
          yylval.intval = strtol(yytext, NULL, 8);
          return NUM;
       }

{DNUM} {
          yylval.intval = strtol(yytext, NULL, 10);
          return NUM;
       }

{XNUM} {
          yylval.intval = strtol(yytext+2, NULL, 16);
          return NUM;
       }

{SYM}  {
          yylval.strval = yytext;
          return IDENT;
       }

#.*        { /* ignore comments */ }
[ \t\r]+   { /* ignore whitespace */ }
\n         { /* ignore lin-breaks */ }

. {
    fprintf(stderr, "ERROR: invalid input `%c' in line %u\n", yytext[0], yylineno);
    exit(EXIT_FAILURE);
  }

%%

int yywrap()
{
  return 1;
}
