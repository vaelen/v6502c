v6502c
======

# Note: This is a work in progress! It is not yet ready to use.

v6502c implements an emulated 6502 CPU in ANSI C.

It began as a port of my v6502 project, which is similar but is
written in Rust. The reason I wrote this new version in ANSI C was so
that I could use it as a component in retro computing projects for
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

