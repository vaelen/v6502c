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

 #include <addrlist.h>

 /** Initialize an address range list. */
void init_address_range_list(address_range_list *list) {
  if (list == NULL) return;
  list->first = NULL;
  list->last = NULL;
}

/** 
 * Add an address range to an address range list.
 * The list will be expanded as needed.
 * Overlapping or adjacent ranges will be merged.
 */
void add_address_range(address_range_list *list, address_range ar) {
  address_range_node *new_node = NULL;
  address_range_node *current = NULL;

  if (list == NULL) return;
  current = list->first;
  while (current != NULL) {
    if (ar.end < current->range.start - 1) {
      /* New range is before current range, insert here */
      new_node = (address_range_node *) malloc(sizeof(address_range_node));
      if (new_node == NULL) {
        fprintf(stderr, "Error: Out of memory\n");
        return;
      }
      new_node->range = ar;
      new_node->next = current;
      new_node->prev = current->prev;
      if (current->prev != NULL) {
        current->prev->next = new_node;
      } else {
        list->first = new_node;
      }
      current->prev = new_node;
      return;
    } else if (ar.start > current->range.end + 1) {
      /* New range is after current range, continue */
      current = current->next;
    } else {
      /* Ranges overlap or are adjacent, merge */
      if (ar.start < current->range.start) {
        current->range.start = ar.start;
      }
      if (ar.end > current->range.end) {
        current->range.end = ar.end;
      }
      return;
    }
  }
  /* Add to end of list */
  new_node = (address_range_node *) malloc(sizeof(address_range_node));
  if (new_node == NULL) {
    fprintf(stderr, "Error: Out of memory\n");
    return;
  }
  new_node->range = ar;
  new_node->next = NULL;
  new_node->prev = list->last;
  if (list->last != NULL) {
    list->last->next = new_node;
  } else {
    list->first = new_node;
  }
  list->last = new_node;
}

/** 
 * Remove an address range from an address range list.
 * The list will be adjusted as needed.
 * Ranges that are partially overlapped will be split.
 */
void remove_address_range(address_range_list *list, address_range ar) {
  address_range_node *to_delete = NULL;
  address_range_node *current = NULL;

  if (list == NULL) return;
  current = list->first;
  while (current != NULL) {
    if (ar.end < current->range.start) {
      /* No more overlaps possible */
      return;
    } else if (ar.start > current->range.end) {
      /* No overlap, continue */
      current = current->next;
    } else {
      /* Overlap found */
      if (ar.start <= current->range.start && ar.end >= current->range.end) {
        /* Remove entire current range */
        to_delete = current;
        current = current->next;
        if (to_delete->prev != NULL) {
          to_delete->prev->next = to_delete->next;
        } else {
          list->first = to_delete->next;
        }
        if (to_delete->next != NULL) {
          to_delete->next->prev = to_delete->prev;
        } else {
          list->last = to_delete->prev;
        }
        free(to_delete);
      } else if (ar.start > current->range.start && ar.end < current->range.end) {
        /* Split current range */
        address_range_node *new_node = (address_range_node *) malloc(sizeof(address_range_node));
        if (new_node == NULL) {
          fprintf(stderr, "Error: Out of memory\n");
          return;
        }
        new_node->range.start = ar.end + 1;
        new_node->range.end = current->range.end;
        new_node->next = current->next;
        new_node->prev = current;
        if (current->next != NULL) {
          current->next->prev = new_node;
        } else {
          list->last = new_node;
        }
        current->next = new_node;
        current->range.end = ar.start - 1;
        return;
      } else if (ar.start <= current->range.start) {
        /* Adjust start of current range */
        current->range.start = ar.end + 1;
        current = current->next;
      } else if (ar.end >= current->range.end) {
        /* Adjust end of current range */
        current->range.end = ar.start - 1;
        current = current->next;
      }
    }
  }
}

/** Check if an address is within a given address range. */
bool is_address_in_range(address_range ar, address a) {
  return (a >= ar.start && a <= ar.end);
}

/** Check if an address is within any range in an address range list. */
bool is_address_in_range_list(address_range_list *list, address a) {
  address_range_node *current = NULL;

  if (list == NULL) return FALSE;
  current = list->first;
  while (current != NULL) {
    if (is_address_in_range(current->range, a)) {
      return TRUE;
    }
    current = current->next;
  }
  return FALSE;
}

/** Remove all addresses in an address range list and free related memory. */
void clear_address_range_list(address_range_list *list) {
  address_range_node *current = NULL;
  address_range_node *next = NULL;

  if (list == NULL) return;
  current = list->first;
  while (current != NULL) {
    next = current->next;
    free(current);
    current = next;
  }
  list->first = NULL;
  list->last = NULL;
}
