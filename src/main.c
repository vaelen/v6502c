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

#include "main.h"
#include "devices.h"

#include <signal.h>
#include <unistd.h>
#include <ctype.h>

static byte mem[0x10000];
static address_range_list protected_ranges;
static cpu c;
static cpu prevc;
static int tick_duration = 50;

/* Emulated devices */
static acia_t *acia1;   /* Primary serial: stdin/stdout */
static acia_t *acia2;   /* Secondary serial: disconnected */
static via_t *via;      /* VIA with timers */
static fileio_t *fio;   /* File I/O device */

void _tick(void) {
  /* Update VIA timers */
  if (via != NULL) {
    via_tick(via);
  }

  /* Check for differences */
  if (V6502C_TRACE) {
    print_pc_change(prevc.pc, c.pc);
    print_register_change(" A", prevc.a, c.a);
    print_register_change(" X", prevc.x, c.x);
    print_register_change(" Y", prevc.y, c.y);
    print_register_change("SR", prevc.sr, c.sr);
    print_register_change("SP", prevc.sp, c.sp);
    prevc = c;
  }
  usleep(tick_duration);
}

void signal_handler(int sig) {
  switch (sig) {
  case SIGINT:
    puts("BREAK");
    cpu_halt(&c);
    signal(SIGINT, signal_handler);
  }
}

byte _read(address a) {
  /* ACIA #1: $C010-$C013 */
  if (a >= 0xC010 && a <= 0xC013) {
    return acia_read(acia1, (byte)(a & 0x03));
  }
  /* ACIA #2: $C020-$C023 */
  if (a >= 0xC020 && a <= 0xC023) {
    return acia_read(acia2, (byte)(a & 0x03));
  }
  /* VIA: $C030-$C03F */
  if (a >= 0xC030 && a <= 0xC03F) {
    return via_read(via, (byte)(a & 0x0F));
  }
  /* File I/O: $C040-$C04F */
  if (a >= 0xC040 && a <= 0xC04F) {
    return fileio_read(fio, (byte)(a & 0x0F));
  }
  return mem[a];
}

void _write(address a, byte b) {
  /* ACIA #1: $C010-$C013 */
  if (a >= 0xC010 && a <= 0xC013) {
    acia_write(acia1, (byte)(a & 0x03), b);
    return;
  }
  /* ACIA #2: $C020-$C023 */
  if (a >= 0xC020 && a <= 0xC023) {
    acia_write(acia2, (byte)(a & 0x03), b);
    return;
  }
  /* VIA: $C030-$C03F */
  if (a >= 0xC030 && a <= 0xC03F) {
    via_write(via, (byte)(a & 0x0F), b);
    return;
  }
  /* File I/O: $C040-$C04F */
  if (a >= 0xC040 && a <= 0xC04F) {
    fileio_write(fio, (byte)(a & 0x0F), b);
    return;
  }
  if (is_address_protected(&protected_ranges, a)) {
    /* Address is write-protected */
    if (V6502C_VERBOSE) {
      fprintf(stderr, "Write to protected address %04X ignored\n", a);
    }
    return;
  }
  mem[a] = b;
}

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
  char *current = NULL, *end = NULL;

  if (argv == NULL || argc == NULL || cmdbuf == NULL) return;

  current = cmdbuf;
  end = cmdbuf + strlen(cmdbuf);
  
  /** tokenize by whitespace */

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


/** Add a protected memory range where writes are ignored. */
void add_protected_range(address_range ar) {
  printf("Protecting memory range %04X.%04X\n", ar.start, ar.end);
  add_address_range(&protected_ranges, ar);
}

/** Remove a protected memory range, allowing writes. */
void remove_protected_range(address_range ar) {
  printf("Unprotecting memory range %04X.%04X\n", ar.start, ar.end);
  remove_address_range(&protected_ranges, ar);
}

/** Check if an address is within any protected memory range. */
bool is_address_protected(address_range_list *list, address a) {
  return is_address_in_range_list(list, a);
}

void read_lines(cpu *c, FILE *in) {
  int l = 0, i = 0;
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
      /** Is this really an EOF, or just an error? */
      if (feof(in)) {
        done = TRUE;
      }
    } else if (parse_command(c, cmdbuf)) {
        done = TRUE;
    }
  }
  
}

int write_file(cpu *c, address_range ar, char *filename) {
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

int read_file(cpu *c, char *filename) {
  FILE *file = NULL;

  printf("Loading %s\n", filename);

  file = fopen(filename, "r");
  if (file == NULL) {
    printf("Could not open file: %s\n", filename);
    return 0;
  }

  read_lines(c, file);

  fclose(file);

  return 0;
}

int parse_command(cpu *c, char *cmdbuf) {
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
    /** Empty command, do nothing. */
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
    prevc = *c;
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
      read_file(c, filename);
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
        write_file(c, ar, filename);
      }
    }
  } else if (!strcmp("PROTECT", cmd)) {
    if (argc == 1) {
      puts("Please provide an address range.");
    } else {
      if (!parse_address_range(argv[1], &ar)) {
        printf("Invalid address range: %s\n", argv[1]);
      } else {
        add_protected_range(ar);
      }
    }
  } else if (!strcmp("UNPROTECT", cmd)) {
    if (argc == 1) {
      puts("Please provide an address range.");
    } else {
      if (!parse_address_range(argv[1], &ar)) {
        printf("Invalid address range: %s\n", argv[1]);
      } else {
        remove_protected_range(ar);
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
          } else if (i < argc &&
              argv[i+1][0] == 'R' ||
              argv[i+1][0] == 'r') {
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

int main(int argc, char** argv) {
  int i;

  /* Initialize protected address ranges */
  init_address_range_list(&protected_ranges);

  /* Create device instances */
  acia1 = acia_create(stdin, stdout);
  acia2 = acia_create(NULL, NULL);  /* Disconnected for now */
  via = via_create();
  fio = fileio_create();

  cpu_init(&c);

  c.read = _read;
  c.write = _write;
  c.tick = _tick;

  signal(SIGINT, signal_handler);

  cpu_reset(&c);
  cpu_step(&c);

  puts(V6502C_VERSION);
  puts(V6502C_COPYRIGHT);
  puts("");

  /* Process command-line files as scripts */
  for (i = 1; i < argc; i++) {
    read_file(&c, argv[i]);
  }

  puts("Type 'help' for help.");
  puts("");

  read_lines(&c, stdin);

  /* Clean up devices */
  acia_destroy(acia1);
  acia_destroy(acia2);
  via_destroy(via);
  fileio_destroy(fio);

  clear_address_range_list(&protected_ranges);

  return 0;
}
