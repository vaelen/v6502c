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
 * Comprehensive tests for emulated peripheral devices
 */

#include <stdio.h>
#include <string.h>
#include "devices.h"

/* ANSI color codes for terminal output */
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_RESET "\033[0m"

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

/* Test framework functions */
static void pass(const char *test_name) {
    printf("Testing %s... " COLOR_GREEN "passed" COLOR_RESET "\n", test_name);
    tests_passed++;
}

static void fail(const char *test_name, const char *reason) {
    printf("Testing %s... " COLOR_RED "failed" COLOR_RESET ": %s\n", test_name, reason);
    tests_failed++;
}

/*
 * ============================================================================
 * ACIA (6551) Tests
 * ============================================================================
 */

/* Test ACIA creation and destruction */
static void test_acia_create_destroy(void) {
    acia_t *dev;

    /* Test creation with NULL streams */
    dev = acia_create(NULL, NULL);
    if (dev == NULL) {
        fail("ACIA create/destroy", "Failed to create ACIA with NULL streams");
        return;
    }
    acia_destroy(dev);

    /* Test creation with real streams */
    dev = acia_create(stdin, stdout);
    if (dev == NULL) {
        fail("ACIA create/destroy", "Failed to create ACIA with real streams");
        return;
    }
    acia_destroy(dev);

    /* Test destroy with NULL (should not crash) */
    acia_destroy(NULL);

    pass("ACIA create/destroy");
}

/* Test ACIA reset */
static void test_acia_reset(void) {
    acia_t *dev;

    dev = acia_create(NULL, NULL);
    if (dev == NULL) {
        fail("ACIA reset", "Failed to create ACIA");
        return;
    }

    /* Set some values */
    dev->command = 0xFF;
    dev->control = 0xFF;
    dev->rx_data = 0xAA;
    dev->rx_full = 1;

    /* Reset and verify */
    acia_reset(dev);

    if (dev->command != 0x00) {
        fail("ACIA reset", "Command register not cleared");
        acia_destroy(dev);
        return;
    }
    if (dev->control != 0x00) {
        fail("ACIA reset", "Control register not cleared");
        acia_destroy(dev);
        return;
    }
    if (dev->rx_full != 0) {
        fail("ACIA reset", "RX full flag not cleared");
        acia_destroy(dev);
        return;
    }

    /* Test reset with NULL (should not crash) */
    acia_reset(NULL);

    acia_destroy(dev);
    pass("ACIA reset");
}

/* Test ACIA status register */
static void test_acia_status(void) {
    acia_t *dev;
    byte status;

    dev = acia_create(NULL, NULL);
    if (dev == NULL) {
        fail("ACIA status register", "Failed to create ACIA");
        return;
    }

    /* With no input, TDRE should be set, RDRF should be clear */
    status = acia_read(dev, ACIA_REG_STATUS);
    if (!(status & ACIA_STATUS_TDRE)) {
        fail("ACIA status register", "TDRE should be set (always ready to transmit)");
        acia_destroy(dev);
        return;
    }
    if (status & ACIA_STATUS_RDRF) {
        fail("ACIA status register", "RDRF should be clear (no data available)");
        acia_destroy(dev);
        return;
    }

    /* Manually set rx_full and verify RDRF is set */
    dev->rx_data = 0x42;
    dev->rx_full = 1;
    status = acia_read(dev, ACIA_REG_STATUS);
    if (!(status & ACIA_STATUS_RDRF)) {
        fail("ACIA status register", "RDRF should be set when data is buffered");
        acia_destroy(dev);
        return;
    }

    acia_destroy(dev);
    pass("ACIA status register");
}

/* Test ACIA data register read */
static void test_acia_data_read(void) {
    acia_t *dev;
    byte data;

    dev = acia_create(NULL, NULL);
    if (dev == NULL) {
        fail("ACIA data read", "Failed to create ACIA");
        return;
    }

    /* Buffer some data */
    dev->rx_data = 0x42;
    dev->rx_full = 1;

    /* Read data */
    data = acia_read(dev, ACIA_REG_DATA);
    if (data != 0x42) {
        fail("ACIA data read", "Did not read expected data");
        acia_destroy(dev);
        return;
    }

    /* rx_full should be cleared after read */
    if (dev->rx_full != 0) {
        fail("ACIA data read", "rx_full not cleared after read");
        acia_destroy(dev);
        return;
    }

    acia_destroy(dev);
    pass("ACIA data read");
}

