/**
 *
 * System Monitor / UI for the vMachine.
 * Provides an interactive command-line interface for controlling
 * the virtual machine, inspecting memory, and debugging code.
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

#include "monitor.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* Monitor entry point - starts the interactive REPL */
void monitor_run(vmachine_t *machine) {
  monitor_repl(machine, stdin);
}

/* Monitor REPL - reads commands from a file or stdin */
void monitor_repl(vmachine_t *machine, FILE *in) {
  int l = 0;
  bool done = FALSE;
  char cmdbuf[256];

  while (!done) {
    if (in == stdin) {
      printf("=> ");
    }

    l = read_line(in, cmdbuf, 255);
    if (l >= 0) {
      cmdbuf[l] = 0;
    }

    if (l == EOF) {
      /* Is this really an EOF, or just an error? */
      if (feof(in)) {
        done = TRUE;
      }
    } else if (parse_command(machine, cmdbuf)) {
        done = TRUE;
    }
  }
}

/* Trace callback for use with vmachine_t.trace_fn */
void monitor_trace_fn(vmachine_t *machine, cpu *prevc, cpu *c) {
  print_pc_change(prevc->pc, c->pc);
  print_register_change(" A", prevc->a, c->a);
  print_register_change(" X", prevc->x, c->x);
  print_register_change(" Y", prevc->y, c->y);
  print_register_change("SR", prevc->sr, c->sr);
  print_register_change("SP", prevc->sp, c->sp);
}

/* Input/output utilities */

