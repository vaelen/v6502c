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

#include "vmachine.h"

void machine_tick(vmachine_t *machine) {
  /* Update VIA timers */
  if (machine->via != NULL) {
    via_tick(machine->via);
  }

  /* Call trace callback if tracing is enabled */
  if (V6502C_TRACE && machine->trace_fn != NULL) {
    machine->trace_fn(machine, &machine->prevc, &machine->c);
    machine->prevc = machine->c;
  }
}

byte machine_read(vmachine_t *machine, address a) {
  /* ACIA #1: $C010-$C013 */
  if (a >= 0xC010 && a <= 0xC013) {
    return acia_read(machine->acia1, (byte)(a & 0x03));
  }
  /* ACIA #2: $C020-$C023 */
  if (a >= 0xC020 && a <= 0xC023) {
    return acia_read(machine->acia2, (byte)(a & 0x03));
  }
  /* VIA: $C030-$C03F */
  if (a >= 0xC030 && a <= 0xC03F) {
    return via_read(machine->via, (byte)(a & 0x0F));
  }
  /* File I/O: $C040-$C04F */
  if (a >= 0xC040 && a <= 0xC04F) {
    return fileio_read(machine->fio, (byte)(a & 0x0F));
  }
  return machine->mem[a];
}

void machine_write(vmachine_t *machine, address a, byte b) {
  /* ACIA #1: $C010-$C013 */
  if (a >= 0xC010 && a <= 0xC013) {
    acia_write(machine->acia1, (byte)(a & 0x03), b);
    return;
  }
  /* ACIA #2: $C020-$C023 */
  if (a >= 0xC020 && a <= 0xC023) {
    acia_write(machine->acia2, (byte)(a & 0x03), b);
    return;
  }
  /* VIA: $C030-$C03F */
  if (a >= 0xC030 && a <= 0xC03F) {
    via_write(machine->via, (byte)(a & 0x0F), b);
    return;
  }
  /* File I/O: $C040-$C04F */
  if (a >= 0xC040 && a <= 0xC04F) {
    fileio_write(machine->fio, (byte)(a & 0x0F), b);
    return;
  }
  if (is_address_protected(&machine->protected_ranges, a)) {
    /* Address is write-protected */
    if (V6502C_VERBOSE) {
      fprintf(stderr, "Write to protected address %04X ignored\n", a);
    }
    return;
  }
  machine->mem[a] = b;
}

/* Add a protected memory range where writes are ignored. */
void add_protected_range(vmachine_t *machine, address_range ar) {
  add_address_range(&machine->protected_ranges, ar);
}

/* Remove a protected memory range, allowing writes. */
void remove_protected_range(vmachine_t *machine, address_range ar) {
  remove_address_range(&machine->protected_ranges, ar);
}

/* Check if an address is within any protected memory range. */
bool is_address_protected(address_range_list *list, address a) {
  return is_address_in_range_list(list, a);
}

/* Initialize the virtual machine instance. */
void init_vmachine(vmachine_t *machine, vmachine_config_t *config) {
  address_range ar;

  /* Initialize protected address ranges */
  init_address_range_list(&machine->protected_ranges);

  /* Create device instances */
  machine->acia1 = acia_create(config->acia1_input, config->acia1_output);
  machine->acia2 = acia_create(config->acia2_input, config->acia2_output);
  machine->via = via_create();
  machine->fio = fileio_create();
  machine->trace_fn = NULL;

  cpu_init(&machine->c);

  /* Load ROM into memory (up to 16KB) */
  memset(machine->mem, 0, sizeof(machine->mem));
  if (config->rom_size < 0) {
    config->rom_size = 0;
  } else if (config->rom_size > VMACHINE_ROM_SIZE) {
    config->rom_size = VMACHINE_ROM_SIZE;
  }
  memcpy(&machine->mem[VMACHINE_ROM_START], config->rom_data, config->rom_size);
  
  /* Protect ROM area from writes */
  ar.start = VMACHINE_ROM_START;
  ar.end = 0xFFFF;
  add_protected_range(machine, ar);
}

/* Clean up the virtual machine instance. */
void cleanup_vmachine(vmachine_t *machine) {
  /* Clean up devices */
  acia_destroy(machine->acia1);
  acia_destroy(machine->acia2);
  via_destroy(machine->via);
  fileio_destroy(machine->fio);

  clear_address_range_list(&machine->protected_ranges);
}