/* Test ACIA data register write (transmit) */
static void test_acia_data_write(void) {
    acia_t *dev;
    FILE *out;
    const char *tmpfile = "/tmp/v6502c_acia_test.txt";
    int c;

    /* Create a temp file to capture output */
    out = fopen(tmpfile, "w");
    if (out == NULL) {
        fail("ACIA data write", "Failed to create temp file");
        return;
    }

    dev = acia_create(NULL, out);
    if (dev == NULL) {
        fail("ACIA data write", "Failed to create ACIA");
        fclose(out);
        remove(tmpfile);
        return;
    }

    /* Write a character */
    acia_write(dev, ACIA_REG_DATA, 'X');

    /* Close the file and reopen to read */
    fclose(out);
    dev->output = NULL;

    out = fopen(tmpfile, "r");
    if (out == NULL) {
        fail("ACIA data write", "Failed to reopen temp file");
        acia_destroy(dev);
        remove(tmpfile);
        return;
    }

    c = fgetc(out);
    fclose(out);
    remove(tmpfile);

    if (c != 'X') {
        fail("ACIA data write", "Character not transmitted correctly");
        acia_destroy(dev);
        return;
    }

    acia_destroy(dev);
    pass("ACIA data write");
}

/* Test ACIA command and control registers */
static void test_acia_command_control(void) {
    acia_t *dev;

    dev = acia_create(NULL, NULL);
    if (dev == NULL) {
        fail("ACIA command/control", "Failed to create ACIA");
        return;
    }

    /* Write to command register */
    acia_write(dev, ACIA_REG_COMMAND, 0xAB);
    if (acia_read(dev, ACIA_REG_COMMAND) != 0xAB) {
        fail("ACIA command/control", "Command register read/write failed");
        acia_destroy(dev);
        return;
    }

    /* Write to control register */
    acia_write(dev, ACIA_REG_CONTROL, 0xCD);
    if (acia_read(dev, ACIA_REG_CONTROL) != 0xCD) {
        fail("ACIA command/control", "Control register read/write failed");
        acia_destroy(dev);
        return;
    }

    /* Write to status register triggers reset */
    dev->command = 0xFF;
    dev->control = 0xFF;
    acia_write(dev, ACIA_REG_STATUS, 0x00);
    if (dev->command != 0x00 || dev->control != 0x00) {
        fail("ACIA command/control", "Writing to status should trigger reset");
        acia_destroy(dev);
        return;
    }

    acia_destroy(dev);
    pass("ACIA command/control");
}

/* Test ACIA with NULL device pointer */
static void test_acia_null_device(void) {
    byte result;

    /* These should not crash and should return sensible values */
    result = acia_read(NULL, ACIA_REG_DATA);
    if (result != 0xFF) {
        fail("ACIA NULL device", "Read from NULL should return 0xFF");
        return;
    }

    acia_write(NULL, ACIA_REG_DATA, 0x42);
    /* Just verify it doesn't crash */

    pass("ACIA NULL device");
}

/*
 * ============================================================================
 * VIA (6522) Tests
 * ============================================================================
 */

/* Test VIA creation and destruction */
static void test_via_create_destroy(void) {
    via_t *dev;

    dev = via_create();
    if (dev == NULL) {
        fail("VIA create/destroy", "Failed to create VIA");
        return;
    }

    via_destroy(dev);

    /* Test destroy with NULL (should not crash) */
    via_destroy(NULL);

    pass("VIA create/destroy");
}

