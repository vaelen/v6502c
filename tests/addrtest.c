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
 * Comprehensive tests for address range list functions
 */

#include <stdio.h>
#include "addrlist.h"

/* ANSI color codes for terminal output */
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_RESET "\033[0m"

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

/* Test framework functions */
static void pass(const char *test_name) {
    printf("Testing %s... " COLOR_GREEN "passed" COLOR_RESET "\n", test_name);
    tests_passed++;
}

static void fail(const char *test_name, const char *reason) {
    printf("Testing %s... " COLOR_RED "failed" COLOR_RESET ": %s\n", test_name, reason);
    tests_failed++;
}

/* Helper to count nodes in list */
static int count_nodes(address_range_list *list) {
    int count = 0;
    address_range_node *current;
    if (list == NULL) return 0;
    current = list->first;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}

/* Helper to create an address range */
static address_range make_range(address start, address end) {
    address_range ar;
    ar.start = start;
    ar.end = end;
    return ar;
}

/*
 * ============================================================================
 * Initialization Tests
 * ============================================================================
 */

static void test_init_list(void) {
    address_range_list list;

    init_address_range_list(&list);

    if (list.first != NULL) {
        fail("init list", "first should be NULL");
        return;
    }
    if (list.last != NULL) {
        fail("init list", "last should be NULL");
        return;
    }

    /* Test with NULL (should not crash) */
    init_address_range_list(NULL);

    pass("init list");
}

/*
 * ============================================================================
 * Add Range Tests
 * ============================================================================
 */

