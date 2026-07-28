v6502c
======

v6502c implements an emulated 6502 CPU in ANSI C.

The project is three layers: a portable CPU core (`src/v6502.c`), a set of
emulated peripherals (`src/devices.c`), and an example machine that wires them
together into an Apple II-inspired computer capable of running Microsoft BASIC.
All of it builds into `libv6502`, so you can embed just the CPU or the whole
machine.

## Building

```
$ make          # library, emulator, hello example, bin2woz
$ make test     # build and run the three test suites
$ make clean
```

Requirements:

- clang-18, or edit `CC` in the Makefile for gcc
- vasm6502 (oldstyle) — optional, for assembling the `.s` files
- cc65 — optional, for rebuilding the MS BASIC ROM

## Running

```
$ ./bin/v6502c <romfile> [scriptfile...]
```

The first argument is a ROM image in Wozmon format, loaded at `$D000`. Any
further arguments are monitor script files — plain text files of monitor
commands executed before the interactive prompt appears. Ready-made ones live in
`config/`.

With no script file, the emulator resets and runs the ROM immediately after a
two second pause, and you only reach the monitor prompt once the program halts
or you press Ctrl+C.

By default each ACIA is given its own pseudo-terminal, whose device name is
printed at startup. The monitor stays on stdin, so you drive the emulated
machine from one terminal and the debugger from another. To wire the primary
ACIA to stdin/stdout instead, comment out `#define __CREATE_PTYS__` in
`utils/cli.h` — but note that BASIC and the monitor then compete for the same
input.

## Running MS BASIC

Start the emulator with the BASIC startup script:

```
$ ./bin/v6502c rom/basic.woz config/basic
Loaded ROM: rom/basic.woz, Size: 12288 bytes
ACIA1 PTY: /dev/pts/8
ACIA2 PTY: /dev/pts/9
v6502c v1.0
Copyright (c) 2025, Andrew C. Young <andrew@vaelen.org>

Processing command-line script files...
Loading config/basic
Protecting memory range D000.FFFF
```

Then connect a terminal to the first PTY:

```
$ tio /dev/pts/8
[11:42:25.405] Connected

WRITTEN BY WEILAND & GATES

TERMINAL WIDTH? 80

 48127 BYTES FREE


COPYRIGHT 1977 BY MICROSOFT CO.

OK
10 PRINT "HELLO WORLD"
20 GOTO 10
LIST

 10 PRINT "HELLO WORLD"
 20 GOTO 10
OK
RUN
HELLO WORLD
HELLO WORLD
HELLO WORLD

BREAK IN  10
OK
```

There is no `MEMORY SIZE?` prompt — this build skips the memory probe and uses a
fixed 48K.

`config/basic` enables `VERBOSE`, which logs every byte through the ACIA as
`[TX: ..]` / `[RX: ..]` on the monitor terminal. Delete that line from the script
for quiet operation.

To start BASIC by hand instead:

```
$ ./bin/v6502c rom/basic.woz
=> PROTECT D000.FFFF
=> F01A R
```

## Emulator Commands

```
=> help
Commands:
  H | HELP         - show this help screen
  R | RESET        - reset CPU
  S | STEP         - step
  G | GO [10F0]    - start execution [at address 10F0 if provided]
  T | TRACE [10F0] - start execution and print all changes to CPU state
  V | VERBOSE      - toggle verbose output
  Q | QUIT         - quit

Working with Registers:
  ?         - print all register values
  PC [FFFF] - print or set the program counter
  A [FF]    - print or set the accumulator
  X [FF]    - print or set the X index register
  Y [FF]    - print or set the Y index register
  SR [FF]   - print or set the status register
  SP [FF]   - print or set the stack pointer
  CPU [6502|65C02] - print or set CPU variant for BCD behavior

Memory Access (Wozmon Compatible)
  FFFF            - print value at address FFFF
  FF00.FFFF       - print values of addresses FF00 to FFFF
  FFFF: FF [FE..] - set values starting at address FFFF
  FF00.FFFF: FF   - set addresses FF00 to FFFF to the value FF
  .FFFF           - print values from last used addresses to FFFF
  :FF [FE..]      - set the value FF starting at last used address
  10F0 R          - start execution at address 10F0 (alias for GO)

Data Import / Export:
  LOAD <FILENAME>           - Load Wozmon formatted data.
  SAVE 1000.10F0 <FILENAME> - Save data in Wozmon format.
  PROTECT D000.FFFF         - Protect memory range from writes.
  UNPROTECT D000.FFFF       - Unprotect memory range for writes.
```