/* Test VIA reset */
static void test_via_reset(void) {
    via_t *dev;

    dev = via_create();
    if (dev == NULL) {
        fail("VIA reset", "Failed to create VIA");
        return;
    }

    /* Set some values */
    dev->port_a = 0xFF;
    dev->port_b = 0xFF;
    dev->ddr_a = 0xFF;
    dev->ddr_b = 0xFF;
    dev->t1_counter = 0x1234;
    dev->t1_latch = 0x5678;
    dev->t2_counter = 0x9ABC;
    dev->ifr = 0x7F;
    dev->ier = 0x7F;
    dev->t1_running = 1;
    dev->t2_running = 1;

    /* Reset and verify */
    via_reset(dev);

    if (dev->port_a != 0x00 || dev->port_b != 0x00) {
        fail("VIA reset", "Ports not cleared");
        via_destroy(dev);
        return;
    }
    if (dev->ddr_a != 0x00 || dev->ddr_b != 0x00) {
        fail("VIA reset", "DDR not cleared");
        via_destroy(dev);
        return;
    }
    if (dev->t1_counter != 0xFFFF || dev->t1_latch != 0xFFFF) {
        fail("VIA reset", "Timer 1 not reset to 0xFFFF");
        via_destroy(dev);
        return;
    }
    if (dev->t2_counter != 0xFFFF) {
        fail("VIA reset", "Timer 2 not reset to 0xFFFF");
        via_destroy(dev);
        return;
    }
    if (dev->ifr != 0x00 || dev->ier != 0x00) {
        fail("VIA reset", "Interrupt registers not cleared");
        via_destroy(dev);
        return;
    }
    if (dev->t1_running != 0 || dev->t2_running != 0) {
        fail("VIA reset", "Timers should be stopped");
        via_destroy(dev);
        return;
    }

    /* Test reset with NULL (should not crash) */
    via_reset(NULL);

    via_destroy(dev);
    pass("VIA reset");
}

/* Test VIA port I/O */
static void test_via_ports(void) {
    via_t *dev;

    dev = via_create();
    if (dev == NULL) {
        fail("VIA ports", "Failed to create VIA");
        return;
    }

    /* Write and read Port A */
    via_write(dev, VIA_REG_PORTA, 0xAA);
    if (via_read(dev, VIA_REG_PORTA) != 0xAA) {
        fail("VIA ports", "Port A read/write failed");
        via_destroy(dev);
        return;
    }

    /* Write and read Port B */
    via_write(dev, VIA_REG_PORTB, 0x55);
    if (via_read(dev, VIA_REG_PORTB) != 0x55) {
        fail("VIA ports", "Port B read/write failed");
        via_destroy(dev);
        return;
    }

    /* Write and read DDR A */
    via_write(dev, VIA_REG_DDRA, 0xF0);
    if (via_read(dev, VIA_REG_DDRA) != 0xF0) {
        fail("VIA ports", "DDR A read/write failed");
        via_destroy(dev);
        return;
    }

    /* Write and read DDR B */
    via_write(dev, VIA_REG_DDRB, 0x0F);
    if (via_read(dev, VIA_REG_DDRB) != 0x0F) {
        fail("VIA ports", "DDR B read/write failed");
        via_destroy(dev);
        return;
    }

    /* Test Port A no-handshake register */
    via_write(dev, VIA_REG_PORTANH, 0xBB);
    if (via_read(dev, VIA_REG_PORTANH) != 0xBB) {
        fail("VIA ports", "Port A no-handshake read/write failed");
        via_destroy(dev);
        return;
    }

    via_destroy(dev);
    pass("VIA ports");
}

/* Test VIA Timer 1 */
static void test_via_timer1(void) {
    via_t *dev;
    int i;

    dev = via_create();
    if (dev == NULL) {
        fail("VIA Timer 1", "Failed to create VIA");
        return;
    }

    /* Set up Timer 1 latch */
    via_write(dev, VIA_REG_T1LL, 0x10);
    via_write(dev, VIA_REG_T1LH, 0x00);

    /* Verify latch was set */
    if (via_read(dev, VIA_REG_T1LL) != 0x10) {
        fail("VIA Timer 1", "Latch low byte not set");
        via_destroy(dev);
        return;
    }
    if (via_read(dev, VIA_REG_T1LH) != 0x00) {
        fail("VIA Timer 1", "Latch high byte not set");
        via_destroy(dev);
        return;
    }

    /* Start Timer 1 by writing to T1CH */
    via_write(dev, VIA_REG_T1CL, 0x10);
    via_write(dev, VIA_REG_T1CH, 0x00);

    if (!dev->t1_running) {
        fail("VIA Timer 1", "Timer 1 should be running after write to T1CH");
        via_destroy(dev);
        return;
    }

    /* Tick the timer and check it counts down */
    for (i = 0; i < 5; i++) {
        via_tick(dev);
    }

    if (dev->t1_counter >= 0x0010) {
        fail("VIA Timer 1", "Timer 1 should have counted down");
        via_destroy(dev);
        return;
    }

    /* Tick until timer expires */
    while (dev->t1_counter > 0) {
        via_tick(dev);
    }
    via_tick(dev);

    /* Check T1 interrupt flag is set */
    if (!(dev->ifr & VIA_INT_T1)) {
        fail("VIA Timer 1", "T1 interrupt flag should be set on expiry");
        via_destroy(dev);
        return;
    }

    /* In one-shot mode, timer should stop */
    if (dev->t1_running) {
        fail("VIA Timer 1", "Timer 1 should stop in one-shot mode");
        via_destroy(dev);
        return;
    }

    /* Reading T1CL should clear interrupt flag */
    via_read(dev, VIA_REG_T1CL);
    if (dev->ifr & VIA_INT_T1) {
        fail("VIA Timer 1", "Reading T1CL should clear T1 interrupt flag");
        via_destroy(dev);
        return;
    }

    via_destroy(dev);
    pass("VIA Timer 1");
}

