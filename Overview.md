# v6502c Project Overview

## Introduction

v6502c is a portable 6502 CPU emulator written in ANSI C. It is a port of the
author's v6502 Rust project, designed specifically for embedding in retro
computing projects targeting systems like the classic Macintosh. The emulator
prioritizes portability over cycle accuracy, making it suitable for educational
purposes and running software like Microsoft BASIC or EhBASIC.

What started as a single CPU core plus a debugger is now a small library
(`libv6502`) with three layers: the CPU itself, a set of emulated peripherals,
and an example machine ("vMachine") that wires them together into an Apple
II-inspired computer capable of running Microsoft BASIC.

## Project Structure

```
v6502c/
├── Makefile              # Build configuration (clang, ANSI mode)
├── README.md             # User documentation
├── CLAUDE.md             # Guidance for Claude Code
├── LICENSE               # MIT License
├── bin/                  # Compiled binaries (v6502c, hello, bin2woz)
├── lib/                  # libv6502.a and libv6502.so
├── obj/                  # Object files
├── config/               # Monitor startup scripts (basic, iotest, memdump)
├── rom/                  # Pre-built ROM images (basic.woz, iotest.woz)
├── programs/             # Test programs in 6502 assembly
├── src/
│   ├── vtypes.h          # byte / address / bool typedefs
│   ├── v6502.{c,h}       # Core CPU implementation
│   ├── inst.h            # Instruction/addressing mode lookup tables
│   ├── devices.{c,h}     # 6551 ACIA, 6522 VIA, file I/O device
│   ├── addrlist.{c,h}    # Address-range list (write protection)
│   ├── vmachine.{c,h}    # Example machine: memory, device decoding
│   ├── monitor.{c,h}     # Wozmon-compatible monitor and script interpreter
│   ├── hello.{c,h,s}     # Standalone embedding example
│   └── infloop.s         # Infinite loop test program
├── tests/                # cputest.c, devtest.c, addrtest.c
├── utils/
│   ├── cli.c             # bin/v6502c entry point
│   └── bin2woz.c         # Binary → Wozmon format converter
└── msbasic/              # Microsoft BASIC port (builds rom/basic.woz)
```

## Architecture

### CPU Structure

The emulator models the 6502 CPU with the following state:

| Component | Size   | Description             |
|-----------|--------|-------------------------|
| PC        | 16-bit | Program Counter         |
| A         | 8-bit  | Accumulator             |
| X         | 8-bit  | X Index Register        |
| Y         | 8-bit  | Y Index Register        |
| SR        | 8-bit  | Status Register (flags) |
| SP        | 8-bit  | Stack Pointer           |

Plus emulator state: `variant` (6502 vs 65C02), `halted`, and pending `irq` /
`nmi` flags.

### Callback Interface

The CPU core has no notion of memory layout. Host code supplies function
pointers:

- `ReadFn` — memory read callback
- `WriteFn` — memory write callback
- `TickFn` — called after each instruction, for timing or tracing

This allows host programs to implement memory-mapped I/O, bank switching,
ROM/RAM partitioning, and hardware peripheral emulation without touching the
core.

### Layering

`v6502.c` alone is enough to embed the CPU — `src/hello.c` does exactly that in
about 60 lines. Everything above it is optional:

- `devices.c` implements peripherals as plain structs with `read`/`write`
  functions taking a register offset, so a host can map them anywhere.
- `vmachine.c` is the reference machine: a flat 64K `mem[]` array, address
  decoding for the four devices, and a write-protection list.
- `monitor.c` is a Wozmon-compatible command interpreter that reads either from
  a terminal or from a script file.

`libv6502` bundles all five modules, so linking the library gets the machine and
monitor along with the CPU.

## Emulated Machine

The vMachine memory map is Apple II-inspired:

| Address Range | Description                                        |
|---------------|----------------------------------------------------|
| `$0000-$00FF` | Zero page                                          |
| `$0100-$01FF` | Stack                                              |
| `$0200-$03FF` | Free RAM / BASIC scratch                           |
| `$0400-$BFFF` | RAM — BASIC program and variable space             |
| `$C000-$C00F` | Unmapped (behaves as RAM)                          |
| `$C010-$C013` | ACIA #1 (6551) — console                           |
| `$C020-$C023` | ACIA #2 (6551) — present but disconnected          |
| `$C030-$C03F` | VIA (6522) — ports, two timers, interrupts         |
| `$C040-$C04F` | File I/O device — BASIC LOAD/SAVE                  |
| `$C050-$CFFF` | Unmapped (behaves as RAM)                          |
| `$D000-$F7FF` | BASIC ROM                                          |
| `$F800-$FFF9` | Extension ROM — line editor, ACIA and file glue    |
| `$FFFA-$FFFF` | NMI / RESET / IRQ vectors                          |

