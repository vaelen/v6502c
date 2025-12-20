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

#include "cli.h"
#include "vmachine.h"
#include "monitor.h"

#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#if defined(__CREATE_PTYS__)
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
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

int load_binary_rom(const char *filename, byte *buffer, size_t max_size) {
  FILE *f = fopen(filename, "rb");
  size_t bytes_read = 0;
  if (f == NULL) {
    fprintf(stderr, "Error: Unable to open ROM file '%s'\n", filename);
    return -1;
  }
  bytes_read = fread(buffer, 1, max_size, f);
  fclose(f);
  return (int)bytes_read;
}

int load_woz_rom(const char *filename, byte *buffer, size_t max_size, size_t offset) {
  /* Woz Format: */
  /* D000: F5 D6 FA D5 09 DC 8B D8 */
  FILE *f = fopen(filename, "r");
  size_t max_offset = 0;
  if (f == NULL) {
    fprintf(stderr, "Error: Unable to open ROM file '%s'\n", filename);
    return -1;
  }

  /* Initialize buffer to zeros */
  memset(buffer, 0, max_size);

  {
    char line[256];
    while (fgets(line, sizeof(line), f) != NULL) {
      char *p = line;
      unsigned int addr;
      size_t buf_offset;

      /* Parse the address before the colon */
      addr = 0;
      while (*p && *p != ':') {
        if (*p >= '0' && *p <= '9') {
          addr = (addr << 4) | (*p - '0');
        } else if (*p >= 'A' && *p <= 'F') {
          addr = (addr << 4) | (*p - 'A' + 10);
        } else if (*p >= 'a' && *p <= 'f') {
          addr = (addr << 4) | (*p - 'a' + 10);
        }
        p++;
      }

      if (*p != ':') {
        continue; /* Skip lines without a colon */
      }
      p++; /* Skip the colon */

      /* Calculate buffer offset */
      if (addr < offset) {
        continue; /* Address is below our offset, skip */
      }
      buf_offset = addr - offset;
      if (buf_offset >= max_size) {
        continue; /* Address is outside our buffer range, skip */
      }

      /* Parse hex bytes */
      while (*p) {
        unsigned int byte_val;
        int nibbles = 0;

        /* Skip whitespace */
        while (*p == ' ' || *p == '\t') {
          p++;
        }

        if (*p == '\0' || *p == '\n' || *p == '\r') {
          break;
        }

        /* Parse a hex byte (1 or 2 nibbles) */
        byte_val = 0;
        while (nibbles < 2) {
          if (*p >= '0' && *p <= '9') {
            byte_val = (byte_val << 4) | (*p - '0');
            nibbles++;
            p++;
          } else if (*p >= 'A' && *p <= 'F') {
            byte_val = (byte_val << 4) | (*p - 'A' + 10);
            nibbles++;
            p++;
          } else if (*p >= 'a' && *p <= 'f') {
            byte_val = (byte_val << 4) | (*p - 'a' + 10);
            nibbles++;
            p++;
          } else {
            break;
          }
        }

        if (nibbles > 0 && buf_offset < max_size) {
          buffer[buf_offset] = (byte)byte_val;
          buf_offset++;
          if (buf_offset > max_offset) {
            max_offset = buf_offset;
          }
        }
      }
    }
  }
  fclose(f);
  return (int)max_offset;
}

int load_rom(const char *filename, byte *buffer, size_t max_size, size_t offset) {
  /* Determine file type by extension */
  const char *ext = strrchr(filename, '.');
  if (ext != NULL) {
    if (strcmp(ext, ".woz") == 0) {
      /* Load WOZ format ROM */
      return load_woz_rom(filename, buffer, max_size, offset);
    }
  }
  /* Default to binary ROM */
  return load_binary_rom(filename, buffer, max_size);
}

#if defined(__CREATE_PTYS__)