/* Test VIA Timer 1 continuous mode */
static void test_via_timer1_continuous(void) {
    via_t *dev;

    dev = via_create();
    if (dev == NULL) {
        fail("VIA Timer 1 continuous", "Failed to create VIA");
        return;
    }

    /* Set continuous mode */
    via_write(dev, VIA_REG_ACR, VIA_ACR_T1_CONTINUOUS);

    /* Set up and start Timer 1 */
    via_write(dev, VIA_REG_T1LL, 0x05);
    via_write(dev, VIA_REG_T1LH, 0x00);
    via_write(dev, VIA_REG_T1CL, 0x05);
    via_write(dev, VIA_REG_T1CH, 0x00);

    /* Tick until timer expires */
    while (dev->t1_counter > 0) {
        via_tick(dev);
    }
    via_tick(dev);

    /* In continuous mode, timer should reload and keep running */
    if (!dev->t1_running) {
        fail("VIA Timer 1 continuous", "Timer 1 should keep running in continuous mode");
        via_destroy(dev);
        return;
    }

    /* Counter should have reloaded from latch */
    if (dev->t1_counter != 0x0005) {
        fail("VIA Timer 1 continuous", "Timer 1 should reload from latch");
        via_destroy(dev);
        return;
    }

    via_destroy(dev);
    pass("VIA Timer 1 continuous");
}

/* Test VIA Timer 2 */
static void test_via_timer2(void) {
    via_t *dev;

    dev = via_create();
    if (dev == NULL) {
        fail("VIA Timer 2", "Failed to create VIA");
        return;
    }

    /* Set up Timer 2 */
    via_write(dev, VIA_REG_T2CL, 0x08);
    via_write(dev, VIA_REG_T2CH, 0x00);

    if (!dev->t2_running) {
        fail("VIA Timer 2", "Timer 2 should be running after write to T2CH");
        via_destroy(dev);
        return;
    }

    /* Tick until timer expires */
    while (dev->t2_counter > 0) {
        via_tick(dev);
    }
    via_tick(dev);

    /* Check T2 interrupt flag is set */
    if (!(dev->ifr & VIA_INT_T2)) {
        fail("VIA Timer 2", "T2 interrupt flag should be set on expiry");
        via_destroy(dev);
        return;
    }

    /* Timer 2 always stops after expiry (no continuous mode) */
    if (dev->t2_running) {
        fail("VIA Timer 2", "Timer 2 should stop after expiry");
        via_destroy(dev);
        return;
    }

    /* Reading T2CL should clear interrupt flag */
    via_read(dev, VIA_REG_T2CL);
    if (dev->ifr & VIA_INT_T2) {
        fail("VIA Timer 2", "Reading T2CL should clear T2 interrupt flag");
        via_destroy(dev);
        return;
    }

    via_destroy(dev);
    pass("VIA Timer 2");
}

/* Test VIA interrupt enable register */
static void test_via_ier(void) {
    via_t *dev;

    dev = via_create();
    if (dev == NULL) {
        fail("VIA IER", "Failed to create VIA");
        return;
    }

    /* Enable T1 and T2 interrupts (bit 7 = 1 means set) */
    via_write(dev, VIA_REG_IER, 0x80 | VIA_INT_T1 | VIA_INT_T2);
    if ((dev->ier & (VIA_INT_T1 | VIA_INT_T2)) != (VIA_INT_T1 | VIA_INT_T2)) {
        fail("VIA IER", "Failed to enable interrupts");
        via_destroy(dev);
        return;
    }

    /* IER read should have bit 7 set */
    if (!(via_read(dev, VIA_REG_IER) & 0x80)) {
        fail("VIA IER", "IER bit 7 should always read as 1");
        via_destroy(dev);
        return;
    }

    /* Disable T1 interrupt (bit 7 = 0 means clear) */
    via_write(dev, VIA_REG_IER, VIA_INT_T1);
    if (dev->ier & VIA_INT_T1) {
        fail("VIA IER", "Failed to disable T1 interrupt");
        via_destroy(dev);
        return;
    }
    if (!(dev->ier & VIA_INT_T2)) {
        fail("VIA IER", "T2 interrupt should still be enabled");
        via_destroy(dev);
        return;
    }

    via_destroy(dev);
    pass("VIA IER");
}

