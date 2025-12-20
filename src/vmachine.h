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

#define VMACHINE_ROM_START 0xD000
#define VMACHINE_ROM_SIZE  0x3000

/* Forward declaration for trace callback */
struct vmachine;

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

  /* Optional trace callback - called each tick when V6502C_TRACE is set */
  void (*trace_fn)(struct vmachine *machine, cpu *prevc, cpu *c);
} vmachine_t;

typedef struct vmachine_config {
  byte *rom_data;
  size_t rom_size;
  int tick_duration;
  FILE *acia1_input;
  FILE *acia1_output;
  FILE *acia2_input;
  FILE *acia2_output;
} vmachine_config_t;

/* Machine lifecycle functions */
void init_vmachine(vmachine_t *machine, vmachine_config_t *config);
void cleanup_vmachine(vmachine_t *machine);

/* Machine I/O functions (for CPU callbacks) */
void machine_tick(vmachine_t *machine);
byte machine_read(vmachine_t *machine, address a);
void machine_write(vmachine_t *machine, address a, byte b);

/* Memory protection functions */
void add_protected_range(vmachine_t *machine, address_range ar);
void remove_protected_range(vmachine_t *machine, address_range ar);
bool is_address_protected(address_range_list *list, address a);

#endif