static void test_add_single_range(void) {
    address_range_list list;
    address_range ar;

    init_address_range_list(&list);

    ar = make_range(0x1000, 0x1FFF);
    add_address_range(&list, ar);

    if (list.first == NULL) {
        fail("add single range", "first should not be NULL");
        return;
    }
    if (list.last == NULL) {
        fail("add single range", "last should not be NULL");
        return;
    }
    if (list.first != list.last) {
        fail("add single range", "first and last should be same for single node");
        clear_address_range_list(&list);
        return;
    }
    if (list.first->range.start != 0x1000) {
        fail("add single range", "start address incorrect");
        clear_address_range_list(&list);
        return;
    }
    if (list.first->range.end != 0x1FFF) {
        fail("add single range", "end address incorrect");
        clear_address_range_list(&list);
        return;
    }
    if (list.first->next != NULL || list.first->prev != NULL) {
        fail("add single range", "single node should have NULL prev/next");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("add single range");
}

static void test_add_range_null_list(void) {
    address_range ar;

    ar = make_range(0x1000, 0x1FFF);
    /* Should not crash */
    add_address_range(NULL, ar);

    pass("add range NULL list");
}

static void test_add_disjoint_ranges_ascending(void) {
    address_range_list list;
    address_range_node *node;

    init_address_range_list(&list);

    add_address_range(&list, make_range(0x1000, 0x1FFF));
    add_address_range(&list, make_range(0x3000, 0x3FFF));
    add_address_range(&list, make_range(0x5000, 0x5FFF));

    if (count_nodes(&list) != 3) {
        fail("add disjoint ranges ascending", "should have 3 nodes");
        clear_address_range_list(&list);
        return;
    }

    /* Verify order */
    node = list.first;
    if (node->range.start != 0x1000 || node->range.end != 0x1FFF) {
        fail("add disjoint ranges ascending", "first node incorrect");
        clear_address_range_list(&list);
        return;
    }
    node = node->next;
    if (node->range.start != 0x3000 || node->range.end != 0x3FFF) {
        fail("add disjoint ranges ascending", "second node incorrect");
        clear_address_range_list(&list);
        return;
    }
    node = node->next;
    if (node->range.start != 0x5000 || node->range.end != 0x5FFF) {
        fail("add disjoint ranges ascending", "third node incorrect");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("add disjoint ranges ascending");
}

static void test_add_disjoint_ranges_descending(void) {
    address_range_list list;
    address_range_node *node;

    init_address_range_list(&list);

    add_address_range(&list, make_range(0x5000, 0x5FFF));
    add_address_range(&list, make_range(0x3000, 0x3FFF));
    add_address_range(&list, make_range(0x1000, 0x1FFF));

    if (count_nodes(&list) != 3) {
        fail("add disjoint ranges descending", "should have 3 nodes");
        clear_address_range_list(&list);
        return;
    }

    /* Verify order (should still be sorted ascending) */
    node = list.first;
    if (node->range.start != 0x1000) {
        fail("add disjoint ranges descending", "first node should start at 0x1000");
        clear_address_range_list(&list);
        return;
    }
    node = node->next;
    if (node->range.start != 0x3000) {
        fail("add disjoint ranges descending", "second node should start at 0x3000");
        clear_address_range_list(&list);
        return;
    }
    node = node->next;
    if (node->range.start != 0x5000) {
        fail("add disjoint ranges descending", "third node should start at 0x5000");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("add disjoint ranges descending");
}

static void test_add_range_in_middle(void) {
    address_range_list list;
    address_range_node *node;

    init_address_range_list(&list);

    add_address_range(&list, make_range(0x1000, 0x1FFF));
    add_address_range(&list, make_range(0x5000, 0x5FFF));
    add_address_range(&list, make_range(0x3000, 0x3FFF));

    if (count_nodes(&list) != 3) {
        fail("add range in middle", "should have 3 nodes");
        clear_address_range_list(&list);
        return;
    }

    /* Verify middle node is in correct position */
    node = list.first->next;
    if (node->range.start != 0x3000 || node->range.end != 0x3FFF) {
        fail("add range in middle", "middle node incorrect");
        clear_address_range_list(&list);
        return;
    }
    if (node->prev != list.first) {
        fail("add range in middle", "prev pointer incorrect");
        clear_address_range_list(&list);
        return;
    }
    if (node->next != list.last) {
        fail("add range in middle", "next pointer incorrect");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("add range in middle");
}

static void test_add_adjacent_ranges_merge(void) {
    address_range_list list;

    init_address_range_list(&list);

    /* Add range, then add adjacent range */
    add_address_range(&list, make_range(0x1000, 0x1FFF));
    add_address_range(&list, make_range(0x2000, 0x2FFF));

    if (count_nodes(&list) != 1) {
        fail("add adjacent ranges merge", "adjacent ranges should merge");
        clear_address_range_list(&list);
        return;
    }

    if (list.first->range.start != 0x1000 || list.first->range.end != 0x2FFF) {
        fail("add adjacent ranges merge", "merged range incorrect");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("add adjacent ranges merge");
}

static void test_add_overlapping_ranges_merge(void) {
    address_range_list list;

    init_address_range_list(&list);

    add_address_range(&list, make_range(0x1000, 0x2000));
    add_address_range(&list, make_range(0x1800, 0x2800));

    if (count_nodes(&list) != 1) {
        fail("add overlapping ranges merge", "overlapping ranges should merge");
        clear_address_range_list(&list);
        return;
    }

    if (list.first->range.start != 0x1000 || list.first->range.end != 0x2800) {
        fail("add overlapping ranges merge", "merged range incorrect");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("add overlapping ranges merge");
}

static void test_add_contained_range(void) {
    address_range_list list;

    init_address_range_list(&list);

    add_address_range(&list, make_range(0x1000, 0x3000));
    add_address_range(&list, make_range(0x1500, 0x2500));

    if (count_nodes(&list) != 1) {
        fail("add contained range", "contained range should merge");
        clear_address_range_list(&list);
        return;
    }

    /* Original range should be unchanged */
    if (list.first->range.start != 0x1000 || list.first->range.end != 0x3000) {
        fail("add contained range", "range should be unchanged");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("add contained range");
}

static void test_add_containing_range(void) {
    address_range_list list;

    init_address_range_list(&list);

    add_address_range(&list, make_range(0x1500, 0x2500));
    add_address_range(&list, make_range(0x1000, 0x3000));

    if (count_nodes(&list) != 1) {
        fail("add containing range", "containing range should merge");
        clear_address_range_list(&list);
        return;
    }

    if (list.first->range.start != 0x1000 || list.first->range.end != 0x3000) {
        fail("add containing range", "range should be expanded");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("add containing range");
}

static void test_add_duplicate_range(void) {
    address_range_list list;

    init_address_range_list(&list);

    add_address_range(&list, make_range(0x1000, 0x2000));
    add_address_range(&list, make_range(0x1000, 0x2000));

    if (count_nodes(&list) != 1) {
        fail("add duplicate range", "duplicate should merge");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("add duplicate range");
}

/*
 * ============================================================================
 * Remove Range Tests
 * ============================================================================
 */

static void test_remove_range_null_list(void) {
    address_range ar;

    ar = make_range(0x1000, 0x1FFF);
    /* Should not crash */
    remove_address_range(NULL, ar);

    pass("remove range NULL list");
}

static void test_remove_range_empty_list(void) {
    address_range_list list;

    init_address_range_list(&list);

    /* Should not crash */
    remove_address_range(&list, make_range(0x1000, 0x2000));

    if (list.first != NULL || list.last != NULL) {
        fail("remove range empty list", "list should still be empty");
        return;
    }

    pass("remove range empty list");
}

static void test_remove_entire_range(void) {
    address_range_list list;

    init_address_range_list(&list);

    add_address_range(&list, make_range(0x1000, 0x2000));
    remove_address_range(&list, make_range(0x1000, 0x2000));

    if (count_nodes(&list) != 0) {
        fail("remove entire range", "list should be empty");
        clear_address_range_list(&list);
        return;
    }

    if (list.first != NULL || list.last != NULL) {
        fail("remove entire range", "first/last should be NULL");
        return;
    }

    pass("remove entire range");
}

static void test_remove_containing_range(void) {
    address_range_list list;

    init_address_range_list(&list);

    add_address_range(&list, make_range(0x1000, 0x2000));
    remove_address_range(&list, make_range(0x0000, 0x3000));

    if (count_nodes(&list) != 0) {
        fail("remove containing range", "list should be empty");
        clear_address_range_list(&list);
        return;
    }

    pass("remove containing range");
}

static void test_remove_range_from_start(void) {
    address_range_list list;

    init_address_range_list(&list);

    add_address_range(&list, make_range(0x1000, 0x2000));
    remove_address_range(&list, make_range(0x1000, 0x1500));

    if (count_nodes(&list) != 1) {
        fail("remove range from start", "should have 1 node");
        clear_address_range_list(&list);
        return;
    }

    if (list.first->range.start != 0x1501 || list.first->range.end != 0x2000) {
        fail("remove range from start", "remaining range incorrect");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("remove range from start");
}

static void test_remove_range_from_end(void) {
    address_range_list list;

    init_address_range_list(&list);

    add_address_range(&list, make_range(0x1000, 0x2000));
    remove_address_range(&list, make_range(0x1800, 0x2000));

    if (count_nodes(&list) != 1) {
        fail("remove range from end", "should have 1 node");
        clear_address_range_list(&list);
        return;
    }

    if (list.first->range.start != 0x1000 || list.first->range.end != 0x17FF) {
        fail("remove range from end", "remaining range incorrect");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("remove range from end");
}

static void test_remove_range_from_middle_split(void) {
    address_range_list list;
    address_range_node *first;
    address_range_node *second;

    init_address_range_list(&list);

    add_address_range(&list, make_range(0x1000, 0x3000));
    remove_address_range(&list, make_range(0x1800, 0x2800));

    if (count_nodes(&list) != 2) {
        fail("remove range from middle split", "should have 2 nodes after split");
        clear_address_range_list(&list);
        return;
    }

    first = list.first;
    second = list.last;

    if (first->range.start != 0x1000 || first->range.end != 0x17FF) {
        fail("remove range from middle split", "first range incorrect");
        clear_address_range_list(&list);
        return;
    }

    if (second->range.start != 0x2801 || second->range.end != 0x3000) {
        fail("remove range from middle split", "second range incorrect");
        clear_address_range_list(&list);
        return;
    }

    /* Verify linkage */
    if (first->next != second || second->prev != first) {
        fail("remove range from middle split", "node linkage incorrect");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("remove range from middle split");
}

static void test_remove_no_overlap(void) {
    address_range_list list;

    init_address_range_list(&list);

    add_address_range(&list, make_range(0x1000, 0x2000));
    remove_address_range(&list, make_range(0x3000, 0x4000));

    if (count_nodes(&list) != 1) {
        fail("remove no overlap", "should still have 1 node");
        clear_address_range_list(&list);
        return;
    }

    if (list.first->range.start != 0x1000 || list.first->range.end != 0x2000) {
        fail("remove no overlap", "range should be unchanged");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("remove no overlap");
}

static void test_remove_across_multiple_ranges(void) {
    address_range_list list;

    init_address_range_list(&list);

    add_address_range(&list, make_range(0x1000, 0x1FFF));
    add_address_range(&list, make_range(0x3000, 0x3FFF));
    add_address_range(&list, make_range(0x5000, 0x5FFF));

    /* Remove a range that spans the middle range and partially overlaps others */
    remove_address_range(&list, make_range(0x1800, 0x5500));

    if (count_nodes(&list) != 2) {
        fail("remove across multiple ranges", "should have 2 nodes");
        clear_address_range_list(&list);
        return;
    }

    if (list.first->range.start != 0x1000 || list.first->range.end != 0x17FF) {
        fail("remove across multiple ranges", "first range incorrect");
        clear_address_range_list(&list);
        return;
    }

    if (list.last->range.start != 0x5501 || list.last->range.end != 0x5FFF) {
        fail("remove across multiple ranges", "last range incorrect");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("remove across multiple ranges");
}

/*
 * ============================================================================
 * Address In Range Tests
 * ============================================================================
 */

static void test_is_address_in_range(void) {
    address_range ar;

    ar = make_range(0x1000, 0x2000);

    if (!is_address_in_range(ar, 0x1000)) {
        fail("is_address_in_range", "start address should be in range");
        return;
    }
    if (!is_address_in_range(ar, 0x2000)) {
        fail("is_address_in_range", "end address should be in range");
        return;
    }
    if (!is_address_in_range(ar, 0x1500)) {
        fail("is_address_in_range", "middle address should be in range");
        return;
    }
    if (is_address_in_range(ar, 0x0FFF)) {
        fail("is_address_in_range", "address before start should not be in range");
        return;
    }
    if (is_address_in_range(ar, 0x2001)) {
        fail("is_address_in_range", "address after end should not be in range");
        return;
    }

    pass("is_address_in_range");
}

static void test_is_address_in_range_list(void) {
    address_range_list list;

    init_address_range_list(&list);

    add_address_range(&list, make_range(0x1000, 0x1FFF));
    add_address_range(&list, make_range(0x3000, 0x3FFF));

    /* Test address in first range */
    if (!is_address_in_range_list(&list, 0x1500)) {
        fail("is_address_in_range_list", "address in first range should be found");
        clear_address_range_list(&list);
        return;
    }

    /* Test address in second range */
    if (!is_address_in_range_list(&list, 0x3500)) {
        fail("is_address_in_range_list", "address in second range should be found");
        clear_address_range_list(&list);
        return;
    }

    /* Test address between ranges */
    if (is_address_in_range_list(&list, 0x2500)) {
        fail("is_address_in_range_list", "address between ranges should not be found");
        clear_address_range_list(&list);
        return;
    }

    /* Test address before first range */
    if (is_address_in_range_list(&list, 0x0500)) {
        fail("is_address_in_range_list", "address before ranges should not be found");
        clear_address_range_list(&list);
        return;
    }

    /* Test address after last range */
    if (is_address_in_range_list(&list, 0x5000)) {
        fail("is_address_in_range_list", "address after ranges should not be found");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("is_address_in_range_list");
}

static void test_is_address_in_range_list_null(void) {
    if (is_address_in_range_list(NULL, 0x1000) != FALSE) {
        fail("is_address_in_range_list NULL", "NULL list should return FALSE");
        return;
    }

    pass("is_address_in_range_list NULL");
}

static void test_is_address_in_range_list_empty(void) {
    address_range_list list;

    init_address_range_list(&list);

    if (is_address_in_range_list(&list, 0x1000) != FALSE) {
        fail("is_address_in_range_list empty", "empty list should return FALSE");
        return;
    }

    pass("is_address_in_range_list empty");
}

/*
 * ============================================================================
 * Clear List Tests
 * ============================================================================
 */

static void test_clear_list(void) {
    address_range_list list;

    init_address_range_list(&list);

    add_address_range(&list, make_range(0x1000, 0x1FFF));
    add_address_range(&list, make_range(0x3000, 0x3FFF));
    add_address_range(&list, make_range(0x5000, 0x5FFF));

    clear_address_range_list(&list);

    if (list.first != NULL) {
        fail("clear list", "first should be NULL after clear");
        return;
    }
    if (list.last != NULL) {
        fail("clear list", "last should be NULL after clear");
        return;
    }

    pass("clear list");
}

static void test_clear_list_null(void) {
    /* Should not crash */
    clear_address_range_list(NULL);

    pass("clear list NULL");
}

static void test_clear_list_empty(void) {
    address_range_list list;

    init_address_range_list(&list);

    /* Should not crash */
    clear_address_range_list(&list);

    if (list.first != NULL || list.last != NULL) {
        fail("clear list empty", "list should remain empty");
        return;
    }

    pass("clear list empty");
}

/*
 * ============================================================================
 * Edge Case Tests
 * ============================================================================
 */

static void test_edge_case_full_address_space(void) {
    address_range_list list;

    init_address_range_list(&list);

    /* Add entire address space */
    add_address_range(&list, make_range(0x0000, 0xFFFF));

    if (count_nodes(&list) != 1) {
        fail("full address space", "should have 1 node");
        clear_address_range_list(&list);
        return;
    }

    if (!is_address_in_range_list(&list, 0x0000)) {
        fail("full address space", "address 0x0000 should be in range");
        clear_address_range_list(&list);
        return;
    }
    if (!is_address_in_range_list(&list, 0xFFFF)) {
        fail("full address space", "address 0xFFFF should be in range");
        clear_address_range_list(&list);
        return;
    }
    if (!is_address_in_range_list(&list, 0x8000)) {
        fail("full address space", "address 0x8000 should be in range");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("full address space");
}

static void test_edge_case_single_address(void) {
    address_range_list list;

    init_address_range_list(&list);

    /* Add single address range */
    add_address_range(&list, make_range(0x1234, 0x1234));

    if (count_nodes(&list) != 1) {
        fail("single address", "should have 1 node");
        clear_address_range_list(&list);
        return;
    }

    if (!is_address_in_range_list(&list, 0x1234)) {
        fail("single address", "address should be in range");
        clear_address_range_list(&list);
        return;
    }
    if (is_address_in_range_list(&list, 0x1233)) {
        fail("single address", "address-1 should not be in range");
        clear_address_range_list(&list);
        return;
    }
    if (is_address_in_range_list(&list, 0x1235)) {
        fail("single address", "address+1 should not be in range");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("single address");
}

static void test_edge_case_boundary_addresses(void) {
    address_range_list list;

    init_address_range_list(&list);

    /* Test at address space boundaries */
    add_address_range(&list, make_range(0x0000, 0x00FF));
    add_address_range(&list, make_range(0xFF00, 0xFFFF));

    if (count_nodes(&list) != 2) {
        fail("boundary addresses", "should have 2 nodes");
        clear_address_range_list(&list);
        return;
    }

    if (!is_address_in_range_list(&list, 0x0000)) {
        fail("boundary addresses", "0x0000 should be in range");
        clear_address_range_list(&list);
        return;
    }
    if (!is_address_in_range_list(&list, 0xFFFF)) {
        fail("boundary addresses", "0xFFFF should be in range");
        clear_address_range_list(&list);
        return;
    }

    clear_address_range_list(&list);
    pass("boundary addresses");
}

/*
 * ============================================================================
 * Main test runner
 * ============================================================================
 */

int main(void) {
    printf("Address Range List Test Suite\n");
    printf("==============================\n\n");

    printf("--- Initialization Tests ---\n");
    test_init_list();

    printf("\n--- Add Range Tests ---\n");
    test_add_single_range();
    test_add_range_null_list();
    test_add_disjoint_ranges_ascending();
    test_add_disjoint_ranges_descending();
    test_add_range_in_middle();
    test_add_adjacent_ranges_merge();
    test_add_overlapping_ranges_merge();
    test_add_contained_range();
    test_add_containing_range();
    test_add_duplicate_range();

    printf("\n--- Remove Range Tests ---\n");
    test_remove_range_null_list();
    test_remove_range_empty_list();
    test_remove_entire_range();
    test_remove_containing_range();
    test_remove_range_from_start();
    test_remove_range_from_end();
    test_remove_range_from_middle_split();
    test_remove_no_overlap();
    test_remove_across_multiple_ranges();

    printf("\n--- Address In Range Tests ---\n");
    test_is_address_in_range();
    test_is_address_in_range_list();
    test_is_address_in_range_list_null();
    test_is_address_in_range_list_empty();

    printf("\n--- Clear List Tests ---\n");
    test_clear_list();
    test_clear_list_null();
    test_clear_list_empty();

    printf("\n--- Edge Case Tests ---\n");
    test_edge_case_full_address_space();
    test_edge_case_single_address();
    test_edge_case_boundary_addresses();

    printf("\n==============================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
