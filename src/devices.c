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
 * Emulated peripheral devices for v6502c
 */

#include "devices.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

/*
 * Helper: Check if input is available on a FILE* without blocking
 *
 * We need to check both:
 * 1. stdio's internal buffer (data already read from fd)
 * 2. The file descriptor itself (data waiting in kernel)
 */
static int input_available(FILE *f) {
    int fd;
    struct timeval tv;
    fd_set fds;

    if (f == NULL) return 0;

    /* First check if stdio has buffered data */
    /* Note: This is a workaround for stdio/select interaction issues */
    fd = fileno(f);

    /* Check kernel buffer via select */
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    return select(fd + 1, &fds, NULL, NULL, &tv) > 0;
}

/*
 * 6551 ACIA Implementation
 */

acia_t *acia_create(FILE *in, FILE *out) {
    acia_t *dev = (acia_t *)malloc(sizeof(acia_t));
    if (dev == NULL) return NULL;

    dev->input = in;
    dev->output = out;
    acia_reset(dev);

    return dev;
}

void acia_destroy(acia_t *dev) {
    if (dev != NULL) {
        free(dev);
    }
}

void acia_reset(acia_t *dev) {
    if (dev == NULL) return;

    dev->command = 0x00;
    dev->control = 0x00;
    dev->rx_data = 0x00;
    dev->rx_full = 0;
}

byte acia_read(acia_t *dev, byte reg) {
    byte status;
    int c;

    if (dev == NULL) return 0xFF;

    switch (reg & 0x03) {
    case ACIA_REG_DATA:
        /* Read received data */
        /* If buffer is empty but input available, read it now */
        if (!dev->rx_full && dev->input != NULL && input_available(dev->input)) {
            /* Use read() instead of fgetc() to avoid stdio buffering issues */
            char ch;
            int fd = fileno(dev->input);
            if (read(fd, &ch, 1) == 1) {
                c = (unsigned char)ch;
                /* Convert LF to CR for BASIC compatibility */
                /* Unix terminals send LF ($0A) but BASIC expects CR ($0D) */
                if (c == '\n') {
                    c = '\r';
                }
                dev->rx_data = (byte)c;
                if (V6502C_VERBOSE) {
                    fprintf(stderr, "[RX: %02X '%c']\n", dev->rx_data,
                        (dev->rx_data >= 32 && dev->rx_data < 127) ? dev->rx_data : '.');
                }
            }
        }
        dev->rx_full = 0;
        return dev->rx_data;

    case ACIA_REG_STATUS:
        /* Build status register */
        status = ACIA_STATUS_TDRE; /* Always ready to transmit */

        /* Check if we have buffered data or new input available */
        /* Note: Don't consume input here - just check availability */
        /* Input is only consumed when data register is read */
        if (dev->rx_full || (dev->input != NULL && input_available(dev->input))) {
            status |= ACIA_STATUS_RDRF;
        }

        return status;

    case ACIA_REG_COMMAND:
        return dev->command;

    case ACIA_REG_CONTROL:
        return dev->control;
    }

    return 0xFF;
}

void acia_write(acia_t *dev, byte reg, byte value) {
    if (dev == NULL) return;

    switch (reg & 0x03) {
    case ACIA_REG_DATA:
        /* Transmit data */
        if (dev->output != NULL) {
            if (V6502C_VERBOSE) {
                fprintf(stderr, "[TX: %02X '%c']\n", value,
                    (value >= 32 && value < 127) ? value : '.');
            }
            fputc(value, dev->output);
            fflush(dev->output);
        }
        break;

    case ACIA_REG_STATUS:
        /* Programmed reset */
        acia_reset(dev);
        break;

    case ACIA_REG_COMMAND:
        dev->command = value;
        break;

    case ACIA_REG_CONTROL:
        dev->control = value;
        break;
    }
}

/*
 * 6522 VIA Implementation
 */

via_t *via_create(void) {
    via_t *dev = (via_t *)malloc(sizeof(via_t));
    if (dev == NULL) return NULL;

    via_reset(dev);

    return dev;
}

void via_destroy(via_t *dev) {
    if (dev != NULL) {
        free(dev);
    }
}

void via_reset(via_t *dev) {
    if (dev == NULL) return;

    dev->port_a = 0x00;
    dev->port_b = 0x00;
    dev->ddr_a = 0x00;
    dev->ddr_b = 0x00;
    dev->t1_counter = 0xFFFF;
    dev->t1_latch = 0xFFFF;
    dev->t2_counter = 0xFFFF;
    dev->t2_latch_low = 0xFF;
    dev->shift_reg = 0x00;
    dev->acr = 0x00;
    dev->pcr = 0x00;
    dev->ifr = 0x00;
    dev->ier = 0x00;
    dev->t1_running = 0;
    dev->t2_running = 0;
}