ROM is enforced by the write-protection list rather than by address decoding, so
a machine that skips the `PROTECT` commands in `config/*` has 64K of writable
RAM with devices layered on top.

### Devices

**MOS 6551 ACIA** — four registers (data, status, command, control). Backed by a
`FILE *` pair, so it can be attached to stdin/stdout, a PTY, or a file. Two
instances are instantiated; the second has no backing stream in the default
build and always reports "no data ready".

**MOS 6522 VIA** — ports A and B with data-direction registers, two 16-bit
timers with one-shot and continuous modes, shift register, and the interrupt
flag/enable registers. Timers advance one count per `tick`, i.e. per
instruction, not per clock cycle.

**File I/O device** — a non-historical device providing BASIC's `LOAD` and
`SAVE`. A filename is written one character at a time through an index register,
then a command byte opens, reads, writes, or closes the file.

## Implemented Features

### Addressing Modes (15 total)

Standard 6502: implied, accumulator, immediate, zero-page, zero-page X,
zero-page Y, absolute, absolute X, absolute Y, indirect (JMP only), pre-indexed
indirect (X), post-indexed indirect (Y), relative.

65C02 extensions: zero-page indirect, absolute indexed indirect.

### Instructions

**Data movement:** LDA, LDX, LDY, STA, STX, STY, TAX, TAY, TSX, TXA, TXS, TYA,
PHA, PHP, PLA, PLP

**Arithmetic/logic:** ADC, SBC, AND, ORA, EOR, ASL, LSR, ROL, ROR, INC, DEC,
INX, INY, DEX, DEY, CMP, CPX, CPY, BIT

**Control flow:** JMP, JSR, RTS, RTI, BRK, NOP, BEQ, BNE, BCS, BCC, BMI, BPL,
BVS, BVC

**Status flags:** CLC, CLD, CLI, CLV, SEC, SED, SEI

**Not implemented:** every 65C02-only opcode. BRA, STZ, PHX, PHY, PLX, PLY, STP,
WAI, TSB, TRB, and all of BBR/BBS/RMB/SMB appear in the `inst.h` lookup tables
but have no case in the execution switch, so they decode and then silently do
nothing. Both 65C02 addressing modes are implemented; nothing reaches them yet.

### BCD Mode

Full decimal mode for ADC and SBC. ADC performs nibble-carry decimal adjustment;
SBC uses the +10 borrow algorithm. N and Z flags reflect the binary result, which
matches real hardware. The V flag differs by variant: the NMOS 6502 clears it in
decimal mode, the 65C02 computes it normally. `cpu_set_variant()` selects the
behavior (65C02 is the default) and the monitor exposes it as `CPU [6502|65C02]`.

### Interrupts

IRQ, NMI, and BRK are fully serviced with correct stack frames. Interrupts are
checked after each instruction. NMI has priority; IRQ is level-triggered and
remains pending while the I flag is set.

### Monitor

Wozmon-compatible command interpreter:

- Register inspection and modification (`?`, `PC`, `A`, `X`, `Y`, `SR`, `SP`)
- Memory examination and editing (`FFFF`, `FF00.FFFF`, `FFFF: FF FE`, `:FF`)
- `RESET`, `STEP`, `GO`, `TRACE` execution control
- `PROTECT` / `UNPROTECT` address ranges
- `LOAD` / `SAVE` in Wozmon format
- `CPU 6502` / `CPU 65C02` variant selection
- `VERBOSE` toggling for device and protection logging
- SIGINT handling for graceful interruption of a running program

The same interpreter reads script files, which is how `config/basic`,
`config/iotest`, and `config/memdump` drive startup.

## Current Status

### Working

- Full standard 6502 instruction set with all addressing modes
- BCD mode for ADC and SBC, with per-variant flag behavior
- IRQ, NMI, and BRK servicing
- ACIA, VIA, and file I/O device emulation
- Write-protected memory ranges
- Wozmon-compatible monitor with scripting
- Static and shared library builds
- Runs Microsoft BASIC to a usable `OK` prompt, including `LOAD`/`SAVE` and a
  line editor supplied by the extension ROM