/* Test VIA interrupt flag register */
static void test_via_ifr(void) {
    via_t *dev;
    byte ifr_val;

    dev = via_create();
    if (dev == NULL) {
        fail("VIA IFR", "Failed to create VIA");
        return;
    }

    /* Set some flags manually */
    dev->ifr = VIA_INT_T1 | VIA_INT_T2;

    /* Without IER enabled, bit 7 should be clear */
    ifr_val = via_read(dev, VIA_REG_IFR);
    if (ifr_val & VIA_INT_IRQ) {
        fail("VIA IFR", "IRQ bit should be clear when no interrupts enabled");
        via_destroy(dev);
        return;
    }

    /* Enable T1 interrupt */
    dev->ier = VIA_INT_T1;

    /* Now IRQ bit should be set */
    ifr_val = via_read(dev, VIA_REG_IFR);
    if (!(ifr_val & VIA_INT_IRQ)) {
        fail("VIA IFR", "IRQ bit should be set when enabled interrupt is active");
        via_destroy(dev);
        return;
    }

    /* Writing 1 to IFR bits should clear them */
    via_write(dev, VIA_REG_IFR, VIA_INT_T1);
    if (dev->ifr & VIA_INT_T1) {
        fail("VIA IFR", "Writing 1 to IFR should clear T1 flag");
        via_destroy(dev);
        return;
    }
    if (!(dev->ifr & VIA_INT_T2)) {
        fail("VIA IFR", "T2 flag should still be set");
        via_destroy(dev);
        return;
    }

    via_destroy(dev);
    pass("VIA IFR");
}

/* Test VIA IRQ pending function */
static void test_via_irq_pending(void) {
    via_t *dev;

    dev = via_create();
    if (dev == NULL) {
        fail("VIA IRQ pending", "Failed to create VIA");
        return;
    }

    /* No IRQ when no flags set */
    if (via_irq_pending(dev)) {
        fail("VIA IRQ pending", "No IRQ should be pending initially");
        via_destroy(dev);
        return;
    }

    /* Set flag but don't enable interrupt */
    dev->ifr = VIA_INT_T1;
    if (via_irq_pending(dev)) {
        fail("VIA IRQ pending", "No IRQ when interrupt not enabled");
        via_destroy(dev);
        return;
    }

    /* Enable the interrupt */
    dev->ier = VIA_INT_T1;
    if (!via_irq_pending(dev)) {
        fail("VIA IRQ pending", "IRQ should be pending when flag set and enabled");
        via_destroy(dev);
        return;
    }

    /* Test with NULL */
    if (via_irq_pending(NULL)) {
        fail("VIA IRQ pending", "NULL device should return 0");
        via_destroy(dev);
        return;
    }

    via_destroy(dev);
    pass("VIA IRQ pending");
}

/* Test VIA other registers */
static void test_via_other_registers(void) {
    via_t *dev;

    dev = via_create();
    if (dev == NULL) {
        fail("VIA other registers", "Failed to create VIA");
        return;
    }

    /* Test shift register */
    via_write(dev, VIA_REG_SR, 0x5A);
    if (via_read(dev, VIA_REG_SR) != 0x5A) {
        fail("VIA other registers", "Shift register read/write failed");
        via_destroy(dev);
        return;
    }

    /* Test ACR */
    via_write(dev, VIA_REG_ACR, 0xA5);
    if (via_read(dev, VIA_REG_ACR) != 0xA5) {
        fail("VIA other registers", "ACR read/write failed");
        via_destroy(dev);
        return;
    }

    /* Test PCR */
    via_write(dev, VIA_REG_PCR, 0x3C);
    if (via_read(dev, VIA_REG_PCR) != 0x3C) {
        fail("VIA other registers", "PCR read/write failed");
        via_destroy(dev);
        return;
    }

    via_destroy(dev);
    pass("VIA other registers");
}

