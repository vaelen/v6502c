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

#ifndef DEVICES_H
#define DEVICES_H

#include <stdio.h>
#include "v6502.h"

/*
 * MOS 6551 ACIA (Asynchronous Communications Interface Adapter)
 *
 * Register offsets:
 *   +0: Data Register (read: RX data, write: TX data)
 *   +1: Status Register (read) / Programmed Reset (write)
 *   +2: Command Register
 *   +3: Control Register
 *
 * Status Register bits:
 *   Bit 0: Parity Error
 *   Bit 1: Framing Error
 *   Bit 2: Overrun
 *   Bit 3: Receive Data Register Full (RDRF)
 *   Bit 4: Transmit Data Register Empty (TDRE)
 *   Bit 5: DCD
 *   Bit 6: DSR
 *   Bit 7: IRQ
 */

#define ACIA_REG_DATA    0
#define ACIA_REG_STATUS  1
#define ACIA_REG_COMMAND 2
#define ACIA_REG_CONTROL 3

#define ACIA_STATUS_PE   0x01
#define ACIA_STATUS_FE   0x02
#define ACIA_STATUS_OVR  0x04
#define ACIA_STATUS_RDRF 0x08
#define ACIA_STATUS_TDRE 0x10
#define ACIA_STATUS_DCD  0x20
#define ACIA_STATUS_DSR  0x40
#define ACIA_STATUS_IRQ  0x80

typedef struct {
    FILE *input;
    FILE *output;
    byte command;
    byte control;
    byte rx_data;
    int rx_full;
} acia_t;

acia_t *acia_create(FILE *in, FILE *out);
void acia_destroy(acia_t *dev);
void acia_reset(acia_t *dev);
byte acia_read(acia_t *dev, byte reg);
void acia_write(acia_t *dev, byte reg, byte value);

/*
 * MOS 6522 VIA (Versatile Interface Adapter)
 *
 * Register offsets:
 *   +0: Port B I/O
 *   +1: Port A I/O
 *   +2: Data Direction B
 *   +3: Data Direction A
 *   +4: Timer 1 Counter Low
 *   +5: Timer 1 Counter High
 *   +6: Timer 1 Latch Low
 *   +7: Timer 1 Latch High
 *   +8: Timer 2 Counter Low
 *   +9: Timer 2 Counter High
 *   +A: Shift Register
 *   +B: Auxiliary Control Register
 *   +C: Peripheral Control Register
 *   +D: Interrupt Flag Register
 *   +E: Interrupt Enable Register
 *   +F: Port A (no handshake)
 */

#define VIA_REG_PORTB   0x00
#define VIA_REG_PORTA   0x01
#define VIA_REG_DDRB    0x02
#define VIA_REG_DDRA    0x03
#define VIA_REG_T1CL    0x04
#define VIA_REG_T1CH    0x05
#define VIA_REG_T1LL    0x06
#define VIA_REG_T1LH    0x07
#define VIA_REG_T2CL    0x08
#define VIA_REG_T2CH    0x09
#define VIA_REG_SR      0x0A
#define VIA_REG_ACR     0x0B
#define VIA_REG_PCR     0x0C
#define VIA_REG_IFR     0x0D
#define VIA_REG_IER     0x0E
#define VIA_REG_PORTANH 0x0F

/* Interrupt Flag/Enable Register bits */
#define VIA_INT_CA2  0x01
#define VIA_INT_CA1  0x02
#define VIA_INT_SR   0x04
#define VIA_INT_CB2  0x08
#define VIA_INT_CB1  0x10
#define VIA_INT_T2   0x20
#define VIA_INT_T1   0x40
#define VIA_INT_IRQ  0x80

/* Auxiliary Control Register bits */
#define VIA_ACR_T1_CONTINUOUS 0x40
#define VIA_ACR_T1_PB7_OUT    0x80

typedef struct {
    byte port_a;
    byte port_b;
    byte ddr_a;
    byte ddr_b;
    address t1_counter;
    address t1_latch;
    address t2_counter;
    byte t2_latch_low;
    byte shift_reg;
    byte acr;
    byte pcr;
    byte ifr;
    byte ier;
    int t1_running;
    int t2_running;
} via_t;

via_t *via_create(void);
void via_destroy(via_t *dev);
void via_reset(via_t *dev);
byte via_read(via_t *dev, byte reg);
void via_write(via_t *dev, byte reg, byte value);
void via_tick(via_t *dev);
int via_irq_pending(via_t *dev);

/*
 * File I/O Device
 *
 * Custom device for BASIC LOAD/SAVE operations.
 *
 * Register offsets:
 *   +0: Status (read) / Command (write)
 *   +1: Data In (read) / Data Out (write)
 *   +2: Filename index (auto-increments on write to +3)
 *   +3: Filename character at current index
 *
 * Commands:
 *   $00: Reset/Close
 *   $01: Open for Read
 *   $02: Open for Write
 *   $03: Read byte
 *   $04: Write byte
 *   $05: Close file
 *
 * Status bits:
 *   Bit 0: File open
 *   Bit 1: EOF
 *   Bit 2: Error
 *   Bit 7: Ready (1) / Busy (0)
 */

#define FILEIO_REG_STATUS   0x00
#define FILEIO_REG_DATA     0x01
#define FILEIO_REG_NAMEINDEX 0x02
#define FILEIO_REG_NAMECHAR 0x03

#define FILEIO_CMD_RESET    0x00
#define FILEIO_CMD_OPEN_R   0x01
#define FILEIO_CMD_OPEN_W   0x02
#define FILEIO_CMD_READ     0x03
#define FILEIO_CMD_WRITE    0x04
#define FILEIO_CMD_CLOSE    0x05

#define FILEIO_STATUS_OPEN  0x01
#define FILEIO_STATUS_EOF   0x02
#define FILEIO_STATUS_ERR   0x04
#define FILEIO_STATUS_READY 0x80

#define FILEIO_NAME_MAXLEN  256

typedef struct {
    FILE *file;
    byte status;
    byte data;
    byte name_index;
    char filename[FILEIO_NAME_MAXLEN];
} fileio_t;

fileio_t *fileio_create(void);
void fileio_destroy(fileio_t *dev);
void fileio_reset(fileio_t *dev);
byte fileio_read(fileio_t *dev, byte reg);
void fileio_write(fileio_t *dev, byte reg, byte value);

#endif /* DEVICES_H */
