#ifndef _CLI_H_
#define _CLI_H_

#define __CREATE_PTYS__

/* Enable POSIX/XSI functions like ptsname, fdopen in strict ANSI mode */
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <unistd.h>

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

#if defined(__CREATE_PTYS__)

/**
 * PTY handle structure returned by pty_alloc.
 * Contains both the master file descriptor and the FILE* for convenience.
 */
typedef struct pty_handle {
  int master_fd;
  FILE *file;
  char slave_name[256];
} pty_handle_t;

/**
 * Allocate a new pseudo-terminal.
 * Returns a pty_handle_t pointer on success, NULL on failure.
 * The caller must call pty_destroy() to free resources.
 */
pty_handle_t *pty_alloc(void);

/**
 * Destroy a previously allocated PTY and free its resources.
 * Closes the file descriptor and FILE*, then frees the handle.
 */
void pty_destroy(pty_handle_t *pty);

#endif /* __unix__ || __APPLE__ || _POSIX_VERSION */

#endif
