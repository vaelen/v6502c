#ifndef _MAIN_H_
#define _MAIN_H_

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
#include <string.h>

#include <v6502.h>

typedef struct addrr {
  address start;
  address end;
} address_range;

void tick(void);

int read_line(FILE *in, char *buf, int maxlen);
void print_register(char *name, byte value);
void print_register_change(char *name, byte old, byte new);
void print_pc(address value);
void print_pc_change(address old, address new);
void print_memory_header(void);
void print_memory_location(address a);
void print_memory(cpu *c, address start, address end);
void print_help(void);
void not_implemented(void);
int is_whitespace(char c);
void parseargs(char *cmdbuf, int *argc, char **argv);
int parse_byte(char *s, byte *b);
int parse_address(char *s, address *a);
int parse_address_range(char *s, address_range *r);
void read_lines(cpu *c, FILE *in);
int write_file(cpu *c, address_range ar, char *filename);
int read_file(cpu *c, char *filename);
int parse_command(cpu *c, char *cmdbuf);

#endif
