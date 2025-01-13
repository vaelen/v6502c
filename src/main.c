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

#include <stdio.h>
#include <string.h>

#include <v6502.h>

byte mem[0xFFFF];

byte read(address a) {
  return mem[a];
}

void write(address a, byte b) {
  mem[a] = b;
}

int read_line(char *buf, int maxlen) {
  int c, count = 0;

  c = getchar();
  while (c != EOF && c != '\n' && count < maxlen) {
    if (c != '\r') {
      buf[count] = toupper((unsigned char) c);
      count = count + 1;
    }
    if (count < maxlen) {
      c = getchar();
    }
  }

  if (c == EOF) {
    return EOF;
  }

  return count;
}

void print_register(char *name, byte value) {
  printf("%s : %02X\n", name, value);
}

void print_pc(address value) {
  printf("PC : %04X\n", value);
}

void print_help() {
  puts("Commands:");
  puts("  H | HELP  - show this help screen");
  puts("  R | RESET - reset CPU");
  puts("  S | STEP  - step");
  puts("  G | GO    - go");
  puts("  Q | QUIT  - quit");
  puts("");
  puts("Working with Registers:");
  puts("  ?         - print all register values");
  puts("  PC [FFFF] - print or set the program counter");
  puts("  A [FF]    - print or set the accumulator");
  puts("  X [FF]    - print or set the X index register");
  puts("  Y [FF]    - print or set the Y index register");
  puts("  SR [FF]   - print or set the status register");
  puts("  SP [FF]   - print or set the stack pointer");
  puts("");
  puts("Simple Memory Access:");
  puts("  M FFFF          - show value at address FFFF");
  puts("  M FF00.FFFF     - show values from address FF00 to FFFF");
  puts("  M FFFF FF[FE..] - set values starting at address FFFF");
  puts("  M FF00.FFFF FF  - set values from address FF00 to FFFF");
  puts("");
}

int starts_with(char *prefix, char *s) {
  return !strncmp(prefix, s, strlen(prefix));
}

int main() {

  cpu c;
  int l = 0, done = 0;
  char cmd[256];
  address a = 0;
  byte b = 0;

  c.read = read;
  c.write = write;

  cpu_reset(&c);

  puts(V6502C_VERSION);
  puts(V6502C_COPYRIGHT);
  puts("");

  puts("Type 'help' for help.");
  puts("");
  
  while (!done) {
    printf("=> ");

    l = read_line(cmd, 255);
    if (l >= 0) {
      cmd[l] = 0;
    }

    if (l == EOF) {
      done = 1;
    } else if(starts_with("H", cmd) || starts_with("HELP", cmd)) {
      print_help();
    } else if (starts_with("Q", cmd) || starts_with("QUIT", cmd)) {
      done = 1;
    } else if (starts_with("R", cmd) || starts_with("RESET", cmd)) {
      cpu_reset(&c);
    } else if (starts_with("S", cmd) || starts_with("STEP", cmd)) {
      cpu_step(&c);
    } else if (starts_with("G", cmd) || starts_with("GO", cmd)) {
      cpu_run(&c);
    } else if (starts_with("?", cmd)) {
      print_pc(c.pc);
      print_register(" A", c.a);
      print_register(" X", c.x);
      print_register(" Y", c.y);
      print_register("SR", c.sr);
      print_register("SP", c.sp);
    } else if (starts_with("PC", cmd)) {
      print_pc(c.pc);
    } else if (starts_with("A", cmd)) {
      print_register("A", c.a);
    } else if (starts_with("X", cmd)) {
      print_register("X", c.x);
    } else if (starts_with("Y", cmd)) {
      print_register("Y", c.y);
    } else if (starts_with("SR", cmd)) {
      print_register("SR", c.sr);
    } else if (starts_with("SP", cmd)) {
      print_register("SP", c.sp);
    }
    
  }
  
  return 0;
}