pty_handle_t *pty_alloc(void) {
  pty_handle_t *pty;
  int master_fd;
  char *slave_name;

  master_fd = posix_openpt(O_RDWR | O_NOCTTY);
  if (master_fd < 0) {
    return NULL;
  }

  if (grantpt(master_fd) < 0) {
    close(master_fd);
    return NULL;
  }

  if (unlockpt(master_fd) < 0) {
    close(master_fd);
    return NULL;
  }

  slave_name = ptsname(master_fd);
  if (slave_name == NULL) {
    close(master_fd);
    return NULL;
  }

  pty = (pty_handle_t *)malloc(sizeof(pty_handle_t));
  if (pty == NULL) {
    close(master_fd);
    return NULL;
  }

  pty->master_fd = master_fd;
  pty->file = fdopen(master_fd, "r+");
  if (pty->file == NULL) {
    close(master_fd);
    free(pty);
    return NULL;
  }

  /* Configure terminal: raw mode, 9600 baud, 8N1 */
  {
    struct termios tio;
    if (tcgetattr(master_fd, &tio) == 0) {
      /* Set raw mode - disable canonical processing and echo */
      tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
      tio.c_iflag &= ~(IXON | IXOFF | ICRNL | IGNCR);
      tio.c_iflag |= INLCR; /* Translate NL to CR for BASIC compatibility */
      tio.c_oflag &= ~(OPOST | ONLCR | OCRNL);
      /* 8 data bits, no parity */
      tio.c_cflag &= ~(PARENB | CSIZE);
      tio.c_cflag |= CS8;
      /* Set baud rate to 115200 */
      cfsetispeed(&tio, B115200);
      cfsetospeed(&tio, B115200);
      /* Minimum 1 byte, no timeout */
      tio.c_cc[VMIN] = 1;
      tio.c_cc[VTIME] = 0;
      tcsetattr(master_fd, TCSANOW, &tio);
    }
  }

  strncpy(pty->slave_name, slave_name, sizeof(pty->slave_name) - 1);
  pty->slave_name[sizeof(pty->slave_name) - 1] = '\0';

  return pty;
}

void pty_destroy(pty_handle_t *pty) {
  if (pty == NULL) {
    return;
  }
  if (pty->file != NULL) {
    fclose(pty->file);
  }
  free(pty);
}

#endif /* __CREATE_PTYS__ */

int main(int argc, char** argv) {
  int i;
  vmachine_t machine;
  vmachine_config_t config;

  byte rom_data[VMACHINE_ROM_SIZE];
  size_t rom_size = 0;
  char *rom_filename = argv[1];

#if defined(__CREATE_PTYS__)
  pty_handle_t *pty1 = NULL;
  pty_handle_t *pty2 = NULL;
#endif

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <romfile> [scriptfile...]\n", argv[0]);
    return 1;
  }

  /* Load ROM */
  rom_size = load_rom(rom_filename, rom_data, sizeof(rom_data), VMACHINE_ROM_START);
  if (rom_size < 0) {
    return 1;
  }
  printf("Loaded ROM: %s, Size: %d bytes\n", rom_filename, (int)rom_size);

#if defined(__CREATE_PTYS__)
  /* Allocate PTYs for ACIA devices */
  pty1 = pty_alloc();
  if (pty1 == NULL) {
    fprintf(stderr, "Warning: Failed to allocate PTY for ACIA1\n");
  } else {
    printf("ACIA1 PTY: %s\n", pty1->slave_name);
  }

  pty2 = pty_alloc();
  if (pty2 == NULL) {
    fprintf(stderr, "Warning: Failed to allocate PTY for ACIA2\n");
  } else {
    printf("ACIA2 PTY: %s\n", pty2->slave_name);
  }
#endif

  /* Initialize VM configuration */
  memset(&config, 0, sizeof(config));
  config.rom_data = rom_data;
  config.rom_size = rom_size;
  config.tick_duration = 50; /* 50ms per tick */

#if defined(__CREATE_PTYS__)
  config.acia1_input = pty1 ? pty1->file : NULL;
  config.acia1_output = pty1 ? pty1->file : NULL;
  config.acia2_input = pty2 ? pty2->file : NULL;
  config.acia2_output = pty2 ? pty2->file : NULL;
#else
  config.acia1_input = stdin;
  config.acia1_output = stdout;
  config.acia2_input = NULL;
  config.acia2_output = NULL;
#endif

  init_vmachine(&machine, &config);
  g_machine = &machine;
  machine.c.read = _read;
  machine.c.write = _write;
  machine.c.tick = _tick;
  machine.trace_fn = monitor_trace_fn;

  signal(SIGINT, signal_handler);

  puts(V6502C_VERSION);
  puts(V6502C_COPYRIGHT);
  puts("");

  if (argc > 2) {
    puts("Processing command-line script files...");
    for (i = 2; i < argc; i++) {
      read_file(&machine, argv[i]);
    }
  } else {
    /* No script files provided, start with default settings. */
    puts("No script files provided, starting with default settings...");
    sleep(2); /* Give the user time to connect a terminal */
    V6502C_TRACE = FALSE;
    V6502C_VERBOSE = TRUE;
    cpu_reset(&machine.c);
    cpu_step(&machine.c);
    cpu_run(&machine.c);
  }

  puts("Type 'help' for help.");
  puts("");

  monitor_repl(&machine, stdin);

  cleanup_vmachine(&machine);

#if defined(__CREATE_PTYS__)
  if (pty1 != NULL) {
    pty_destroy(pty1);
  }
  if (pty2 != NULL) {
    pty_destroy(pty2);
  }
#endif

  return 0;
}