int read_line(FILE *in, char *buf, int maxlen) {
  int c = 0, count = 0;

  c = fgetc(in);
  while (c != EOF && c != '\n' && c != '\r' && count < maxlen) {
    buf[count] = (char) c;
    count++;
    if (count < maxlen) {
      c = fgetc(in);
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
  if (old == new) {
    return;
  }
  printf("%s : %02X -> %02X\n", name, old, new);
}

void print_pc(address value) {
  printf("PC : %04X\n", value);
}

void print_pc_change(address old, address new) {
  if (old == new) {
    return;
  }
  printf("PC : %04X -> %04X\n", old, new);
}

void print_memory_header(void) {
  puts("       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
}

void print_memory_location(address a) {
  printf("%04X: ", a);
}

void print_memory(cpu *c, address start, address end) {
  address current = start & 0xFFF0;
  int column = 0, row = 0;

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

    if (current == 0xFFFF) {
      break;
    }

    current++;
    column = (column + 1) % 16;
  }
  puts("");
}

void print_help(void) {
  puts("Commands:");
  puts("  H | HELP         - show this help screen");
  puts("  R | RESET        - reset CPU");
  puts("  S | STEP         - step");
  puts("  G | GO [10F0]    - start execution [at address 10F0 if provided]");
  puts("  T | TRACE [10F0] - start execution and print all changes to CPU state");
  puts("  V | VERBOSE      - toggle verbose output");
  puts("  Q | QUIT         - quit");
  puts("");
  puts("Working with Registers:");
  puts("  ?         - print all register values");
  puts("  PC [FFFF] - print or set the program counter");
  puts("  A [FF]    - print or set the accumulator");
  puts("  X [FF]    - print or set the X index register");
  puts("  Y [FF]    - print or set the Y index register");
  puts("  SR [FF]   - print or set the status register");
  puts("  SP [FF]   - print or set the stack pointer");
  puts("  CPU [6502|65C02] - print or set CPU variant for BCD behavior");
  puts("");
  puts("Memory Access (Wozmon Compatible)");
  puts("  FFFF            - print value at address FFFF");
  puts("  FF00.FFFF       - print values of addresses FF00 to FFFF");
  puts("  FFFF: FF [FE..] - set values starting at address FFFF");
  puts("  FF00.FFFF: FF   - set addresses FF00 to FFFF to the value FF");
  puts("  .FFFF           - print values from last used addresses to FFFF");
  puts("  :FF [FE..]      - set the value FF starting at last used address");
  puts("  10F0 R          - start execution at address 10F0 (alias for GO)");
  puts("");
  puts("Data Import / Export:");
  puts("  LOAD <FILENAME>           - Load Wozmon formatted data.");
  puts("  SAVE 1000.10F0 <FILENAME> - Save data in Wozmon format.");
  puts("  PROTECT D000.FFFF         - Protect memory range from writes.");
  puts("  UNPROTECT D000.FFFF       - Unprotect memory range for writes.");
}

void not_implemented(void) {
  puts("Not Yet Implemented");
}

/* Parsing utilities */

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

/* Parameter parsing */
void parseargs(char *cmdbuf, int *argc, char **argv) {
  int count = 0;
  char *current = NULL, *end = NULL;

  if (argv == NULL || argc == NULL || cmdbuf == NULL) return;

  current = cmdbuf;
  end = cmdbuf + strlen(cmdbuf);

  /* tokenize by whitespace */

  argv[count] = 0;

  while (current < end) {
    if (is_whitespace(*current)) {
      if (argv[count] != 0) {
        /* Found the last non-whitepace character */
        *current = 0;
        count++;
        argv[count] = 0;
      }
    } else {
      if (argv[count] == 0) {
        /* Found the first non-whitespace character */
        argv[count] = current;
      }
    }
    current++;
  }

 if (argv[count] != 0) {
   count++;
 }

  *argc = count;
}

/* Parse a byte value in hexadecimal */
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

/* Parse an address value in hexadecimal */
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

/* Parse an address range value in hexadecimal */
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

/* File I/O commands */

int write_file(vmachine_t *machine, address_range ar, char *filename) {
  cpu *c = &machine->c;
  FILE *file = NULL;
  address a = 0;
  int i = 0;

  printf("Writing %04X.%04X to %s\n", ar.start, ar.end, filename);

  file = fopen(filename, "w");
  if (file == NULL) {
    printf("Could not open file: %s\n", filename);
    return 0;
  }

  i = 0;
  a = ar.start;
  fprintf(file, "%04X:", a);
  while (a <= ar.end) {
    fprintf(file, " %02X", c->read(a));
    a++;
    i++;
    if (a <= ar.end && (i % 8) == 0) {
      fprintf(file, "\n%04X:", a);
    }
  }

  fprintf(file, "\n");

  fflush(file);
  fclose(file);

  return i;
}

int read_file(vmachine_t *machine, char *filename) {
  FILE *file = NULL;

  printf("Loading %s\n", filename);

  file = fopen(filename, "r");
  if (file == NULL) {
    printf("Could not open file: %s\n", filename);
    return 0;
  }

  monitor_repl(machine, file);

  fclose(file);

  return 0;
}

/* Command parsing */

int parse_command(vmachine_t *machine, char *cmdbuf) {
  cpu *c = &machine->c;
  int cmdlen = 0, argc = 0, i = 0, firstarg = 0;
  enum {NOT_EDITING, EDITING, EDITING_RANGE} editing = NOT_EDITING;
  char *cmd = NULL, *argv[256], *arg = NULL, *filename = NULL;
  static address a = 0;
  address current = 0;
  byte b = 0;
  address_range ar;
  char *p = cmdbuf;

  /* Skip leading whitespace */
  while (*p && is_whitespace(*p)) {
    p++;
  }

  /* Ignore comment lines starting with ';' */
  if (*p == ';') {
    return 0;
  }

  parseargs(p, &argc, argv);

  cmd = argv[0];
  if (cmd == NULL) {
    cmd = "";
  }

  cmdlen = strlen(cmd);

  for (i = 0; i < cmdlen; i++) {
    cmd[i] = toupper((unsigned char) cmd[i]);
  }

  if (cmdlen == 0) {
    /* Empty command, do nothing. */
  } else if (!strcmp("H", cmd) || !strcmp("HELP", cmd)) {
    print_help();
  } else if (!strcmp("Q", cmd) || !strcmp("QUIT", cmd)) {
    return 1;
  } else if (!strcmp("R", cmd) || !strcmp("RESET", cmd)) {
    cpu_reset(c);
    cpu_step(c);
  } else if (!strcmp("S", cmd) || !strcmp("STEP", cmd)) {
    cpu_step(c);
  } else if (!strcmp("G", cmd) || !strcmp("GO", cmd) ||
             !strcmp("T", cmd) || !strcmp("TRACE", cmd)) {
    V6502C_TRACE = (!strcmp("T", cmd) || !strcmp("TRACE", cmd));
    machine->prevc = machine->c;
    if (argc > 1) {
      current = c->pc;
      if (parse_address(argv[1], &c->pc)) {
        cpu_run(c);
      } else {
        printf("Invalid address: %s\n", argv[1]);
      }
    } else {
      cpu_run(c);
    }
  } else if (!strcmp("V", cmd) || !strcmp("VERBOSE", cmd)) {
    V6502C_VERBOSE = !V6502C_VERBOSE;
    printf("Verbose output %s\n", V6502C_VERBOSE ? "enabled" : "disabled");
  } else if (!strcmp("?", cmd)) {
    print_pc(c->pc);
    print_register(" A", c->a);
    print_register(" X", c->x);
    print_register(" Y", c->y);
    print_register("SR", c->sr);
    print_register("SP", c->sp);
  } else if (!strcmp("PC", cmd)) {
    if (argc == 1) {
      print_pc(c->pc);
    } else {
      current = c->pc;
      if (!parse_address(argv[1], &c->pc)) {
        printf("Invalid address: %s\n", argv[1]);
      } else {
        print_pc_change(current, c->pc);
      }
    }
  } else if (!strcmp("A", cmd)) {
    if (argc == 1) {
      print_register("A", c->a);
    } else {
      b = c->a;
      if (!parse_byte(argv[1], &c->a)) {
        printf("Invalid value: %s\n", argv[1]);
      } else {
        print_register_change("A", b, c->a);
      }
    }
  } else if (!strcmp("X", cmd)) {
    if (argc == 1) {
      print_register("X", c->x);
    } else {
      b = c->x;
      if (!parse_byte(argv[1], &c->x)) {
        printf("Invalid value: %s\n", argv[1]);
      } else {
        print_register_change("X", b, c->x);
      }
    }
  } else if (!strcmp("Y", cmd)) {
    if (argc == 1) {
      print_register("Y", c->y);
    } else {
      b = c->y;
      if (!parse_byte(argv[1], &c->y)) {
        printf("Invalid value: %s\n", argv[1]);
      } else {
        print_register_change("Y", b, c->y);
      }
    }
  } else if (!strcmp("SR", cmd)) {
    if (argc == 1) {
      print_register("SR", c->sr);
    } else {
      b = c->sr;
      if (!parse_byte(argv[1], &c->sr)) {
        printf("Invalid value: %s\n", argv[1]);
      } else {
        print_register_change("SR", b, c->sr);
      }
    }
  } else if (!strcmp("SP", cmd)) {
    if (argc == 1) {
      print_register("SP", c->sp);
    } else {
      b = c->sp;
      if (!parse_byte(argv[1], &c->sp)) {
        printf("Invalid value: %s\n", argv[1]);
      } else {
        print_register_change("SP", b, c->sp);
      }
    }
  } else if (!strcmp("CPU", cmd)) {
    if (argc == 1) {
      printf("CPU : %s\n", (c->variant == CPU_6502) ? "6502" : "65C02");
    } else {
      if (!strcmp("6502", argv[1])) {
        cpu_set_variant(c, CPU_6502);
        printf("CPU : 65C02 -> 6502\n");
      } else if (!strcmp("65C02", argv[1])) {
        cpu_set_variant(c, CPU_65C02);
        printf("CPU : 6502 -> 65C02\n");
      } else {
        printf("Invalid CPU variant: %s (use 6502 or 65C02)\n", argv[1]);
      }
    }
  } else if (!strcmp("LOAD", cmd)) {
    if (argc == 1) {
      puts("Please provide a filename.");
    } else {
      filename = argv[1];
      read_file(machine, filename);
    }
  } else if (!strcmp("SAVE", cmd)) {
    if (argc == 1) {
      puts("Please provide an address range and a filename.");
    } else if (argc == 2) {
      puts("Please provide a filename.");
    } else {
      if (!parse_address_range(argv[1], &ar)) {
        printf("Invalid address range: %s\n", argv[1]);
      } else {
        filename = argv[2];
        write_file(machine, ar, filename);
      }
    }
  } else if (!strcmp("PROTECT", cmd)) {
    if (argc == 1) {
      puts("Please provide an address range.");
    } else {
      if (!parse_address_range(argv[1], &ar)) {
        printf("Invalid address range: %s\n", argv[1]);
      } else {
        printf("Protecting memory range %04X.%04X\n", ar.start, ar.end);
        add_protected_range(machine, ar);
      }
    }
  } else if (!strcmp("UNPROTECT", cmd)) {
    if (argc == 1) {
      puts("Please provide an address range.");
    } else {
      if (!parse_address_range(argv[1], &ar)) {
        printf("Invalid address range: %s\n", argv[1]);
      } else {
        printf("Unprotecting memory range %04X.%04X\n", ar.start, ar.end);
        remove_protected_range(machine, ar);
      }
    }
  } else {
    editing = NOT_EDITING;
    for (i = 0; i < argc; i++) {
      arg = argv[i];
      if (editing == NOT_EDITING) {
        if (arg[0] == ':') {
          /* Write from last address */
          editing = EDITING;
          current = a;
          argv[i]++;
          i--;
        } else if (arg[0] == '.' || parse_address_range(arg, &ar)) {
          if (arg[0] == '.') {
            /* range starting at last address */
            ar.start = a;
            arg++;
            if (!parse_address(arg, &ar.end)) {
              arg--;
              printf("Invalid value: %s\n", arg);
              i = argc;
              break;
            }
          }
          a = ar.start;
          if (arg[strlen(arg)-1] == ':') {
            editing = EDITING_RANGE;
            current = a;
            firstarg = i;
          } else {
            print_memory(c, ar.start, ar.end);
          }
        } else if (parse_address(arg, &a)) {
          if (arg[strlen(arg)-1] == ':') {
            editing = EDITING;
            current = a;
          } else if (i < argc - 1 &&
              (argv[i+1][0] == 'R' ||
               argv[i+1][0] == 'r')) {
            /* wozmon alias for 'go' command,
              supported for backwards compatibility. */
            i = argc;
            current = c->pc;
            c->pc = a;
            print_pc_change(current, c->pc);
            cpu_run(c);
          } else {
            print_memory(c, a, a);
          }
        } else if (i == 0) {
          printf("Invalid command: %s\n", arg);
          i = argc; /* skip the rest */
        } else {
          printf("Invalid value: %s\n", arg);
        }
      } else {
        /* now we should only get bytes */
        if (parse_byte(arg, &b)) {
          c->write(current, b);
          current++;
          if (editing == EDITING_RANGE) {
            if (current > ar.end) {
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
  } /* command list */

  return 0;
}
