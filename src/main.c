/**
 * 
 * Main entry point for the vMachine application.
 * This is where we deal with the details of the host OS and
 * set up the emulated machine environment.
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

#include "main.h"
#include "vmachine.h"

#include <signal.h>
#include <unistd.h>
#include <ctype.h> 

#ifdef __unix__
#include <termios.h>
#include <unistd.h>
#endif

#ifdef __unix__
  struct termios orig_termios, raw_termios;
#endif

vmachine_t *g_machine;

/* CPU callback wrappers that use the global machine pointer */
static void _tick(void) {
  if (g_machine != NULL) {
    machine_tick(g_machine);
  }
}

static byte _read(address a) {
  if (g_machine != NULL) {
    return machine_read(g_machine, a);
  }
  return 0;
}

static void _write(address a, byte b) {
  if (g_machine != NULL) {
    machine_write(g_machine, a, b);
  }
}

void signal_handler(int sig) {
  switch (sig) {
  case SIGINT:
    if (g_machine != NULL) {
      puts("BREAK");
      cpu_halt(&g_machine->c);
    }
    signal(SIGINT, signal_handler);
  }
}

int main(int argc, char** argv) {
  int i;
  vmachine_t machine;

#ifdef __unix__
  /* Create a terminal configuration that switches to raw mode for immediate character input */
  tcgetattr(STDIN_FILENO, &orig_termios);
  raw_termios = orig_termios;
  raw_termios.c_lflag &= ~(ICANON | ECHO);  /* Disable canonical mode and echo */
  raw_termios.c_cc[VMIN] = 1;   /* Read returns after 1 byte */
  raw_termios.c_cc[VTIME] = 0;  /* No timeout */
  /* Enable NL to NLCR mapping and vice versa */
  raw_termios.c_iflag &= ~ICRNL;
  raw_termios.c_oflag &= ~OCRNL;
  raw_termios.c_oflag |= (OPOST | ONLCR);

  setvbuf(stdout, NULL, _IONBF, 0);
#endif


  init_vmachine(&machine);
  g_machine = &machine;
  machine.c.read = _read;
  machine.c.write = _write;
  machine.c.tick = _tick;

  signal(SIGINT, signal_handler);

  puts(V6502C_VERSION);
  puts(V6502C_COPYRIGHT);
  puts("");

  /* Process command-line files as scripts */
  for (i = 1; i < argc; i++) {
    read_file(&machine, argv[i]);
  }

  puts("Type 'help' for help.");
  puts("");

  run_vmachine(&machine);
  cleanup_vmachine(&machine);

  return 0;
}
