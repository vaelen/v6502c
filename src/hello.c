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


/**
   This is an example of embedding the emulator within another program.
*/

#include <stdio.h>
#include <string.h>

#include <v6502.h>

#include "hello.h"

byte mem[0xFFFF];

byte read(address a) {
  if (a == 0xFF00) {
    /** Character device */
    return getchar();
  }
  return mem[a];
}

void write(address a, byte b) {
  if (a == 0xFF00) {
    /** Character device */
    putchar(b);
    return;
  }
  mem[a] = b;
}

int main(int argc, char** argv) {
  cpu c;

  /** Initialize CPU data structure */
  cpu_init(&c);

  /** Set read and write function pointers */
  c.read = read;
  c.write = write;

  /** Load the program */
  memcpy(&mem[0x1000], src_hello_bin, src_hello_bin_len);

  /* Set reset vector to start of program */
  mem[0xFFFC] = 0x00;
  mem[0xFFFD] = 0x10;
  
  /** Reset the CPU */
  cpu_reset(&c);
  
  /** Run the program */
  cpu_run(&c);

  return 0;
}
