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

typedef struct addrr {
  address start;
  address end;
} address_range;

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

void print_register_change(char *name, byte old, byte new) {
  printf("%s : %02X -> %02X\n", name, old, new);
}

void print_pc(address value) {
  printf("PC : %04X\n", value);
}

void print_pc_change(address old, address new) {
  printf("PC : %04X -> %04X\n", old, new);
}

void print_memory_header() {
  puts("       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
}

void print_memory_location(address a) {
  printf("%04X: ", a);
}

void print_memory(cpu *c, address start, address end) {
  address current = start & 0xFFF0;
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
    if (current >= start) {
      printf("%02X ", c->read(current));
    } else {
      printf("   ");
    }
  
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

/** Parameter parsing */
void parseargs(char *cmdbuf, int *argc, char **argv) {
  int count = 0;
  char *current, *end;

  if (argv == 0 || argc == 0 || cmdbuf == 0) return;

  current = cmdbuf;
  end = cmdbuf + strlen(cmdbuf);
  
  /** tokenize by whitespace */

  argv[count] = 0;
  
  while (current < end) {
    if (is_whitespace(*current)) {
      if (argv[count] != 0) {
	/* Found the last non-whitepace character */
	*current = 0;
	count = count + 1;
	argv[count] = 0;
      }
    } else {
	if (argv[count] == 0) {
	  /* Found the first non-whitespace character */
	  argv[count] = current;
	}
    }
    current = current + 1;
  }

 if (argv[count] != 0) {
   count = count + 1;
 }
  
  *argc = count;
}

/** Parse a byte value in hexadecimal */
int parse_byte(char *s, byte *b) {
  int n;
  unsigned int v;
  n = sscanf(s, "%2x", &v);
  if (n < 1) {
    n = sscanf(s, "%2X", &v);
    if (n < 1) {
      return 0;
    }
  }
  *b = (byte) v;
  return 1;
}

/** Parse an address value in hexadecimal */
int parse_address(char *s, address *a) {
  int n;
  unsigned int v;
  n = sscanf(s, "%4x", &v);
  if (n < 1) {
    n = sscanf(s, "%4X", &v);
    if (n < 1) {
      return 0;
    }
  }
  *a = (address) v;
  return 1;
}

/** Parse an address range value in hexadecimal */
int parse_address_range(char *s, address_range *r) {
  int n;
  unsigned int start, end;
  n = sscanf(s, "%4x.%4x", &start, &end);
  if (n < 2) {
    n = sscanf(s, "%4X.%4X", &start, &end);
    if (n < 2) {
      return 0;
    }
  }
  r->start = (address) start;
  r->end = (address) end;
  return 1;
}

int main() {

  cpu c;
  int l = 0, done = 0, cmdlen = 0, argc = 0, i = 0, editing = 0, firstarg = 0;
  char cmdbuf[256], *cmd, *argv[256], *arg;
  address a = 0;
  byte b = 0;
  address_range ar;

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

    parseargs(cmdbuf, &argc, argv);

    cmd = argv[0];
    if (cmd == 0) {
      cmd = "";
    }

    /*
    printf("Command: %s\n", cmd);
    for (i = 0; i < argc; i++) {
      printf("  Param %d: '%s'\n", i, argv[i]);
    }
    */
    
    cmdlen = strlen(cmd);
    
    if (l == EOF) {
      done = 1;
    } else if (cmdlen == 0) {
      /** Empty command, do nothing. */
    } else if (!strcmp("H", cmd) || !strcmp("HELP", cmd)) {
      print_help();
    } else if (!strcmp("Q", cmd) || !strcmp("QUIT", cmd)) {
      done = 1;
    } else if (!strcmp("R", cmd) || !strcmp("RESET", cmd)) {
      cpu_reset(&c);
    } else if (!strcmp("S", cmd) || !strcmp("STEP", cmd)) {
      cpu_step(&c);
    } else if (!strcmp("G", cmd) || !strcmp("GO", cmd)) {
      cpu_run(&c);
    } else if (!strcmp("?", cmd)) {
      print_pc(c.pc);
      print_register(" A", c.a);
      print_register(" X", c.x);
      print_register(" Y", c.y);
      print_register("SR", c.sr);
      print_register("SP", c.sp);
    } else if (!strcmp("PC", cmd)) {
      if (argc == 1) {
	print_pc(c.pc);
      } else {
	a = c.pc;
	if (!parse_address(argv[1], &c.pc)) {
	  printf("Invalid value: %s\n", argv[1]);
	} else {
	  print_pc_change(a, c.pc);
	}
      }
    } else if (!strcmp("A", cmd)) {
      if (argc == 1) {
	print_register("A", c.a);
      } else {
	b = c.a;
	if (!parse_byte(argv[1], &c.a)) {
	  printf("Invalid value: %s\n", argv[1]);
	} else {
	  print_register_change("A", b, c.a);
	}
      }
    } else if (!strcmp("X", cmd)) {
      if (argc == 1) {
	print_register("X", c.x);
      } else {
	b = c.x;
	if (!parse_byte(argv[1], &c.x)) {
	  printf("Invalid value: %s\n", argv[1]);
	} else {
	  print_register_change("X", b, c.x);
	}
      }
    } else if (!strcmp("Y", cmd)) {
      if (argc == 1) {
	print_register("Y", c.y);
      } else {
	b = c.y;
	if (!parse_byte(argv[1], &c.y)) {
	  printf("Invalid value: %s\n", argv[1]);
	} else {
	  print_register_change("Y", b, c.y);
	}
      }
    } else if (!strcmp("SR", cmd)) {
      if (argc == 1) {
	print_register("SR", c.sr);
      } else {
	b = c.sr;
	if (!parse_byte(argv[1], &c.sr)) {
	  printf("Invalid value: %s\n", argv[1]);
	} else {
	  print_register_change("SR", b, c.sr);
	}
      }
    } else if (!strcmp("SP", cmd)) {
      if (argc == 1) {
	print_register("SP", c.sp);
      } else {
	b = c.sp;
	if (!parse_byte(argv[1], &c.sp)) {
	  printf("Invalid value: %s\n", argv[1]);
	} else {
	  print_register_change("SP", b, c.sp);
	}
      }
    } else if (!strcmp("M", cmd)) {
      if (argc == 1) {
	print_memory(&c, 0x0000, 0x00FF);
      } else {
	editing = 0;
	a = 0;
	ar.start = 0;
	ar.end = 0;
	for (i = 1; i < argc; i++) {
	  arg = argv[i];
	  if (!editing) {
	    if (parse_address_range(arg, &ar)) {
	      if (arg[strlen(arg)-1] == ':') {
		editing = 1;
		a = ar.start;
		firstarg = i;
	      } else {
		print_memory(&c, ar.start, ar.end);
	      }
	    } else if (parse_address(arg, &a)) {
	      if (arg[strlen(arg)-1] == ':') {
		editing = 2;
	      } else {
		print_memory(&c, a, a);
	      }
	    } else {
	      printf("Invalid value: %s\n", argv[1]);
	    }
	  } else {
	    /* now we should only get bytes */
	    if (parse_byte(arg, &b)) {
	      c.write(a, b);
	      a = a + 1;
	      if (editing == 1) {
		if (a > ar.end) {
		  i = argc; /* skip the rest */
		} else if (i == (argc - 1)) {
		  /* we've reached the end of the input,
		     but not the end of the addresses */
		  i = firstarg;
		}
	      }
	    }
	  }
	} /* for i = 1 to argc */
      }
    } else if (!strcmp("IMPORT", cmd)) {
      not_implemented();
    } else if (!strcmp("EXPORT", cmd)) {
      not_implemented();
    } else {
      printf("Invalid command: %s\n", cmd);
    }
    
  }
  
  return 0;
}