byte via_read(via_t *dev, byte reg) {
    byte value;

    if (dev == NULL) return 0xFF;

    switch (reg & 0x0F) {
    case VIA_REG_PORTB:
        return dev->port_b;

    case VIA_REG_PORTA:
    case VIA_REG_PORTANH:
        return dev->port_a;

    case VIA_REG_DDRB:
        return dev->ddr_b;

    case VIA_REG_DDRA:
        return dev->ddr_a;

    case VIA_REG_T1CL:
        /* Reading T1 low clears T1 interrupt flag */
        dev->ifr &= ~VIA_INT_T1;
        return (byte)(dev->t1_counter & 0xFF);

    case VIA_REG_T1CH:
        return (byte)((dev->t1_counter >> 8) & 0xFF);

    case VIA_REG_T1LL:
        return (byte)(dev->t1_latch & 0xFF);

    case VIA_REG_T1LH:
        return (byte)((dev->t1_latch >> 8) & 0xFF);

    case VIA_REG_T2CL:
        /* Reading T2 low clears T2 interrupt flag */
        dev->ifr &= ~VIA_INT_T2;
        return (byte)(dev->t2_counter & 0xFF);

    case VIA_REG_T2CH:
        return (byte)((dev->t2_counter >> 8) & 0xFF);

    case VIA_REG_SR:
        return dev->shift_reg;

    case VIA_REG_ACR:
        return dev->acr;

    case VIA_REG_PCR:
        return dev->pcr;

    case VIA_REG_IFR:
        /* Set bit 7 if any enabled interrupt is active */
        value = dev->ifr;
        if (value & dev->ier & 0x7F) {
            value |= VIA_INT_IRQ;
        }
        return value;

    case VIA_REG_IER:
        /* Bit 7 always reads as 1 */
        return dev->ier | 0x80;
    }

    return 0xFF;
}

void via_write(via_t *dev, byte reg, byte value) {
    if (dev == NULL) return;

    switch (reg & 0x0F) {
    case VIA_REG_PORTB:
        dev->port_b = value;
        break;

    case VIA_REG_PORTA:
    case VIA_REG_PORTANH:
        dev->port_a = value;
        break;

    case VIA_REG_DDRB:
        dev->ddr_b = value;
        break;

    case VIA_REG_DDRA:
        dev->ddr_a = value;
        break;

    case VIA_REG_T1CL:
    case VIA_REG_T1LL:
        /* Write to latch low */
        dev->t1_latch = (dev->t1_latch & 0xFF00) | value;
        break;

    case VIA_REG_T1CH:
        /* Write to latch high and start timer */
        dev->t1_latch = (dev->t1_latch & 0x00FF) | ((address)value << 8);
        dev->t1_counter = dev->t1_latch;
        dev->t1_running = 1;
        dev->ifr &= ~VIA_INT_T1;
        break;

    case VIA_REG_T1LH:
        /* Write to latch high only */
        dev->t1_latch = (dev->t1_latch & 0x00FF) | ((address)value << 8);
        break;

    case VIA_REG_T2CL:
        /* Write to T2 latch low */
        dev->t2_latch_low = value;
        break;

    case VIA_REG_T2CH:
        /* Write to T2 high and start timer */
        dev->t2_counter = ((address)value << 8) | dev->t2_latch_low;
        dev->t2_running = 1;
        dev->ifr &= ~VIA_INT_T2;
        break;

    case VIA_REG_SR:
        dev->shift_reg = value;
        break;

    case VIA_REG_ACR:
        dev->acr = value;
        break;

    case VIA_REG_PCR:
        dev->pcr = value;
        break;

    case VIA_REG_IFR:
        /* Writing 1 to a bit clears it */
        dev->ifr &= ~(value & 0x7F);
        break;

    case VIA_REG_IER:
        /* Bit 7 controls set (1) or clear (0) */
        if (value & 0x80) {
            dev->ier |= (value & 0x7F);
        } else {
            dev->ier &= ~(value & 0x7F);
        }
        break;
    }
}

