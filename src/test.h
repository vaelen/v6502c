#ifndef _TEST_H_
#define _TEST_H_

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

#include "v6502.h"

/* ANSI color codes for terminal output */
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_RESET "\033[0m"

/* Test result tracking */
extern int tests_passed;
extern int tests_failed;

/* Test framework functions */
void pass(const char *test_name);
void fail(const char *test_name, const char *reason);

/* CPU test helpers */
void test_init(void);
void test_cleanup(void);
void test_reset_cpu(void);

/* Memory simulation */
byte test_read(address a);
void test_write(address a, byte b);

/* Individual test functions */
void test_adc_binary(void);
void test_adc_bcd(void);
void test_sbc_binary(void);
void test_sbc_bcd(void);
void test_and(void);
void test_asl(void);
void test_bit(void);
void test_branches(void);
void test_compare_ops(void);
void test_dec_inc(void);
void test_eor(void);
void test_flags(void);
void test_jmp_jsr(void);
void test_load_store(void);
void test_logical_shifts(void);
void test_nop(void);
void test_ora(void);
void test_push_pull(void);
void test_rotates(void);
void test_transfers(void);
void test_cpu_variant_6502(void);
void test_cpu_variant_65c02(void);

#endif
