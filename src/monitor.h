#ifndef _MONITOR_H_
#define _MONITOR_H_

/**
 *
 * System Monitor / UI for the vMachine.
 * Provides an interactive command-line interface for controlling
 * the virtual machine, inspecting memory, and debugging code.
 *
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

#include "vmachine.h"

/* Monitor entry point - starts the interactive REPL */
void monitor_run(vmachine_t *machine);

/* Monitor REPL - reads commands from a file or stdin */
void monitor_repl(vmachine_t *machine, FILE *in);

/* Trace callback for use with vmachine_t.trace_fn */
void monitor_trace_fn(vmachine_t *machine, cpu *prevc, cpu *c);

/* Command parsing */
int parse_command(vmachine_t *machine, char *cmdbuf);

/* Input/output utilities */
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

/* Parsing utilities */
int is_whitespace(char c);
void parseargs(char *cmdbuf, int *argc, char **argv);
int parse_byte(char *s, byte *b);
int parse_address(char *s, address *a);
int parse_address_range(char *s, address_range *r);

/* File I/O commands */
int write_file(vmachine_t *machine, address_range ar, char *filename);
int read_file(vmachine_t *machine, char *filename);

#endif
