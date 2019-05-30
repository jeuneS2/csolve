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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// negate a value, with saturation handling
domain_t neg(const domain_t a) {
  if (a == DOMAIN_MIN) {
    return DOMAIN_MAX;
  }
  if (a == DOMAIN_MAX) {
    return DOMAIN_MIN;
  }
  return -a;
}

// add two values, with saturation handling
domain_t add(const domain_t a, const domain_t b) {
  if (a == DOMAIN_MIN || b == DOMAIN_MIN) {
    return DOMAIN_MIN;
  }
  if (a == DOMAIN_MAX || b == DOMAIN_MAX) {
    return DOMAIN_MAX;
  }

  domain_t c = (domain_t)((udomain_t)a + (udomain_t)b);
  if (((a ^ b) & DOMAIN_MIN) == 0 && ((c ^ a) & DOMAIN_MIN) != 0) {
    return a < 0 ? DOMAIN_MIN : DOMAIN_MAX;
  }
  return c;
}

// multiply two values, with saturation handling
domain_t mul(const domain_t a, const domain_t b) {
  if (a == DOMAIN_MIN) {
    return b < 0 ? DOMAIN_MAX : DOMAIN_MIN;
  }
  if (b == DOMAIN_MIN) {
    return a < 0 ? DOMAIN_MAX : DOMAIN_MIN;
  }
  if (a == DOMAIN_MAX) {
    return b < 0 ? DOMAIN_MIN : DOMAIN_MAX;
  }
  if (b == DOMAIN_MAX) {
    return a < 0 ? DOMAIN_MIN : DOMAIN_MAX;
  }

  ddomain_t c = (ddomain_t)a * (ddomain_t)b;
  domain_t hi = c >> DOMAIN_BITS;
  domain_t lo = c;
  if (hi != (lo >> (DOMAIN_BITS-1))) {
    return hi < 0 ? DOMAIN_MIN : DOMAIN_MAX;
  }
  return lo;
}

// take minimum of two values
domain_t min(const domain_t a, const domain_t b) {
  return a < b ? a : b;
}

// take maximum of two values
domain_t max(const domain_t a, const domain_t b) {
  return a > b ? a : b;
}
