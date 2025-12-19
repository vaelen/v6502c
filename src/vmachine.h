#ifndef _VMACHINE_H_
#define _VMACHINE_H_

/**
 * 
 * "vMachine" is an example implementation using the v6502c library 
 * to emulate a 6502 CPU and its peripherals. It emulates a simple
 * computer similar to the Apple II but without graphics or sound.
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

#include <stdio.h>
#include <string.h>

#include <v6502.h>
#include <addrlist.h>
#include <devices.h>

typedef struct vmachine {
  byte mem[0x10000];
  address_range_list protected_ranges;
  cpu c;
  cpu prevc;
  int tick_duration;

  /* Emulated devices */
  acia_t *acia1;   /* Primary serial: stdin/stdout */
  acia_t *acia2;   /* Secondary serial: disconnected */
  via_t *via;      /* VIA with timers */
  fileio_t *fio;   /* File I/O device */
} vmachine_t;

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
void read_lines(vmachine_t *machine, FILE *in);
int write_file(vmachine_t *machine, address_range ar, char *filename);
int read_file(vmachine_t *machine, char *filename);
int parse_command(vmachine_t *machine, char *cmdbuf);

void add_protected_range(vmachine_t *machine, address_range ar);
void remove_protected_range(vmachine_t *machine, address_range ar);
bool is_address_protected(address_range_list *list, address a);

/* Machine lifecycle functions */
void init_vmachine(vmachine_t *machine);
void run_vmachine(vmachine_t *machine);
void cleanup_vmachine(vmachine_t *machine);

/* Machine I/O functions (for CPU callbacks) */
void machine_tick(vmachine_t *machine);
byte machine_read(vmachine_t *machine, address a);
void machine_write(vmachine_t *machine, address a, byte b);

#endif
