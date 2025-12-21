v6502c
======

v6502c implements an emulated 6502 CPU in ANSI C.

## Building and Running the Emulator

```
$ make
$ ./bin/v6502c rom/basic.woz
```

## Running MSBASIC

First, start the emulator:
```10 PRINT "HELLO WORLD"
20 GOTO 10
LIST

 10 PRINT "HELLO WORLD"
 20 GOTO 10
OK
RUN
HELLO WORLD
HELLO WORLD
HELLO WORLD
HELLO WORLD
HELLO WORLD
HELLO WORLD
HELLO WORLD

BREAK IN  10
OK

$ ./bin/v6502c rom/basic.woz
ACIA1 PTY: /dev/pts/8
ACIA2 PTY: /dev/pts/9
v6502c v1.0
Copyright (c) 2025, Andrew C. Young <andrew@vaelen.org>

No script files provided, starting with default settings...

```

Then connect to the first ptty:
```
$ tio /dev/pts/8
[11:42:25.405] Connected

WRITTEN BY WEILAND & GATES

MEMORY SIZE? 
UTERMINAL WIDTH? 80

 48143 BYTES FREE


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
HELLO WORLD
HELLO WORLD
HELLO WORLD
HELLO WORLD

BREAK IN  10
OK
```

## Emulator Commands

```
v6502c v1.0
Copyright (c) 2025, Andrew C. Young <andrew@vaelen.org>

Type 'help' for help.

=> help
Commands:
  H | HELP        - show this help screen
  R | RESET       - reset CPU
  S | STEP        - step
  G | GO [10F0]   - start execution [at address 10F0 if provided]
  Q | QUIT        - quit

Working with Registers:
  ?         - print all register values
  PC [FFFF] - print or set the program counter
  A [FF]    - print or set the accumulator
  X [FF]    - print or set the X index register
  Y [FF]    - print or set the Y index register
  SR [FF]   - print or set the status register
  SP [FF]   - print or set the stack pointer

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
```

## Memory Map

The default emulator uses an Apple II-inspired memory layout:

| Address Range | Description |
|---------------|-------------|
| `$0000-$00FF` | Zero Page |
| `$0100-$01FF` | Stack |
| `$0200-$BFFF` | RAM (available for programs) |
| `$C000-$C00F` | Reserved |
| `$C010-$C01F` | Primary ACIA (6551) - stdin/stdout |
| `$C020-$C02F` | Secondary ACIA (6551) - disconnected |
| `$C030-$C03F` | VIA (6522) - timers |
| `$C040-$C04F` | File I/O device |
| `$C050-$CFFF` | Reserved for future I/O |
| `$D000-$FFFF` | ROM area (12KB) |
| `$FF00`       | Legacy character I/O (deprecated) |
| `$FFFA-$FFFB` | NMI vector |
| `$FFFC-$FFFD` | RESET vector |
| `$FFFE-$FFFF` | IRQ/BRK vector |

## Emulated Devices

### MOS 6551 ACIA (Asynchronous Communications Interface Adapter)

Two ACIA instances are available for serial I/O:

**Primary ACIA at `$C010-$C01F`** - Connected to stdin/stdout
- `$C010` - Data register (read/write)
- `$C011` - Status register (read-only)
- `$C012` - Command register
- `$C013` - Control register

**Secondary ACIA at `$C020-$C02F`** - Currently disconnected

Status register bits:
- Bit 0 (`$01`): RDRF - Receive Data Register Full
- Bit 4 (`$10`): TDRE - Transmit Data Register Empty

### MOS 6522 VIA (Versatile Interface Adapter)

Located at `$C030-$C03F`, provides two 16-bit timers with interrupt support.

Key registers:
- `$C034` / `$C035` - Timer 1 counter low/high
- `$C036` / `$C037` - Timer 1 latch low/high
- `$C038` / `$C039` - Timer 2 counter low/high
- `$C03B` - Auxiliary control (timer modes)
- `$C03D` - Interrupt enable register
- `$C03E` - Interrupt flag register

### File I/O Device

Located at `$C040-$C04F`, provides LOAD/SAVE functionality for MS BASIC.

Registers:
- `$C040` - Status register
- `$C041` - Command register (1=LOAD, 2=SAVE)
- `$C042` - Start address low byte
- `$C043` - Start address high byte
- `$C044` - End address low byte
- `$C045` - End address high byte
- `$C046` - Filename length
- `$C047` - Filename index (write index, then read/write char at that position)
- `$C048` - Filename character at current index

Status bits:
- Bit 0: Ready
- Bit 1: Error occurred
- Bit 7: Operation complete

## MS BASIC

The emulator can run Microsoft BASIC. A pre-built ROM is available in the `rom/` directory.

### Running MS BASIC

```
$ ./bin/v6502c rom/basic.v6502c
```

Or manually load and start BASIC:
```
$ ./bin/v6502c
=> LOAD rom/basic.woz
=> F01A R
```

MS BASIC will display:
```
MEMORY SIZE?
```
Press Enter to use all available memory, or enter a decimal address.

```
TERMINAL WIDTH?
```
Press Enter for default (72 columns), or enter a number.

### Building the MS BASIC ROM

The MS BASIC ROM is built from the msbasic project (https://github.com/mist64/msbasic).

Prerequisites:
- cc65 toolchain (ca65 assembler, ld65 linker)
- GNU Make

To build:
```
$ cd msbasic
$ make v6502c
```

This produces:
- `tmp/v6502c.bin` - Raw binary ROM image (12KB, loads at $D000)
- `tmp/v6502c.lst` - Assembly listing with addresses

To convert to Wozmon format:
```
$ ./bin/bin2woz D000 msbasic/tmp/v6502c.bin > rom/basic.woz
```

## Details

This project began as a port of my v6502 project, which is similar but
is written in Rust. The reason I wrote this new version in ANSI C was
so that I could use it as a component in retro computing projects for
systems such as the classic Macintosh. It is meant to be as portable
as possible and to rely on the C standard library as little as
possible so that it can be ported to as wide a variety of systems as
possible.

The emulated CPU is not intended to be cycle accurate. It won't run
demos very well, but it should be able to execute things like msbasic
or ehbasic without a problem. It should also be helpful for learning
6502 assembly language programming.

The main code is located in `src/v6502.c` and it's related header
`src/v6502.h`. A default implementation can be found in
`src/main.c`. To build the default implementation on a UNIX or
UNIX-like operating system (Linux, MacOS, FreeBSD, etc.) one should
only need to run `make`.

The code for v6502c has been released under the MIT license, which you
will find in `LICENSE` as well as at the top of each of the source
files. This should make it simple to include the library in your own
projects.