/* Test VIA with NULL device pointer */
static void test_via_null_device(void) {
    byte result;

    /* These should not crash and should return sensible values */
    result = via_read(NULL, VIA_REG_PORTA);
    if (result != 0xFF) {
        fail("VIA NULL device", "Read from NULL should return 0xFF");
        return;
    }

    via_write(NULL, VIA_REG_PORTA, 0x42);
    via_tick(NULL);
    /* Just verify they don't crash */

    pass("VIA NULL device");
}

/*
 * ============================================================================
 * File I/O Device Tests
 * ============================================================================
 */

/* Test File I/O creation and destruction */
static void test_fileio_create_destroy(void) {
    fileio_t *dev;

    dev = fileio_create();
    if (dev == NULL) {
        fail("FileIO create/destroy", "Failed to create FileIO device");
        return;
    }

    fileio_destroy(dev);

    /* Test destroy with NULL (should not crash) */
    fileio_destroy(NULL);

    pass("FileIO create/destroy");
}

/* Test File I/O reset */
static void test_fileio_reset(void) {
    fileio_t *dev;

    dev = fileio_create();
    if (dev == NULL) {
        fail("FileIO reset", "Failed to create FileIO device");
        return;
    }

    /* Set some values */
    dev->status = 0xFF;
    dev->data = 0xAA;
    dev->name_index = 50;
    strcpy(dev->filename, "test.txt");

    /* Reset and verify */
    fileio_reset(dev);

    if (dev->status != FILEIO_STATUS_READY) {
        fail("FileIO reset", "Status should be READY after reset");
        fileio_destroy(dev);
        return;
    }
    if (dev->data != 0x00) {
        fail("FileIO reset", "Data should be cleared");
        fileio_destroy(dev);
        return;
    }
    if (dev->name_index != 0) {
        fail("FileIO reset", "Name index should be cleared");
        fileio_destroy(dev);
        return;
    }
    if (dev->filename[0] != '\0') {
        fail("FileIO reset", "Filename should be cleared");
        fileio_destroy(dev);
        return;
    }

    /* Test reset with NULL (should not crash) */
    fileio_reset(NULL);

    fileio_destroy(dev);
    pass("FileIO reset");
}

/* Test File I/O filename handling */
static void test_fileio_filename(void) {
    fileio_t *dev;
    const char *testname = "test.txt";
    int i;

    dev = fileio_create();
    if (dev == NULL) {
        fail("FileIO filename", "Failed to create FileIO device");
        return;
    }

    /* Write filename character by character */
    fileio_write(dev, FILEIO_REG_NAMEINDEX, 0);
    for (i = 0; testname[i] != '\0'; i++) {
        fileio_write(dev, FILEIO_REG_NAMECHAR, (byte)testname[i]);
    }

    /* Verify name_index auto-incremented */
    if (fileio_read(dev, FILEIO_REG_NAMEINDEX) != strlen(testname)) {
        fail("FileIO filename", "Name index should auto-increment");
        fileio_destroy(dev);
        return;
    }

    /* Read back filename */
    fileio_write(dev, FILEIO_REG_NAMEINDEX, 0);
    for (i = 0; testname[i] != '\0'; i++) {
        byte c;
        fileio_write(dev, FILEIO_REG_NAMEINDEX, (byte)i);
        c = fileio_read(dev, FILEIO_REG_NAMECHAR);
        if (c != (byte)testname[i]) {
            fail("FileIO filename", "Filename character mismatch");
            fileio_destroy(dev);
            return;
        }
    }

    fileio_destroy(dev);
    pass("FileIO filename");
}

/* Test File I/O data register */
static void test_fileio_data(void) {
    fileio_t *dev;

    dev = fileio_create();
    if (dev == NULL) {
        fail("FileIO data register", "Failed to create FileIO device");
        return;
    }

    /* Write and read data register */
    fileio_write(dev, FILEIO_REG_DATA, 0x42);
    if (fileio_read(dev, FILEIO_REG_DATA) != 0x42) {
        fail("FileIO data register", "Data register read/write failed");
        fileio_destroy(dev);
        return;
    }

    fileio_destroy(dev);
    pass("FileIO data register");
}

