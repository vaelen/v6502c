#ifndef _V6502C_H_
#define _V6502C_H_

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

#define V6502C_VERSION "v6502c v1.0"
#define V6502C_COPYRIGHT "Copyright (c) 2025, Andrew C. Young <andrew@vaelen.org>"
#define IRQ_VECTOR 0xFFFE
#define RESET_VECTOR 0xFFFC
#define NMI_VECTOR 0xFFFA

typedef unsigned char byte;
typedef unsigned short int address;

/**
 * The ReadFn and WriteFn types are used to interact with memory
 * in a flexible way. By providing read and write functions that
 * respond to specific memory addresses differently, it is possible
 * to emulate hardware peripherals or memory banking.
 **/

typedef char bool;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

/** Read a byte from the given emulated memory address. */
typedef byte ReadFn(address);

/** Write a byte to the given emulated memory address. */
typedef void WriteFn(address, byte);

/** Called between each CPU cycle.
    Can be used to slow down execution. */
typedef void TickFn(void);

typedef struct cpu_s {
  address pc;
  byte a;
  byte x;
  byte y;
  byte sr;
  byte sp;
  bool halted;
  bool reset;
  bool irq;
  bool nmi;
  ReadFn *read;
  WriteFn *write;
  TickFn *tick;
} cpu;

/** Call this to initialize the CPU data structure before using it. */
void cpu_init(cpu *c);

/** Read a byte from the given address. */
byte cpu_read_byte(cpu *c, address a);

/** Read a two byte address starting at the given address. */
address cpu_read_address(cpu *c, address a);

/** Write a byte to the given address and update SR. */
void cpu_write_byte(cpu *c, address a, byte b);

/** Write a two byte address starting at the given address and update SR. */
void cpu_write_address(cpu *c, address a, address value);

/** Read the next byte from memory and increment the program counter. */
byte cpu_next_byte(cpu *c);

/** Read the next address from memory and increment the program counter. */
address cpu_next_address(cpu *c);

/** Step the CPU by one instruction. */
void cpu_step(cpu *c);

/** Run the CPU until it halts. */
void cpu_run(cpu *c);

/** Halt the CPU. */
void cpu_halt(cpu *c);

/** Reset the CPU. */
void cpu_reset(cpu *c);

/** Trigger a CPU interrupt request (IRQ). */
void cpu_irq(cpu *c);

/** Trigger a CPU non-maskable interrupt. */
void cpu_nmi(cpu *c);

#endif
