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

void print_memory_header() {
  puts("       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
}

void print_memory_location(address a) {
  printf("%04X: ", a);
}

void print_memory(cpu *c, address start, address end) {
  address current = start;
  int column = 0, row = 0;
  byte b = 0;

  while (current <= end) {
    if (column == 0) {
      if (current != start) {
	/* new line */
	puts("");
      }
      if (row == 0) {
	print_memory_header();
      }
      row = (row + 1) % 23;
      print_memory_location(current);
    }
    printf("%02X ", c->read(current));

    current = current + 1;
    column = (column + 1) % 16;
  }
  puts("");
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
  puts("Memory Access:");
  puts("  M                - print values of addresses 0000 to 00FF");
  puts("  M FFFF           - print value at address FFFF");
  puts("  M FF00.FFFF      - print values of addresses FF00 to FFFF");
  puts("  M FFFF: FF[FE..] - set values starting at address FFFF");
  puts("  M FF00.FFFF: FF  - set addresses FF00 to FFFF to the value FF");
  puts("");
  puts("Data Import / Export:");
  puts("  IMPORT SREC - Import Motorola S-Record formatted data.");
  puts("  EXPORT SREC - Export Motorola S-Record formatted data.");
}

int starts_with(char *prefix, char *s) {
  return !strncmp(prefix, s, strlen(prefix));
}

void not_implemented() {
  puts("Not Yet Implemented");
}

int is_whitespace(char c) {
  switch (c) {
  case '\n':
  case '\r':
  case '\t':
  case '\f':
  case '\v':
  case ' ':
    return 1;
  }
  return 0;
}

/** Removes leading whitespace **/
char *ltrim(char *s) {
  char *trimmed = s;
  while (trimmed[0] != 0) {
    if (is_whitespace(trimmed[0])) {
      trimmed = trimmed + 1;
    } else {
      return trimmed;
    }
  }
  return trimmed;
}

/** Removes trailing whitespace **/
void rtrim(char *s) {
  int i = strlen(s);
  while (i > 0) {
    i = i - 1;
    if (is_whitespace(s[i])) {
      s[i] = 0;
    } else {
      return;
    }
  }
}

int main() {

  cpu c;
  int l = 0, done = 0, cmdlen = 0;
  char cmdbuf[256], *cmd;
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

    l = read_line(cmdbuf, 255);
    if (l >= 0) {
      cmdbuf[l] = 0;
    }

    cmd = cmdbuf;

    cmd = ltrim(cmd);
    rtrim(cmd);

    puts(cmd);
    
    cmdlen = strlen(cmd);
    
    if (l == EOF) {
      done = 1;
    } else if (cmdlen == 0) {
      /** Empty command, do nothing. */
    } else if ((cmdlen == 1 && starts_with("H", cmd)) ||
	       starts_with("HELP", cmd)) {
      print_help();
    } else if ((cmdlen == 1 && starts_with("Q", cmd)) ||
	       starts_with("QUIT", cmd)) {
      done = 1;
    } else if ((cmdlen == 1 && starts_with("R", cmd)) ||
	       starts_with("RESET", cmd)) {
      cpu_reset(&c);
    } else if ((cmdlen == 1 && starts_with("S", cmd)) ||
	       starts_with("STEP", cmd)) {
      cpu_step(&c);
    } else if ((cmdlen == 1 && starts_with("G", cmd)) ||
	       (cmdlen == 2 && starts_with("GO", cmd))) {
      cpu_run(&c);
    } else if (cmdlen == 1 && starts_with("?", cmd)) {
      print_pc(c.pc);
      print_register(" A", c.a);
      print_register(" X", c.x);
      print_register(" Y", c.y);
      print_register("SR", c.sr);
      print_register("SP", c.sp);
    } else if (cmdlen == 2 && starts_with("PC", cmd)) {
      print_pc(c.pc);
    } else if (starts_with("PC ", cmd)) {
      not_implemented();
    } else if (cmdlen == 1 && starts_with("A", cmd)) {
      print_register("A", c.a);
    } else if (starts_with("PC ", cmd)) {
      not_implemented();
    } else if (cmdlen == 1 && starts_with("X", cmd)) {
      print_register("X", c.x);
    } else if (starts_with("PC ", cmd)) {
      not_implemented();
    } else if (cmdlen == 1 && starts_with("Y", cmd)) {
      print_register("Y", c.y);
    } else if (starts_with("PC ", cmd)) {
      not_implemented();
    } else if (cmdlen == 2 && starts_with("SR", cmd)) {
      print_register("SR", c.sr);
    } else if (starts_with("PC ", cmd)) {
      not_implemented();
    } else if (cmdlen == 2 && starts_with("SP", cmd)) {
      print_register("SP", c.sp);
    } else if (starts_with("PC ", cmd)) {
      not_implemented();
    } else if (cmdlen == 1 && starts_with("M", cmd)) {
      print_memory(&c, 0x0000, 0x00FF);
    } else if (starts_with("M ", cmd)) {
      not_implemented();
    } else if (starts_with("IMPORT ", cmd)) {
      not_implemented();
    } else if (starts_with("EXPORT ", cmd)) {
      not_implemented();
    } else {
      printf("Invalid command: %s\n", cmd);
    }
    
  }
  
  return 0;
}
