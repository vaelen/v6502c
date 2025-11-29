#ifndef _ADDRLIST_H_
#define _ADDRLIST_H_

/**
 * Copyright (c) 2025 Andrew C. Young
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "vtypes.h"

typedef struct addrr {
  address start;
  address end;
} address_range;

typedef struct addrrnode {
  address_range range;
  struct addrrnode *next;
  struct addrrnode *prev;
} address_range_node;

/**
 * A list of address ranges.
 * The list is implemented as a doubly-linked list.
 * The list is kept sorted by start address.
 * Ranges may not overlap.
 */
typedef struct addrrlist {
  address_range_node *first;
  address_range_node *last;
} address_range_list;

void init_address_range_list(address_range_list *list);
void add_address_range(address_range_list *list, address_range ar);
void remove_address_range(address_range_list *list, address_range ar);
bool is_address_in_range(address_range ar, address a);
bool is_address_in_range_list(address_range_list *list, address a);
void clear_address_range_list(address_range_list *list);

#endif /* _ADDRLIST_H_ */