void via_tick(via_t *dev) {
    if (dev == NULL) return;

    /* Timer 1 */
    if (dev->t1_running) {
        if (dev->t1_counter == 0) {
            /* Timer expired */
            dev->ifr |= VIA_INT_T1;
            if (dev->acr & VIA_ACR_T1_CONTINUOUS) {
                /* Continuous mode: reload from latch */
                dev->t1_counter = dev->t1_latch;
            } else {
                /* One-shot mode: stop timer */
                dev->t1_running = 0;
            }
        } else {
            dev->t1_counter--;
        }
    }

    /* Timer 2 */
    if (dev->t2_running) {
        if (dev->t2_counter == 0) {
            /* Timer expired */
            dev->ifr |= VIA_INT_T2;
            dev->t2_running = 0;
        } else {
            dev->t2_counter--;
        }
    }
}

int via_irq_pending(via_t *dev) {
    if (dev == NULL) return 0;
    return (dev->ifr & dev->ier & 0x7F) != 0;
}

/*
 * File I/O Device Implementation
 */

fileio_t *fileio_create(void) {
    fileio_t *dev = (fileio_t *)malloc(sizeof(fileio_t));
    if (dev == NULL) return NULL;

    dev->file = NULL;  /* Must initialize before reset checks it */
    fileio_reset(dev);

    return dev;
}

void fileio_destroy(fileio_t *dev) {
    if (dev != NULL) {
        if (dev->file != NULL) {
            fclose(dev->file);
        }
        free(dev);
    }
}

void fileio_reset(fileio_t *dev) {
    if (dev == NULL) return;

    if (dev->file != NULL) {
        fclose(dev->file);
        dev->file = NULL;
    }
    dev->status = FILEIO_STATUS_READY;
    dev->data = 0x00;
    dev->name_index = 0;
    memset(dev->filename, 0, sizeof(dev->filename));
}

byte fileio_read(fileio_t *dev, byte reg) {
    if (dev == NULL) return 0xFF;

    switch (reg) {
    case FILEIO_REG_STATUS:
        return dev->status;
    case FILEIO_REG_DATA:
        return dev->data;
    case FILEIO_REG_NAMEINDEX:
        return dev->name_index;
    case FILEIO_REG_NAMECHAR:
        return (byte)dev->filename[dev->name_index];
    }

    return 0xFF;
}

void fileio_write(fileio_t *dev, byte reg, byte value) {
    int c;

    if (dev == NULL) return;

    switch (reg) {
    case FILEIO_REG_STATUS:
        /* Command register */
        switch (value) {
        case FILEIO_CMD_RESET:
            fileio_reset(dev);
            break;

        case FILEIO_CMD_OPEN_R:
            if (dev->file != NULL) {
                fclose(dev->file);
            }
            dev->filename[dev->name_index] = '\0';
            dev->file = fopen(dev->filename, "rb");
            if (dev->file != NULL) {
                dev->status = FILEIO_STATUS_READY | FILEIO_STATUS_OPEN;
            } else {
                dev->status = FILEIO_STATUS_READY | FILEIO_STATUS_ERR;
            }
            break;

        case FILEIO_CMD_OPEN_W:
            if (dev->file != NULL) {
                fclose(dev->file);
            }
            dev->filename[dev->name_index] = '\0';
            dev->file = fopen(dev->filename, "wb");
            if (dev->file != NULL) {
                dev->status = FILEIO_STATUS_READY | FILEIO_STATUS_OPEN;
            } else {
                dev->status = FILEIO_STATUS_READY | FILEIO_STATUS_ERR;
            }
            break;

        case FILEIO_CMD_READ:
            if (dev->file != NULL) {
                c = fgetc(dev->file);
                if (c == EOF) {
                    dev->status |= FILEIO_STATUS_EOF;
                    dev->data = 0x00;
                } else {
                    dev->data = (byte)c;
                }
            } else {
                dev->status |= FILEIO_STATUS_ERR;
            }
            break;

        case FILEIO_CMD_WRITE:
            if (dev->file != NULL) {
                fputc(dev->data, dev->file);
            } else {
                dev->status |= FILEIO_STATUS_ERR;
            }
            break;

        case FILEIO_CMD_CLOSE:
            if (dev->file != NULL) {
                fclose(dev->file);
                dev->file = NULL;
            }
            dev->status = FILEIO_STATUS_READY;
            break;
        }
        break;

    case FILEIO_REG_DATA:
        dev->data = value;
        break;

    case FILEIO_REG_NAMEINDEX:
        dev->name_index = value;
        break;

    case FILEIO_REG_NAMECHAR:
        /* Write character at current index and auto-increment */
        dev->filename[dev->name_index] = (char)value;
        dev->name_index++;
        break;
    }
}
