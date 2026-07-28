# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
make              # libv6502 (.a + .so), v6502c, hello, bin2woz
make test         # build and run cputest, devtest, addrtest
make clean        # remove bin/, obj/, lib/ contents
```

Compiler: clang-18 with `-ansi -Wpedantic -Isrc` (strict ANSI C compliance required).

Assembler: vasm6502 (oldstyle) for `.s` files in `src/` and `programs/`.
The MS BASIC ROM is built separately with cc65 (`cd msbasic && ./make.sh`), then
converted with `./bin/bin2woz D000 msbasic/tmp/v6502c.bin > rom/basic.woz`.

## Running

```bash
./bin/v6502c rom/basic.woz config/basic   # BASIC, driven by a monitor script
./bin/v6502c rom/basic.woz                # no script: auto reset + run, verbose
./bin/v6502c rom/basic.woz config/memdump # dump all of memory, then quit
./bin/hello                               # standalone embedded example
```

The first argument is always a ROM file (Wozmon format, loaded at `$D000`).
Remaining arguments are monitor script files — plain text files of monitor
commands, run before the interactive REPL starts. See `config/`.

`utils/cli.h` defines `__CREATE_PTYS__`, so by default the emulator allocates a
PTY per ACIA and prints the slave device names at startup. Attach a terminal to
the first one (`tio /dev/pts/N`) to talk to BASIC; the monitor REPL stays on
stdin. Comment out that `#define` to wire ACIA #1 to stdin/stdout instead — but
then BASIC and the monitor compete for the same input.

## Architecture

### Layers

| File                 | Role                                                            |
|----------------------|-----------------------------------------------------------------|
| `src/v6502.{c,h}`    | CPU core: instructions, addressing modes, stack, interrupts     |
| `src/inst.h`         | Opcode → instruction / addressing mode lookup tables            |
| `src/vtypes.h`       | `byte`, `address`, `bool` typedefs                              |
| `src/devices.{c,h}`  | 6551 ACIA, 6522 VIA, and a custom file I/O device               |
| `src/addrlist.{c,h}` | Address-range list used for write protection                    |
| `src/vmachine.{c,h}` | Example machine: 64K memory, device decoding, protection        |
| `src/monitor.{c,h}`  | Wozmon-compatible monitor REPL and script interpreter           |
| `utils/cli.c`        | `bin/v6502c` entry point: ROM loading, PTY setup, argv handling  |
| `utils/bin2woz.c`    | Raw binary → Wozmon text format converter                       |
| `src/hello.c`        | Minimal embedding example, independent of vmachine              |

The CPU core knows nothing about the machine; everything above `v6502.c` is
optional. `libv6502` bundles all of it, so linking the library pulls in the
example machine and monitor as well.

### Embedding Interface

```c
cpu c;
cpu_init(&c);
c.read = my_read_fn;    /* byte read(address) */
c.write = my_write_fn;  /* void write(address, byte) */
c.tick = my_tick_fn;    /* void tick(void) - optional */
cpu_reset(&c);
cpu_run(&c);            /* runs until cpu_halt() */
```

`cpu_run` loops until `cpu_halt()` is called. **BRK does not halt** — it vectors
through `$FFFE` like a real 6502, so a program that ends in BRK needs a real IRQ
handler or an explicit halt.

`cpu_irq(&c)` and `cpu_nmi(&c)` trigger interrupts. IRQ is level-triggered and
stays pending while the I flag is set; NMI always wins. `cpu_set_variant()`
switches between `CPU_6502` and `CPU_65C02` BCD flag behavior (65C02 default).

### vmachine Memory Map

| Range         | Contents                                                     |
|---------------|--------------------------------------------------------------|
| `$0000-$BFFF` | RAM (BASIC program space starts at `$0400`)                  |
| `$C010-$C013` | ACIA #1 — console                                            |
| `$C020-$C023` | ACIA #2 — present but disconnected                           |
| `$C030-$C03F` | VIA (ports, two timers, IRQ)                                 |
| `$C040-$C04F` | File I/O device (BASIC `LOAD`/`SAVE`)                        |
| `$D000-$F7FF` | BASIC ROM                                                    |
| `$F800-$FFF9` | Extension ROM (line editor, ACIA/file glue)                  |
| `$FFFA-$FFFF` | NMI / RESET / IRQ vectors                                    |

## Gotchas

- **ROM is not decoded, it is protected.** `vmachine_t.mem` is a flat 64K array;
  `$D000-$FFFF` is read-only only because `config/*` issues `PROTECT` commands.
  Device windows are decoded *before* the protection check, so `$C010-$C04F`
  always reach hardware.
- **Unmapped addresses inside `$C000-$CFFF` behave as ordinary RAM.**
- **`tick` fires once per instruction, not once per cycle**, despite the comment
  in `v6502.h`. VIA timers therefore count instructions. The emulator is not
  cycle-accurate by design.
- **`VERBOSE` logs every ACIA byte** as `[TX: ..]` / `[RX: ..]`. `config/basic`
  enables it, which makes BASIC output very noisy — drop the line for clean runs.
- **`$FF00` character I/O and `$FF01` halt only exist in `src/hello.c`**, which
  supplies its own read/write callbacks. vmachine has neither device.
- **Editing `src/hello.s` requires vasm6502 and `xxd`** — `src/hello.h` is a
  generated `xxd -i` dump of the assembled binary, and the Makefile only
  regenerates it when `hello.h` is missing or older than `hello.s`.
- **`V6502C_TRACE` and `V6502C_VERBOSE` are globals in `v6502.c`**, not
  environment variables. The monitor toggles them via `TRACE`/`VERBOSE`.
- **Unimplemented opcodes silently do nothing.** The `default:` case in
  `cpu_step`'s switch falls through without warning, so every 65C02-only opcode
  behaves like a NOP of the right length instead of failing loudly.
- **`__CREATE_PTYS__` is defined in `utils/cli.h`, not on the compiler command
  line.** Grepping the Makefile for it finds nothing, which makes the PTY path
  look dead when it is actually the default.
- **With no script file the emulator resets and runs immediately** after a 2
  second pause, so you only reach the monitor prompt once the program halts or
  you press Ctrl+C. Pass a `config/` script to control startup.

## Test Suite

- `tests/cputest.c` — 32 instruction/flag/interrupt/BCD tests
- `tests/devtest.c` — 27 ACIA, VIA, and file I/O device tests
- `tests/addrtest.c` — 30 address-range list tests

Color-coded pass/fail output. `make test` runs all three.

## Incomplete Features

- **No 65C02 opcodes are implemented.** `inst.h` defines BRA, STZ, PHX, PHY,
  PLX, PLY, STP, WAI, TSB, TRB, and all of BBR/BBS/RMB/SMB, but none of them
  have a case in the `cpu_step` switch — they decode and then do nothing. The
  only 65C02 behavior that actually works is the BCD overflow-flag variant. Both
  65C02 addressing modes *are* implemented; nothing reaches them yet.

## Code Style

- ANSI C only (no C99+ features)
- Minimize standard library dependencies for portability to classic systems

## msbasic Directory

The `msbasic/` directory contains a port of Microsoft BASIC. Only files in
`msbasic/versions/` and `msbasic/v6502c.cfg` should be modified — the other
source files are shared by multiple BASIC versions. `versions/defines_v6502c.s`
holds the zero-page and I/O address assignments; `v6502c.cfg` holds the ROM
segment layout.
