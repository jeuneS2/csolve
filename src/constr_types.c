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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "csolve.h"

#define CONSTR_TYPES(UPNAME, NAME, OP)                                  \
  const struct constr_type_t CONSTR_ ## UPNAME =                        \
    { eval_ ## NAME, propagate_ ## NAME, normal_ ## NAME, OP_ ## UPNAME };
CONSTR_TYPE_LIST(CONSTR_TYPES)