/* Test File I/O file operations */
static void test_fileio_operations(void) {
    fileio_t *dev;
    const char *filename = "/tmp/v6502c_test.txt";
    const char *testdata = "Hello, 6502!";
    FILE *f;
    int i;
    byte status;

    dev = fileio_create();
    if (dev == NULL) {
        fail("FileIO operations", "Failed to create FileIO device");
        return;
    }

    /* Create a test file */
    f = fopen(filename, "w");
    if (f == NULL) {
        fail("FileIO operations", "Failed to create test file");
        fileio_destroy(dev);
        return;
    }
    fputs(testdata, f);
    fclose(f);

    /* Set filename */
    fileio_write(dev, FILEIO_REG_NAMEINDEX, 0);
    for (i = 0; filename[i] != '\0'; i++) {
        fileio_write(dev, FILEIO_REG_NAMECHAR, (byte)filename[i]);
    }

    /* Open for reading */
    fileio_write(dev, FILEIO_REG_STATUS, FILEIO_CMD_OPEN_R);
    status = fileio_read(dev, FILEIO_REG_STATUS);
    if (!(status & FILEIO_STATUS_OPEN)) {
        fail("FileIO operations", "File should be open");
        remove(filename);
        fileio_destroy(dev);
        return;
    }
    if (status & FILEIO_STATUS_ERR) {
        fail("FileIO operations", "Error flag should not be set");
        remove(filename);
        fileio_destroy(dev);
        return;
    }

    /* Read first character */
    fileio_write(dev, FILEIO_REG_STATUS, FILEIO_CMD_READ);
    if (fileio_read(dev, FILEIO_REG_DATA) != 'H') {
        fail("FileIO operations", "First character should be 'H'");
        remove(filename);
        fileio_destroy(dev);
        return;
    }

    /* Close file */
    fileio_write(dev, FILEIO_REG_STATUS, FILEIO_CMD_CLOSE);
    status = fileio_read(dev, FILEIO_REG_STATUS);
    if (status & FILEIO_STATUS_OPEN) {
        fail("FileIO operations", "File should be closed");
        remove(filename);
        fileio_destroy(dev);
        return;
    }

    /* Clean up */
    remove(filename);
    fileio_destroy(dev);
    pass("FileIO operations");
}

/* Test File I/O write operations */
static void test_fileio_write_operations(void) {
    fileio_t *dev;
    const char *filename = "/tmp/v6502c_test_write.txt";
    FILE *f;
    int i, c;
    byte status;

    dev = fileio_create();
    if (dev == NULL) {
        fail("FileIO write operations", "Failed to create FileIO device");
        return;
    }

    /* Set filename */
    fileio_write(dev, FILEIO_REG_NAMEINDEX, 0);
    for (i = 0; filename[i] != '\0'; i++) {
        fileio_write(dev, FILEIO_REG_NAMECHAR, (byte)filename[i]);
    }

    /* Open for writing */
    fileio_write(dev, FILEIO_REG_STATUS, FILEIO_CMD_OPEN_W);
    status = fileio_read(dev, FILEIO_REG_STATUS);
    if (!(status & FILEIO_STATUS_OPEN)) {
        fail("FileIO write operations", "File should be open for writing");
        fileio_destroy(dev);
        return;
    }

    /* Write a character */
    fileio_write(dev, FILEIO_REG_DATA, 'X');
    fileio_write(dev, FILEIO_REG_STATUS, FILEIO_CMD_WRITE);

    /* Close file */
    fileio_write(dev, FILEIO_REG_STATUS, FILEIO_CMD_CLOSE);

    /* Verify the file was written */
    f = fopen(filename, "r");
    if (f == NULL) {
        fail("FileIO write operations", "Could not open written file");
        fileio_destroy(dev);
        return;
    }
    c = fgetc(f);
    fclose(f);

    if (c != 'X') {
        fail("FileIO write operations", "Written character mismatch");
        remove(filename);
        fileio_destroy(dev);
        return;
    }

    /* Clean up */
    remove(filename);
    fileio_destroy(dev);
    pass("FileIO write operations");
}