Script files contain these same commands. Lines starting with `;` are comments.

## Memory Map

The default machine uses an Apple II-inspired memory layout:

| Address Range | Description                                     |
|---------------|-------------------------------------------------|
| `$0000-$00FF` | Zero page                                       |
| `$0100-$01FF` | Stack                                           |
| `$0200-$03FF` | Free RAM                                        |
| `$0400-$BFFF` | RAM — BASIC program and variable space          |
| `$C000-$C00F` | Unmapped                                        |
| `$C010-$C013` | Primary ACIA (6551)                             |
| `$C020-$C023` | Secondary ACIA (6551) — no backing stream       |
| `$C030-$C03F` | VIA (6522) — ports and timers                   |
| `$C040-$C04F` | File I/O device                                 |
| `$C050-$CFFF` | Unmapped, reserved for future I/O               |
| `$D000-$F7FF` | BASIC ROM                                       |
| `$F800-$FFF9` | Extension ROM — line editor and I/O routines    |
| `$FFFA-$FFFB` | NMI vector                                      |
| `$FFFC-$FFFD` | RESET vector                                    |
| `$FFFE-$FFFF` | IRQ/BRK vector                                  |

ROM is read-only because `config/*` issues `PROTECT` commands, not because of
address decoding — the machine's memory is a flat 64K array with the device
windows layered on top. Unmapped addresses in the `$C000` page behave as
ordinary RAM.

## Emulated Devices

### MOS 6551 ACIA

Two instances. The primary at `$C010` carries the console; the secondary at
`$C020` has no backing stream in the default build and always reports that no
data is ready.

| Address | Register                                    |
|---------|---------------------------------------------|
| `$C010` | Data — receive on read, transmit on write   |
| `$C011` | Status on read, programmed reset on write   |
| `$C012` | Command                                     |
| `$C013` | Control                                     |

Status register bits:

| Bit | Mask  | Meaning                             |
|-----|-------|-------------------------------------|
| 0   | `$01` | Parity error                        |
| 1   | `$02` | Framing error                       |
| 2   | `$04` | Overrun                             |
| 3   | `$08` | RDRF — receive data register full   |
| 4   | `$10` | TDRE — transmit data register empty |
| 5   | `$20` | DCD                                 |
| 6   | `$40` | DSR                                 |
| 7   | `$80` | IRQ                                 |

### MOS 6522 VIA

Located at `$C030-$C03F`, with two 16-bit timers and interrupt support.

| Address | Register                        |
|---------|---------------------------------|
| `$C030` | Port B                          |
| `$C031` | Port A                          |
| `$C032` | Data direction B                |
| `$C033` | Data direction A                |
| `$C034` | Timer 1 counter low             |
| `$C035` | Timer 1 counter high            |
| `$C036` | Timer 1 latch low               |
| `$C037` | Timer 1 latch high              |
| `$C038` | Timer 2 counter low             |
| `$C039` | Timer 2 counter high            |
| `$C03A` | Shift register                  |
| `$C03B` | Auxiliary control (timer modes) |
| `$C03C` | Peripheral control              |
| `$C03D` | Interrupt flag register         |
| `$C03E` | Interrupt enable register       |
| `$C03F` | Port A, no handshake            |

Timers count down once per instruction rather than once per clock cycle, since
the emulator is not cycle-accurate.

### File I/O Device

Located at `$C040-$C04F`, this is a made-up device that gives MS BASIC `LOAD`
and `SAVE`. A filename is written one character at a time, then a command byte
opens, transfers, and closes the file.

| Address | Register                                  |
|---------|-------------------------------------------|
| `$C040` | Status on read, command on write          |
| `$C041` | Data — read a byte, or write a byte       |
| `$C042` | Filename index                            |
| `$C043` | Filename character at the current index   |

