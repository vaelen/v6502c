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
#include "inst.h"

void _reset(cpu *c) {
  if (c == NULL) return; 
  c->pc = cpu_read_address(c, RESET_VECTOR);
  c->a = 0;
  c->x = 0;
  c->y = 0;
  /*
    Based on the SP from Visual6502 after a reset.
   */
  c->sr = 0x36;
  /*
    In a real 6502, the reset sequence results in three stack push
    operations and therefore the SP will equal 0xFD after a reset.
    See: https://www.pagetable.com/?p=410
  */
  c->sp = 0xFD;
  c->halted = FALSE;
  c->reset = FALSE;
  c->irq = FALSE;
  c->nmi = FALSE;
}

void cpu_init(cpu *c) {
  if (c == NULL) return; 
  c->write = NULL;
  c->read = NULL;
  c->tick = NULL;
  _reset(c);
}

byte cpu_read_byte(cpu *c, address a) {
  byte b = 0;
  if (c == NULL || c->read == NULL) return 0;
  b = c->read(a);
  /** TODO: Update status register. */
  return b;
}

address cpu_read_address(cpu *c, address a) {
  byte hi = 0, lo = 0;
  address value;
  
  if (c == NULL) return 0;
  lo = cpu_read_byte(c, a++);
  hi = cpu_read_byte(c, a++);
  value = (hi << 8) | lo;
  return value;
}

void cpu_write_byte(cpu *c, address a, byte b) {
  if (c == NULL || c->write == NULL) return;
  c->write(a, b);
  /** TODO: Update status register. */
}

void cpu_write_address(cpu *c, address a, address value) {
  byte hi = 0, lo = 0;
  if (c == NULL) return;
  lo = (byte) value;
  hi = (byte) value >> 8;
  cpu_write_byte(c, a++, hi);
  cpu_write_byte(c, a++, lo);
}

byte cpu_next_byte(cpu *c) {
  if (c == NULL) return 0;
  return cpu_read_byte(c, c->pc++);
}

address cpu_next_address(cpu *c) {
  address a = 0;
  if (c == NULL) return 0;
  a = cpu_read_address(c, c->pc);
  c->pc += 2;
  return a;
}

void cpu_step(cpu *c) {
  byte b = 0, lo = 0, hi = 0;
  address a = 0;
  enum instruction_t instruction = I_BRK;
  enum addressing_t addressing = A_IMP;
  
  if (c == NULL) return;

  /** Handle reset. */
  if (c->reset) {
    c->reset = FALSE;
    _reset(c);
    return;
  }
  
  b = cpu_next_byte(c);

  instruction = instructions[b];
  addressing = addressings[b];

  switch (addressing) {
  case A_IMP:
  default:
    /* No address needed */
    break;
  }
  
  switch (instruction) {
  case I_NOP:
  default:
    /* Do nothing */
    break;
  }

  /** Handle Interrupts */
  if (c->nmi) {
    c->nmi = FALSE;
  } else if (c->irq) {
    c->irq = FALSE;
  }
  
}

void cpu_run(cpu *c) {
  if (c == NULL) return;
  while (!c->halted) {
    cpu_step(c);
    if (c->tick != NULL) {
      c->tick();
    }
  }
}

/** Halt the CPU. */
void cpu_halt(cpu *c) {
  if (c == NULL) return;
  c->halted = TRUE;
}

/** Trigger a CPU interrupt request (IRQ). */
void cpu_irq(cpu *c) {
  if (c == NULL) return;
  c->irq = TRUE;
}

/** Trigger a CPU reset. */
void cpu_reset(cpu *c) {
  if (c == NULL) return;
  c->reset = TRUE;
}  

/** Trigger a CPU reset. */
void cpu_nmi(cpu *c) {
  if (c == NULL) return;
  c->nmi = TRUE;
}  