/* Test File I/O error handling */
static void test_fileio_errors(void) {
    fileio_t *dev;
    const char *badfilename = "/nonexistent/path/file.txt";
    int i;
    byte status;

    dev = fileio_create();
    if (dev == NULL) {
        fail("FileIO errors", "Failed to create FileIO device");
        return;
    }

    /* Set non-existent filename */
    fileio_write(dev, FILEIO_REG_NAMEINDEX, 0);
    for (i = 0; badfilename[i] != '\0'; i++) {
        fileio_write(dev, FILEIO_REG_NAMECHAR, (byte)badfilename[i]);
    }

    /* Try to open for reading */
    fileio_write(dev, FILEIO_REG_STATUS, FILEIO_CMD_OPEN_R);
    status = fileio_read(dev, FILEIO_REG_STATUS);
    if (!(status & FILEIO_STATUS_ERR)) {
        fail("FileIO errors", "Error flag should be set for non-existent file");
        fileio_destroy(dev);
        return;
    }

    /* Try to read without open file */
    fileio_reset(dev);
    fileio_write(dev, FILEIO_REG_STATUS, FILEIO_CMD_READ);
    status = fileio_read(dev, FILEIO_REG_STATUS);
    if (!(status & FILEIO_STATUS_ERR)) {
        fail("FileIO errors", "Error flag should be set when reading without open file");
        fileio_destroy(dev);
        return;
    }

    /* Try to write without open file */
    fileio_reset(dev);
    fileio_write(dev, FILEIO_REG_DATA, 0x42);
    fileio_write(dev, FILEIO_REG_STATUS, FILEIO_CMD_WRITE);
    status = fileio_read(dev, FILEIO_REG_STATUS);
    if (!(status & FILEIO_STATUS_ERR)) {
        fail("FileIO errors", "Error flag should be set when writing without open file");
        fileio_destroy(dev);
        return;
    }

    fileio_destroy(dev);
    pass("FileIO errors");
}

/* Test File I/O EOF handling */
static void test_fileio_eof(void) {
    fileio_t *dev;
    const char *filename = "/tmp/v6502c_test_eof.txt";
    FILE *f;
    int i;
    byte status;

    dev = fileio_create();
    if (dev == NULL) {
        fail("FileIO EOF", "Failed to create FileIO device");
        return;
    }

    /* Create an empty test file */
    f = fopen(filename, "w");
    if (f == NULL) {
        fail("FileIO EOF", "Failed to create test file");
        fileio_destroy(dev);
        return;
    }
    fclose(f);

    /* Set filename */
    fileio_write(dev, FILEIO_REG_NAMEINDEX, 0);
    for (i = 0; filename[i] != '\0'; i++) {
        fileio_write(dev, FILEIO_REG_NAMECHAR, (byte)filename[i]);
    }

    /* Open for reading */
    fileio_write(dev, FILEIO_REG_STATUS, FILEIO_CMD_OPEN_R);

    /* Read from empty file */
    fileio_write(dev, FILEIO_REG_STATUS, FILEIO_CMD_READ);
    status = fileio_read(dev, FILEIO_REG_STATUS);
    if (!(status & FILEIO_STATUS_EOF)) {
        fail("FileIO EOF", "EOF flag should be set when reading empty file");
        remove(filename);
        fileio_destroy(dev);
        return;
    }

    /* Clean up */
    remove(filename);
    fileio_destroy(dev);
    pass("FileIO EOF");
}

/* Test FileIO with NULL device pointer */
static void test_fileio_null_device(void) {
    byte result;

    /* These should not crash and should return sensible values */
    result = fileio_read(NULL, FILEIO_REG_STATUS);
    if (result != 0xFF) {
        fail("FileIO NULL device", "Read from NULL should return 0xFF");
        return;
    }

    fileio_write(NULL, FILEIO_REG_STATUS, FILEIO_CMD_RESET);
    /* Just verify it doesn't crash */

    pass("FileIO NULL device");
}

/*
 * ============================================================================
 * Main test runner
 * ============================================================================
 */

int main(void) {
    printf("Device Emulation Test Suite\n");
    printf("============================\n\n");

    printf("--- ACIA (6551) Tests ---\n");
    test_acia_create_destroy();
    test_acia_reset();
    test_acia_status();
    test_acia_data_read();
    test_acia_data_write();
    test_acia_command_control();
    test_acia_null_device();

    printf("\n--- VIA (6522) Tests ---\n");
    test_via_create_destroy();
    test_via_reset();
    test_via_ports();
    test_via_timer1();
    test_via_timer1_continuous();
    test_via_timer2();
    test_via_ier();
    test_via_ifr();
    test_via_irq_pending();
    test_via_other_registers();
    test_via_null_device();

    printf("\n--- File I/O Device Tests ---\n");
    test_fileio_create_destroy();
    test_fileio_reset();
    test_fileio_filename();
    test_fileio_data();
    test_fileio_operations();
    test_fileio_write_operations();
    test_fileio_errors();
    test_fileio_eof();
    test_fileio_null_device();

    printf("\n============================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