Commands written to `$C040`:

| Value | Command        |
|-------|----------------|
| `$00` | Reset / close  |
| `$01` | Open for read  |
| `$02` | Open for write |
| `$03` | Read byte      |
| `$04` | Write byte     |
| `$05` | Close file     |

Status bits read from `$C040`:

| Bit | Mask  | Meaning     |
|-----|-------|-------------|
| 0   | `$01` | File open   |
| 1   | `$02` | End of file |
| 2   | `$04` | Error       |
| 7   | `$80` | Ready       |

## Building the MS BASIC ROM

The ROM in `rom/basic.woz` is prebuilt, so this is only needed if you change the
BASIC sources. It is built from the msbasic project
(https://github.com/mist64/msbasic) and requires the cc65 toolchain.

```
$ cd msbasic && ./make.sh
$ cd .. && ./bin/bin2woz D000 msbasic/tmp/v6502c.bin > rom/basic.woz
```

This produces `msbasic/tmp/v6502c.bin`, a 12KB image that loads at `$D000`, plus
a `.lbl` label file.

Only files under `msbasic/versions/` and `msbasic/v6502c.cfg` are specific to
this project; everything else in `msbasic/` is shared with the other BASIC
variants and should not be edited. `versions/defines_v6502c.s` holds the zero
page and I/O addresses, and `v6502c.cfg` holds the ROM segment layout.

## Embedding

The CPU core has no notion of memory layout. Supply read and write callbacks and
it will run anything:

```c
#include <v6502.h>

byte mem[0x10000];

byte read(address a) { return mem[a]; }
void write(address a, byte b) { mem[a] = b; }

int main(void) {
    cpu c;
    cpu_init(&c);
    c.read = read;
    c.write = write;
    /* load a program, set the reset vector at $FFFC... */
    cpu_reset(&c);
    cpu_run(&c);
    return 0;
}
```

`cpu_run` executes until `cpu_halt()` is called. BRK does not stop the CPU — it
vectors through `$FFFE` as it does on real hardware. `cpu_irq()` and `cpu_nmi()`
raise interrupts, and `cpu_set_variant()` chooses between `CPU_6502` and
`CPU_65C02` decimal-mode flag behavior.

See `src/hello.c` for a complete example. It maps two addresses: `$FF00` as a
character device, and `$FF01` as a halt register whose write callback calls
`cpu_halt()`. `src/hello.s` writes to the latter to end the program.

## Testing

```
$ make test
```

Three suites, 89 tests: `tests/cputest.c` covers instructions, flags,
interrupts, and BCD mode for both CPU variants; `tests/devtest.c` covers the
three devices; `tests/addrtest.c` covers the address-range list used for write
protection.

## Status

The full standard 6502 instruction set is implemented, including decimal mode
and interrupts. The 65C02 additions — BRA, STZ, PHX/PHY/PLX/PLY, STP, WAI, TSB,
TRB, and the BBR/BBS/RMB/SMB families — are present in the opcode tables but not
yet in the execution switch, so they currently behave as no-ops. The only 65C02
behavior that works today is the decimal-mode overflow flag.

## Details

This project began as a port of my v6502 project, which is similar but is
written in Rust. The reason I wrote this new version in ANSI C was so that I
could use it as a component in retro computing projects for systems such as the
classic Macintosh. It is meant to be as portable as possible and to rely on the
C standard library as little as possible so that it can be ported to as wide a
variety of systems as possible.

The emulated CPU is not intended to be cycle accurate. It won't run demos very
well, but it should be able to execute things like msbasic or ehbasic without a
problem. It should also be helpful for learning 6502 assembly language
programming.

The CPU core is `src/v6502.c` and its header `src/v6502.h`. The example machine
is `src/vmachine.c` and the monitor is `src/monitor.c`; `utils/cli.c` ties them
together into the `bin/v6502c` binary. On a UNIX or UNIX-like operating system
(Linux, macOS, FreeBSD, etc.) running `make` should be all that is needed.

The code for v6502c has been released under the MIT license, which you will find
in `LICENSE` as well as at the top of each of the source files. This should make
it simple to include the library in your own projects.