### Test Coverage

`make test` builds and runs three suites, 89 tests total:

| Suite            | Tests | Covers                                         |
|------------------|-------|------------------------------------------------|
| `tests/cputest`  | 32    | Instructions, flags, interrupts, BCD, variants |
| `tests/devtest`  | 27    | ACIA, VIA timers, file I/O device              |
| `tests/addrtest` | 30    | Address-range list add/remove/query            |

## What Remains To Be Done

1. **65C02 instruction set** — 42 opcodes are decoded but not executed. The
   simple ones (BRA, STZ, PHX/PHY/PLX/PLY) are a handful of lines each; TSB, TRB,
   and the BBR/BBS/RMB/SMB families are the remaining work.
2. **Unknown-opcode handling** — the execution switch's `default:` case does
   nothing at all. Trapping or logging would turn silent misbehavior into a
   diagnosable failure.
3. **Klaus Dormann's 6502 functional test** — the standard conformance suite has
   not been run against the core.
4. **Second ACIA** — instantiated but never given a backing stream in the default
   build.

## Design Decisions

### Portability Over Accuracy

The emulator is intentionally not cycle-accurate. Instructions execute in a
single step, and the `tick` callback fires once per instruction rather than once
per clock cycle. This simplifies the implementation and improves performance at
the cost of demo compatibility.

### Minimal Dependencies

ANSI C with minimal standard library use, to maximize portability to older and
exotic systems. The CPU core itself pulls in nothing beyond the typedefs in
`vtypes.h`.

### Callback-Based I/O

Rather than hardcoding a memory layout, all memory access goes through function
pointers. `vmachine.c` is one possible layout, not a required one.

## Building

```bash
make          # everything: library, emulator, hello, bin2woz
make test     # build and run all three test suites
make clean    # remove build artifacts
```

Requirements:

- clang-18 (or modify the Makefile for gcc)
- vasm6502 (oldstyle) — optional, for assembling `.s` files
- cc65 — optional, for rebuilding the MS BASIC ROM

## Usage Examples

### Embedded Usage

```c
#include "v6502.h"

byte memory[0x10000];

byte read(address addr) { return memory[addr]; }
void write(address addr, byte val) { memory[addr] = val; }

int main(void) {
    cpu c;
    cpu_init(&c);
    c.read = read;
    c.write = write;

    /* Load program into memory, set reset vector... */

    cpu_reset(&c);
    cpu_run(&c);  /* Run until cpu_halt() */
    return 0;
}
```

`cpu_run` loops until `cpu_halt()` is called. BRK does *not* stop the CPU — it
vectors through `$FFFE` like real hardware.

See `src/hello.c` for a complete version. It maps `$FF00` as a character
device and `$FF01` as a halt register whose write callback calls `cpu_halt()`.

### Running BASIC

```
$ ./bin/v6502c rom/basic.woz config/basic
```

`config/basic` patches the IRQ/BRK vector at `$FFFE` to `$F01B`, write-protects
`$C000` and `$D000-$FFFF`, enables verbose logging, then resets and runs. Remove
its `VERBOSE` line for output that is not interleaved with per-byte ACIA logging.
Its `LOAD rom/basic.woz` line is redundant now that the ROM is passed as argv[1].

### Interactive Session

```
$ ./bin/v6502c rom/basic.woz config/basic
Loaded ROM: rom/basic.woz, Size: 12288 bytes
ACIA1 PTY: /dev/pts/1
ACIA2 PTY: /dev/pts/2
v6502c v1.0
Copyright (c) 2025, Andrew C. Young <andrew@vaelen.org>

Processing command-line script files...
Loading config/basic
Protecting memory range D000.FFFF
Type 'help' for help.

=> ?
PC : F01A
 A : 00
 X : 00
 Y : 00
SR : 36
SP : FD
=> CPU
CPU : 65C02
=> Q
```

BASIC itself talks to the PTY, not to this terminal — attach to `/dev/pts/1`
with `tio` or `screen` to reach the `OK` prompt while the monitor stays on
stdin. `utils/cli.h` defines `__CREATE_PTYS__`; comment it out to wire ACIA #1
to stdin/stdout instead.

## License

MIT License - see LICENSE file for details.

## Author

Andrew C. Young <andrew@vaelen.org>
