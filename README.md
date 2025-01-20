v6502c
======

# Note: This is a work in progress! It is not yet ready to use.

v6502c implements an emulated 6502 CPU in ANSI C.

## Hello World
The file `hello.c` includes a minimal example of embedding the emulator in another program.
You can build and run it using the following commands:
```
$ make hello
$ ./bin/hello
```

## Building and Running the Emulator
```
$ make
$ ./bin/v6502c
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

