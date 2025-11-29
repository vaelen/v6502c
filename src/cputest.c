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
#include "cputest.h"

/* Test result counters */
int tests_passed = 0;
int tests_failed = 0;

/* Test memory array */
static byte test_memory[0x10000];
static cpu test_cpu;

/* Test framework functions */
void pass(const char *test_name) {
    printf("Testing %s... " COLOR_GREEN "passed" COLOR_RESET "\n", test_name);
    tests_passed++;
}

void fail(const char *test_name, const char *reason) {
    printf("Testing %s... " COLOR_RED "failed" COLOR_RESET ": %s\n", test_name, reason);
    tests_failed++;
}

/* Memory simulation functions */
byte test_read(address a) {
    return test_memory[a];
}

void test_write(address a, byte b) {
    test_memory[a] = b;
}

/* CPU test helpers */
void test_init(void) {
    memset(test_memory, 0, sizeof(test_memory));
    cpu_init(&test_cpu);
    test_cpu.read = test_read;
    test_cpu.write = test_write;
    test_cpu.tick = NULL;
    
    /* Set reset vector to point to 0x0200 */
    test_memory[0xFFFC] = 0x00;
    test_memory[0xFFFD] = 0x02;
}

void test_cleanup(void) {
    /* Nothing to clean up for now */
}

void test_reset_cpu(void) {
    memset(test_memory, 0, sizeof(test_memory));
    /* Set reset vector to point to 0x0200 */
    test_memory[0xFFFC] = 0x00;
    test_memory[0xFFFD] = 0x02;
    cpu_reset(&test_cpu);
    cpu_step(&test_cpu); /* Execute the reset */
    test_cpu.pc = 0x0200; /* Start tests at known location */
}

/* Helper to check if a flag is set */
static bool check_flag(byte flag) {
    return (test_cpu.sr & (1 << flag)) != 0;
}

/* ADC Binary Mode Tests */
void test_adc_binary(void) {
    test_reset_cpu();
    
    /* Test basic addition without carry */
    test_cpu.a = 0x50;
    test_memory[0x0200] = 0x69; /* ADC #$30 */
    test_memory[0x0201] = 0x30;
    cpu_step(&test_cpu);
    
    if (test_cpu.a != 0x80) {
        fail("ADC binary basic", "Expected A=$80, got different value");
        return;
    }
    if (check_flag(0)) { /* CARRY_FLAG */
        fail("ADC binary basic", "Carry flag should be clear");
        return;
    }
    
    /* Test addition with carry set */
    test_reset_cpu();
    test_cpu.a = 0xFF;
    test_cpu.sr |= (1 << 0); /* Set carry */
    test_memory[0x0200] = 0x69; /* ADC #$01 */
    test_memory[0x0201] = 0x01;
    cpu_step(&test_cpu);
    
    if (test_cpu.a != 0x01) {
        fail("ADC binary carry", "Expected A=$01 after overflow");
        return;
    }
    if (!check_flag(0)) { /* Should set carry on overflow */
        fail("ADC binary carry", "Carry flag should be set on overflow");
        return;
    }
    
    /* Test signed overflow */
    test_reset_cpu();
    test_cpu.a = 0x7F; /* +127 */
    test_memory[0x0200] = 0x69; /* ADC #$01 */
    test_memory[0x0201] = 0x01; /* +1 */
    cpu_step(&test_cpu);
    
    if (!check_flag(6)) { /* OVERFLOW_FLAG */
        fail("ADC binary overflow", "Overflow flag should be set (+127 + 1 = -128)");
        return;
    }
    
    pass("ADC binary mode");
}

/* ADC BCD Mode Tests */
void test_adc_bcd(void) {
    test_reset_cpu();
    test_cpu.sr |= (1 << 3); /* Set decimal flag */
    
    /* Test basic BCD addition */
    test_cpu.a = 0x09;
    test_memory[0x0200] = 0x69; /* ADC #$08 */
    test_memory[0x0201] = 0x08;
    cpu_step(&test_cpu);
    
    if (test_cpu.a != 0x17) {
        fail("ADC BCD basic", "Expected A=$17 (BCD 9+8=17)");
        return;
    }
    
    /* Test BCD carry */
    test_reset_cpu();
    test_cpu.sr |= (1 << 3); /* Set decimal flag */
    test_cpu.a = 0x99;
    test_memory[0x0200] = 0x69; /* ADC #$01 */
    test_memory[0x0201] = 0x01;
    cpu_step(&test_cpu);
    
    if (test_cpu.a != 0x00 || !check_flag(0)) {
        fail("ADC BCD carry", "Expected A=$00 with carry set (BCD 99+1=100)");
        return;
    }
    
    pass("ADC BCD mode");
}

/* SBC Binary Mode Tests */
void test_sbc_binary(void) {
    test_reset_cpu();
    test_cpu.sr |= (1 << 0); /* Set carry (no borrow) */
    
    /* Test basic subtraction */
    test_cpu.a = 0x50;
    test_memory[0x0200] = 0xE9; /* SBC #$30 */
    test_memory[0x0201] = 0x30;
    cpu_step(&test_cpu);
    
    if (test_cpu.a != 0x20) {
        fail("SBC binary basic", "Expected A=$20");
        return;
    }
    if (!check_flag(0)) { /* Should maintain carry (no borrow) */
        fail("SBC binary basic", "Carry should be set (no borrow occurred)");
        return;
    }
    
    /* Test subtraction with borrow */
    test_reset_cpu();
    test_cpu.sr &= ~(1 << 0); /* Clear carry (borrow) */
    test_cpu.a = 0x50;
    test_memory[0x0200] = 0xE9; /* SBC #$30 */
    test_memory[0x0201] = 0x30;
    cpu_step(&test_cpu);
    
    if (test_cpu.a != 0x1F) { /* 0x50 - 0x30 - 1 = 0x1F */
        fail("SBC binary borrow", "Expected A=$1F with borrow");
        return;
    }
    
    pass("SBC binary mode");
}

/* SBC BCD Mode Tests */
void test_sbc_bcd(void) {
    test_reset_cpu();
    test_cpu.sr |= (1 << 3); /* Set decimal flag */
    test_cpu.sr |= (1 << 0); /* Set carry (no borrow) */
    
    /* Test basic BCD subtraction */
    test_cpu.a = 0x17;
    test_memory[0x0200] = 0xE9; /* SBC #$08 */
    test_memory[0x0201] = 0x08;
    cpu_step(&test_cpu);
    
    if (test_cpu.a != 0x09) {
        fail("SBC BCD basic", "Expected A=$09 (BCD 17-8=9)");
        return;
    }
    
    pass("SBC BCD mode");
}

/* Logical AND Tests */
void test_and(void) {
    test_reset_cpu();
    
    test_cpu.a = 0xF0;
    test_memory[0x0200] = 0x29; /* AND #$0F */
    test_memory[0x0201] = 0x0F;
    cpu_step(&test_cpu);
    
    if (test_cpu.a != 0x00) {
        fail("AND operation", "Expected A=$00");
        return;
    }
    if (!check_flag(1)) { /* ZERO_FLAG */
        fail("AND operation", "Zero flag should be set");
        return;
    }
    
    pass("AND operation");
}

/* Arithmetic Shift Left Tests */
void test_asl(void) {
    test_reset_cpu();
    
    test_cpu.a = 0x80;
    test_memory[0x0200] = 0x0A; /* ASL A */
    cpu_step(&test_cpu);
    
    if (test_cpu.a != 0x00) {
        fail("ASL accumulator", "Expected A=$00");
        return;
    }
    if (!check_flag(0)) { /* CARRY_FLAG */
        fail("ASL accumulator", "Carry flag should be set");
        return;
    }
    if (!check_flag(1)) { /* ZERO_FLAG */
        fail("ASL accumulator", "Zero flag should be set");
        return;
    }
    
    pass("ASL accumulator");
}

/* BIT Test */
void test_bit(void) {
    test_reset_cpu();
    
    test_cpu.a = 0xFF;
    test_memory[0x0200] = 0x24; /* BIT $80 */
    test_memory[0x0201] = 0x80;
    test_memory[0x80] = 0xC0; /* Bits 7,6 set */
    cpu_step(&test_cpu);
    
    if (!check_flag(7)) { /* NEGATIVE_FLAG */
        fail("BIT instruction", "Negative flag should be set");
        return;
    }
    if (!check_flag(6)) { /* OVERFLOW_FLAG */
        fail("BIT instruction", "Overflow flag should be set");
        return;
    }
    
    pass("BIT instruction");
}

/* Branch Instructions Tests */
void test_branches(void) {
    address old_pc;
    test_reset_cpu();
    
    /* Test BEQ when zero flag is set */
    test_cpu.sr |= (1 << 1); /* Set zero flag */
    test_memory[0x0200] = 0xF0; /* BEQ +$10 */
    test_memory[0x0201] = 0x10;
    old_pc = test_cpu.pc;
    cpu_step(&test_cpu);
    
    if (test_cpu.pc != old_pc + 2 + 0x10) {
        fail("BEQ branch", "Branch should have been taken");
        return;
    }
    
    pass("Branch instructions");
}

/* Compare Operations Tests */
void test_compare_ops(void) {
    test_reset_cpu();
    
    /* Test CMP with equal values */
    test_cpu.a = 0x50;
    test_memory[0x0200] = 0xC9; /* CMP #$50 */
    test_memory[0x0201] = 0x50;
    cpu_step(&test_cpu);
    
    if (!check_flag(1)) { /* ZERO_FLAG */
        fail("CMP equal", "Zero flag should be set for equal values");
        return;
    }
    if (!check_flag(0)) { /* CARRY_FLAG */
        fail("CMP equal", "Carry flag should be set for A >= operand");
        return;
    }
    
    pass("Compare operations");
}

/* Decrement/Increment Tests */
void test_dec_inc(void) {
    test_reset_cpu();
    
    /* Test DEC */
    test_memory[0x0200] = 0xC6; /* DEC $80 */
    test_memory[0x0201] = 0x80;
    test_memory[0x80] = 0x01;
    cpu_step(&test_cpu);
    
    if (test_memory[0x80] != 0x00) {
        fail("DEC instruction", "Expected memory=$00");
        return;
    }
    if (!check_flag(1)) { /* ZERO_FLAG */
        fail("DEC instruction", "Zero flag should be set");
        return;
    }
    
    pass("Decrement/Increment operations");
}

/* Exclusive OR Tests */
void test_eor(void) {
    test_reset_cpu();
    
    test_cpu.a = 0xFF;
    test_memory[0x0200] = 0x49; /* EOR #$FF */
    test_memory[0x0201] = 0xFF;
    cpu_step(&test_cpu);
    
    if (test_cpu.a != 0x00) {
        fail("EOR operation", "Expected A=$00");
        return;
    }
    if (!check_flag(1)) { /* ZERO_FLAG */
        fail("EOR operation", "Zero flag should be set");
        return;
    }
    
    pass("EOR operation");
}

/* Flag Operations Tests */
void test_flags(void) {
    test_reset_cpu();
    
    /* Test SEC */
    test_memory[0x0200] = 0x38; /* SEC */
    cpu_step(&test_cpu);
    
    if (!check_flag(0)) { /* CARRY_FLAG */
        fail("SEC instruction", "Carry flag should be set");
        return;
    }
    
    /* Test CLC */
    test_cpu.pc = 0x0200;
    test_memory[0x0200] = 0x18; /* CLC */
    cpu_step(&test_cpu);
    
    if (check_flag(0)) { /* CARRY_FLAG */
        fail("CLC instruction", "Carry flag should be clear");
        return;
    }
    
    pass("Flag operations");
}

/* Jump and Subroutine Tests */
void test_jmp_jsr(void) {
    test_reset_cpu();
    
    /* Test JMP absolute */
    test_memory[0x0200] = 0x4C; /* JMP $1000 */
    test_memory[0x0201] = 0x00;
    test_memory[0x0202] = 0x10;
    cpu_step(&test_cpu);
    
    if (test_cpu.pc != 0x1000) {
        fail("JMP absolute", "PC should be $1000");
        return;
    }
    
    pass("Jump and subroutine operations");
}

/* Load and Store Tests */
void test_load_store(void) {
    test_reset_cpu();
    
    /* Test LDA immediate */
    test_memory[0x0200] = 0xA9; /* LDA #$42 */
    test_memory[0x0201] = 0x42;
    cpu_step(&test_cpu);
    
    if (test_cpu.a != 0x42) {
        fail("LDA immediate", "Expected A=$42");
        return;
    }
    
    /* Test STA absolute */
    test_cpu.pc = 0x0200;
    test_memory[0x0200] = 0x8D; /* STA $1000 */
    test_memory[0x0201] = 0x00;
    test_memory[0x0202] = 0x10;
    cpu_step(&test_cpu);
    
    if (test_memory[0x1000] != 0x42) {
        fail("STA absolute", "Expected memory[$1000]=$42");
        return;
    }
    
    pass("Load and store operations");
}

/* Logical Shift Tests */
void test_logical_shifts(void) {
    test_reset_cpu();
    
    /* Test LSR */
    test_cpu.a = 0x81;
    test_memory[0x0200] = 0x4A; /* LSR A */
    cpu_step(&test_cpu);
    
    if (test_cpu.a != 0x40) {
        fail("LSR accumulator", "Expected A=$40");
        return;
    }
    if (!check_flag(0)) { /* CARRY_FLAG */
        fail("LSR accumulator", "Carry flag should be set");
        return;
    }
    
    pass("Logical shift operations");
}

/* NOP Test */
void test_nop(void) {
    cpu state_before;
    test_reset_cpu();
    
    state_before = test_cpu;
    test_memory[0x0200] = 0xEA; /* NOP */
    cpu_step(&test_cpu);
    
    /* Only PC should have changed */
    if (test_cpu.a != state_before.a || test_cpu.x != state_before.x || 
        test_cpu.y != state_before.y || test_cpu.sr != state_before.sr ||
        test_cpu.sp != state_before.sp) {
        fail("NOP instruction", "NOP should not change registers");
        return;
    }
    
    pass("NOP instruction");
}

/* Logical OR Tests */
void test_ora(void) {
    test_reset_cpu();
    
    test_cpu.a = 0x0F;
    test_memory[0x0200] = 0x09; /* ORA #$F0 */
    test_memory[0x0201] = 0xF0;
    cpu_step(&test_cpu);
    
    if (test_cpu.a != 0xFF) {
        fail("ORA operation", "Expected A=$FF");
        return;
    }
    if (!check_flag(7)) { /* NEGATIVE_FLAG */
        fail("ORA operation", "Negative flag should be set");
        return;
    }
    
    pass("ORA operation");
}

/* Push and Pull Tests */
void test_push_pull(void) {
    byte old_sp;
    test_reset_cpu();
    
    /* Test PHA */
    test_cpu.a = 0x42;
    test_memory[0x0200] = 0x48; /* PHA */
    old_sp = test_cpu.sp;
    cpu_step(&test_cpu);
    
    if (test_cpu.sp != old_sp - 1) {
        fail("PHA instruction", "Stack pointer should decrement");
        return;
    }
    if (test_memory[0x0100 + old_sp] != 0x42) {
        fail("PHA instruction", "Value should be pushed to stack");
        return;
    }
    
    /* Test PLA */
    test_cpu.a = 0x00;
    test_cpu.pc = 0x0200;
    test_memory[0x0200] = 0x68; /* PLA */
    cpu_step(&test_cpu);
    
    if (test_cpu.a != 0x42) {
        fail("PLA instruction", "Expected A=$42");
        return;
    }
    
    pass("Push and pull operations");
}

/* Rotate Tests */
void test_rotates(void) {
    test_reset_cpu();
    
    /* Test ROL with carry clear */
    test_cpu.a = 0x80;
    test_memory[0x0200] = 0x2A; /* ROL A */
    cpu_step(&test_cpu);
    
    if (test_cpu.a != 0x00) {
        fail("ROL accumulator", "Expected A=$00");
        return;
    }
    if (!check_flag(0)) { /* CARRY_FLAG */
        fail("ROL accumulator", "Carry flag should be set");
        return;
    }
    
    pass("Rotate operations");
}

/* Transfer Instructions Tests */
void test_transfers(void) {
    test_reset_cpu();
    
    /* Test TAX */
    test_cpu.a = 0x42;
    test_memory[0x0200] = 0xAA; /* TAX */
    cpu_step(&test_cpu);
    
    if (test_cpu.x != 0x42) {
        fail("TAX instruction", "Expected X=$42");
        return;
    }
    
    pass("Transfer operations");
}

/* CPU Variant Tests - Original 6502 BCD Overflow */
void test_cpu_variant_6502(void) {
    test_reset_cpu();
    cpu_set_variant(&test_cpu, CPU_6502);
    
    /* Test ADC: V flag is cleared in BCD mode (6502 behavior) */
    test_cpu.sr |= (1 << 3); /* Set decimal flag */
    test_cpu.a = 0x7F; /* +127 in binary, but 79 in BCD */
    test_memory[0x0200] = 0x69; /* ADC #$01 */
    test_memory[0x0201] = 0x01;
    cpu_step(&test_cpu);
    
    if ((test_cpu.sr & (1 << 6)) != 0) { /* OVERFLOW_FLAG */
        fail("6502 ADC BCD overflow", "V flag should be clear in BCD mode on original 6502");
        return;
    }
    
    /* Test SBC: V flag is also cleared in BCD mode (6502 behavior) */
    test_reset_cpu();
    cpu_set_variant(&test_cpu, CPU_6502);
    test_cpu.sr |= (1 << 3); /* Set decimal flag */
    test_cpu.sr |= (1 << 0); /* Set carry (no borrow) */
    test_cpu.a = 0x00; /* 0 in both binary and BCD */
    test_memory[0x0200] = 0xE9; /* SBC #$7F */
    test_memory[0x0201] = 0x7F;
    cpu_step(&test_cpu);
    
    if ((test_cpu.sr & (1 << 6)) != 0) { /* OVERFLOW_FLAG */
        fail("6502 SBC BCD overflow", "V flag should be clear in BCD mode on original 6502");
        return;
    }
    
    pass("6502 BCD overflow behavior");
}

/* CPU Variant Tests - 65C02 BCD Overflow */
void test_cpu_variant_65c02(void) {
    test_reset_cpu();
    cpu_set_variant(&test_cpu, CPU_65C02);
    
    /* Test ADC: V flag should work in BCD mode (65C02 behavior) */
    test_cpu.sr |= (1 << 3); /* Set decimal flag */
    test_cpu.a = 0x7F; /* +127 in binary, triggers signed overflow */
    test_memory[0x0200] = 0x69; /* ADC #$01 */
    test_memory[0x0201] = 0x01;
    cpu_step(&test_cpu);
    
    if ((test_cpu.sr & (1 << 6)) == 0) { /* OVERFLOW_FLAG */
        fail("65C02 ADC BCD overflow", "V flag should be set for signed overflow in 65C02 BCD mode");
        return;
    }
    
    /* Test SBC: V flag should also work in BCD mode (65C02 behavior) */
    test_reset_cpu();
    cpu_set_variant(&test_cpu, CPU_65C02);
    test_cpu.sr |= (1 << 3); /* Set decimal flag */
    test_cpu.sr |= (1 << 0); /* Set carry (no borrow) */
    test_cpu.a = 0x80; /* -128 in signed binary, should trigger overflow */
    test_memory[0x0200] = 0xE9; /* SBC #$01 */
    test_memory[0x0201] = 0x01;
    cpu_step(&test_cpu);
    
    if ((test_cpu.sr & (1 << 6)) == 0) { /* OVERFLOW_FLAG */
        fail("65C02 SBC BCD overflow", "V flag should be set for signed overflow in 65C02 BCD mode");
        return;
    }
    
    pass("65C02 BCD overflow behavior");
}

/* JSR/RTS Stack Order Tests */
void test_jsr_rts_stack_order(void) {
    byte pushed_pch, pushed_pcl;

    test_reset_cpu();

    /* Set up a subroutine at 0x1000 that just returns */
    test_memory[0x1000] = 0x60; /* RTS */

    /* Place JSR at 0x0200 - should jump to 0x1000 */
    test_memory[0x0200] = 0x20; /* JSR $1000 */
    test_memory[0x0201] = 0x00;
    test_memory[0x0202] = 0x10;

    /* Execute JSR */
    cpu_step(&test_cpu);

    /* Check PC jumped to subroutine */
    if (test_cpu.pc != 0x1000) {
        fail("JSR/RTS stack order", "JSR should jump to $1000");
        return;
    }

    /* Check stack contents - authentic 6502 pushes PCH first, then PCL */
    /* Return address pushed is PC-1 (pointing to last byte of JSR instruction) */
    /* SP was 0xFD, after two pushes it's 0xFB */
    pushed_pch = test_memory[0x01FD]; /* First pushed (high byte) */
    pushed_pcl = test_memory[0x01FC]; /* Second pushed (low byte) */

    /* The return address should be 0x0202 (last byte of JSR instruction) */
    if (pushed_pch != 0x02 || pushed_pcl != 0x02) {
        fail("JSR/RTS stack order", "JSR should push PCH first then PCL");
        return;
    }

    /* Execute RTS - should return to 0x0203 (return addr + 1) */
    cpu_step(&test_cpu);

    if (test_cpu.pc != 0x0203) {
        fail("JSR/RTS stack order", "RTS should return to address after JSR");
        return;
    }

    pass("JSR/RTS stack order");
}

/* BRK Instruction Tests */
void test_brk(void) {
    byte pushed_pch, pushed_pcl, pushed_sr;

    test_reset_cpu();

    /* Set up IRQ vector to point to handler at 0x2000 */
    test_memory[0xFFFE] = 0x00;
    test_memory[0xFFFF] = 0x20;

    /* Set up a simple handler that just does RTI */
    test_memory[0x2000] = 0x40; /* RTI */

    /* Place BRK at 0x0200 with padding byte */
    test_memory[0x0200] = 0x00; /* BRK */
    test_memory[0x0201] = 0xEA; /* NOP (padding byte, skipped) */
    test_memory[0x0202] = 0xEA; /* NOP (return point) */

    /* Clear IRQ_DISABLE to verify it gets set */
    test_cpu.sr &= ~(1 << 2);

    /* Execute BRK */
    cpu_step(&test_cpu);

    /* Verify PC jumped to handler */
    if (test_cpu.pc != 0x2000) {
        fail("BRK instruction", "PC should jump to IRQ vector");
        return;
    }

    /* Verify SP decremented by 3 (PC high, PC low, SR) */
    if (test_cpu.sp != 0xFA) {
        fail("BRK instruction", "SP should decrement by 3");
        return;
    }

    /* Verify IRQ_DISABLE is now set */
    if (!check_flag(2)) {
        fail("BRK instruction", "IRQ_DISABLE should be set");
        return;
    }

    /* Check stack contents */
    pushed_pch = test_memory[0x01FD];
    pushed_pcl = test_memory[0x01FC];
    pushed_sr = test_memory[0x01FB];

    /* Verify pushed PC points past the padding byte (0x0202) */
    if (pushed_pch != 0x02 || pushed_pcl != 0x02) {
        fail("BRK instruction", "Pushed PC should be $0202 (after padding byte)");
        return;
    }

    /* Verify BREAK flag is set in pushed SR */
    if (!(pushed_sr & (1 << 4))) {
        fail("BRK instruction", "BREAK flag should be set in pushed SR");
        return;
    }

    pass("BRK instruction");
}

/* RTI Instruction Tests */
void test_rti(void) {
    test_reset_cpu();

    /* Manually set up an interrupt stack frame */
    /* Push order was: PCH, PCL, SR - so on stack (from high to low): SR, PCL, PCH */
    test_memory[0x01FD] = 0x12; /* PCH */
    test_memory[0x01FC] = 0x34; /* PCL */
    test_memory[0x01FB] = 0x00; /* SR with all flags clear */
    test_cpu.sp = 0xFA; /* Point below the pushed data */

    /* Set some flags to verify they get restored */
    test_cpu.sr = 0xFF;

    /* Place RTI instruction */
    test_memory[0x0200] = 0x40; /* RTI */

    cpu_step(&test_cpu);

    /* Verify PC was restored */
    if (test_cpu.pc != 0x1234) {
        fail("RTI instruction", "PC should be restored to $1234");
        return;
    }

    /* Verify SP was restored (incremented by 3) */
    if (test_cpu.sp != 0xFD) {
        fail("RTI instruction", "SP should be $FD after RTI");
        return;
    }

    pass("RTI instruction");
}

/* Hardware IRQ Tests */
void test_irq(void) {
    byte pushed_sr;

    test_reset_cpu();

    /* Set up IRQ vector */
    test_memory[0xFFFE] = 0x00;
    test_memory[0xFFFF] = 0x30; /* Handler at 0x3000 */

    /* Place NOP instruction for CPU to execute */
    test_memory[0x0200] = 0xEA; /* NOP */

    /* Clear IRQ_DISABLE to allow interrupts */
    test_cpu.sr &= ~(1 << 2);

    /* Trigger IRQ */
    cpu_irq(&test_cpu);

    /* Execute one instruction - IRQ should be serviced after */
    cpu_step(&test_cpu);

    /* Verify PC jumped to handler */
    if (test_cpu.pc != 0x3000) {
        fail("IRQ handling", "PC should jump to IRQ vector");
        return;
    }

    /* Verify BREAK flag is CLEAR in pushed SR (hardware interrupt) */
    pushed_sr = test_memory[0x01FB];
    if (pushed_sr & (1 << 4)) {
        fail("IRQ handling", "BREAK flag should be clear in pushed SR for hardware IRQ");
        return;
    }

    pass("IRQ handling");
}

/* NMI Tests */
void test_nmi(void) {
    test_reset_cpu();

    /* Set up NMI vector */
    test_memory[0xFFFA] = 0x00;
    test_memory[0xFFFB] = 0x40; /* Handler at 0x4000 */

    /* Set IRQ_DISABLE - NMI should still work */
    test_cpu.sr |= (1 << 2);

    /* Place NOP instruction */
    test_memory[0x0200] = 0xEA; /* NOP */

    /* Trigger NMI */
    cpu_nmi(&test_cpu);

    /* Execute one instruction - NMI should be serviced after */
    cpu_step(&test_cpu);

    /* Verify PC jumped to NMI handler */
    if (test_cpu.pc != 0x4000) {
        fail("NMI handling", "PC should jump to NMI vector");
        return;
    }

    pass("NMI handling");
}

/* IRQ Masking Tests */
void test_irq_masking(void) {
    test_reset_cpu();

    /* Set up IRQ vector */
    test_memory[0xFFFE] = 0x00;
    test_memory[0xFFFF] = 0x30;

    /* Place NOP instructions */
    test_memory[0x0200] = 0xEA; /* NOP */
    test_memory[0x0201] = 0xEA; /* NOP */

    /* Set IRQ_DISABLE to mask interrupts */
    test_cpu.sr |= (1 << 2);

    /* Trigger IRQ */
    cpu_irq(&test_cpu);

    /* Execute instruction - IRQ should NOT be serviced */
    cpu_step(&test_cpu);

    /* Verify PC did NOT jump to handler */
    if (test_cpu.pc == 0x3000) {
        fail("IRQ masking", "IRQ should be masked when IRQ_DISABLE is set");
        return;
    }

    /* Verify PC advanced normally (past NOP) */
    if (test_cpu.pc != 0x0201) {
        fail("IRQ masking", "PC should advance to next instruction");
        return;
    }

    pass("IRQ masking");
}

/* Interrupt Priority Tests */
void test_interrupt_priority(void) {
    test_reset_cpu();

    /* Set up both vectors */
    test_memory[0xFFFA] = 0x00;
    test_memory[0xFFFB] = 0x40; /* NMI at 0x4000 */
    test_memory[0xFFFE] = 0x00;
    test_memory[0xFFFF] = 0x30; /* IRQ at 0x3000 */

    /* Clear IRQ_DISABLE */
    test_cpu.sr &= ~(1 << 2);

    /* Place NOP */
    test_memory[0x0200] = 0xEA;

    /* Trigger both interrupts */
    cpu_irq(&test_cpu);
    cpu_nmi(&test_cpu);

    /* Execute - NMI should take priority */
    cpu_step(&test_cpu);

    if (test_cpu.pc != 0x4000) {
        fail("Interrupt priority", "NMI should take priority over IRQ");
        return;
    }

    pass("Interrupt priority");
}

/* Shift/Rotate Memory Mode Tests */
void test_shift_rotate_memory(void) {
    test_reset_cpu();

    /* Test ASL with zero-page addressing */
    test_memory[0x0050] = 0x81; /* Value to shift: 10000001 */
    test_memory[0x0200] = 0x06; /* ASL $50 */
    test_memory[0x0201] = 0x50;
    test_cpu.sr &= ~(1 << 0); /* Clear carry */

    cpu_step(&test_cpu);

    /* Result should be 0x02 (00000010), carry should be set from bit 7 */
    if (test_memory[0x0050] != 0x02) {
        fail("ASL memory mode", "Memory should contain shifted value 0x02");
        return;
    }
    if (!check_flag(0)) { /* CARRY_FLAG */
        fail("ASL memory mode", "Carry flag should be set");
        return;
    }

    /* Test LSR with zero-page addressing */
    test_reset_cpu();
    test_memory[0x0050] = 0x81; /* Value to shift: 10000001 */
    test_memory[0x0200] = 0x46; /* LSR $50 */
    test_memory[0x0201] = 0x50;
    test_cpu.sr &= ~(1 << 0); /* Clear carry */

    cpu_step(&test_cpu);

    /* Result should be 0x40 (01000000), carry should be set from bit 0 */
    if (test_memory[0x0050] != 0x40) {
        fail("LSR memory mode", "Memory should contain shifted value 0x40");
        return;
    }
    if (!check_flag(0)) {
        fail("LSR memory mode", "Carry flag should be set");
        return;
    }

    /* Test ROL with zero-page addressing */
    test_reset_cpu();
    test_memory[0x0050] = 0x81; /* Value to rotate: 10000001 */
    test_memory[0x0200] = 0x26; /* ROL $50 */
    test_memory[0x0201] = 0x50;
    test_cpu.sr |= (1 << 0); /* Set carry - will rotate into bit 0 */

    cpu_step(&test_cpu);

    /* Result should be 0x03 (00000011), carry should be set from old bit 7 */
    if (test_memory[0x0050] != 0x03) {
        fail("ROL memory mode", "Memory should contain rotated value 0x03");
        return;
    }
    if (!check_flag(0)) {
        fail("ROL memory mode", "Carry flag should be set");
        return;
    }

    /* Test ROR with zero-page addressing */
    test_reset_cpu();
    test_memory[0x0050] = 0x81; /* Value to rotate: 10000001 */
    test_memory[0x0200] = 0x66; /* ROR $50 */
    test_memory[0x0201] = 0x50;
    test_cpu.sr |= (1 << 0); /* Set carry - will rotate into bit 7 */

    cpu_step(&test_cpu);

    /* Result should be 0xC0 (11000000), carry should be set from old bit 0 */
    if (test_memory[0x0050] != 0xC0) {
        fail("ROR memory mode", "Memory should contain rotated value 0xC0");
        return;
    }
    if (!check_flag(0)) {
        fail("ROR memory mode", "Carry flag should be set");
        return;
    }

    pass("Shift/rotate memory mode");
}

/* Zero-Page Wrapping Tests */
void test_zero_page_wrapping(void) {
    test_reset_cpu();

    /* Test ZPX wrapping: LDA $FF,X with X=2 should read from $01, not $101 */
    test_memory[0x0001] = 0x42; /* Value at wrapped address */
    test_memory[0x0101] = 0x99; /* Value at non-wrapped address (should NOT be read) */
    test_memory[0x0200] = 0xB5; /* LDA $FF,X */
    test_memory[0x0201] = 0xFF;
    test_cpu.x = 0x02;

    cpu_step(&test_cpu);

    if (test_cpu.a != 0x42) {
        fail("Zero-page X wrapping", "LDA $FF,X with X=2 should wrap to $01");
        return;
    }

    /* Test ZPY wrapping: LDX $FF,Y with Y=3 should read from $02, not $102 */
    test_reset_cpu();
    test_memory[0x0002] = 0x37;
    test_memory[0x0102] = 0x88;
    test_memory[0x0200] = 0xB6; /* LDX $FF,Y */
    test_memory[0x0201] = 0xFF;
    test_cpu.y = 0x03;

    cpu_step(&test_cpu);

    if (test_cpu.x != 0x37) {
        fail("Zero-page Y wrapping", "LDX $FF,Y with Y=3 should wrap to $02");
        return;
    }

    /* Test INX wrapping: LDA ($FF,X) with X=1 should read pointer from $00,$01 */
    test_reset_cpu();
    test_memory[0x0000] = 0x00; /* Low byte of pointer (at wrapped address) */
    test_memory[0x0001] = 0x03; /* High byte of pointer */
    test_memory[0x0300] = 0x55; /* Value at target address */
    test_memory[0x0200] = 0xA1; /* LDA ($FF,X) */
    test_memory[0x0201] = 0xFF;
    test_cpu.x = 0x01;

    cpu_step(&test_cpu);

    if (test_cpu.a != 0x55) {
        fail("Pre-indexed indirect wrapping", "LDA ($FF,X) with X=1 should wrap pointer read");
        return;
    }

    /* Test INY pointer wrapping: LDA ($FF),Y with pointer at $FF should read hi from $00 */
    test_reset_cpu();
    test_memory[0x00FF] = 0x00; /* Low byte of pointer */
    test_memory[0x0000] = 0x04; /* High byte at wrapped address $00 */
    test_memory[0x0400] = 0x77; /* Value at target address */
    test_memory[0x0200] = 0xB1; /* LDA ($FF),Y */
    test_memory[0x0201] = 0xFF;
    test_cpu.y = 0x00;

    cpu_step(&test_cpu);

    if (test_cpu.a != 0x77) {
        fail("Post-indexed indirect wrapping", "LDA ($FF),Y pointer hi byte should wrap to $00");
        return;
    }

    pass("Zero-page wrapping");
}

/* PLA Flag Tests */
void test_pla_flags(void) {
    test_reset_cpu();

    /* Test PLA sets zero flag */
    test_cpu.sp = 0xFC;
    test_memory[0x01FD] = 0x00; /* Zero value on stack */
    test_memory[0x0200] = 0x68; /* PLA */
    test_cpu.sr &= ~(1 << 1); /* Clear zero flag */
    test_cpu.sr |= (1 << 7);  /* Set negative flag */

    cpu_step(&test_cpu);

    if (test_cpu.a != 0x00) {
        fail("PLA flags", "A should be 0");
        return;
    }
    if (!check_flag(1)) { /* ZERO_FLAG */
        fail("PLA flags", "Zero flag should be set");
        return;
    }
    if (check_flag(7)) { /* NEGATIVE_FLAG */
        fail("PLA flags", "Negative flag should be clear");
        return;
    }

    /* Test PLA sets negative flag */
    test_reset_cpu();
    test_cpu.sp = 0xFC;
    test_memory[0x01FD] = 0x80; /* Negative value on stack */
    test_memory[0x0200] = 0x68; /* PLA */
    test_cpu.sr |= (1 << 1);  /* Set zero flag */
    test_cpu.sr &= ~(1 << 7); /* Clear negative flag */

    cpu_step(&test_cpu);

    if (test_cpu.a != 0x80) {
        fail("PLA flags", "A should be 0x80");
        return;
    }
    if (check_flag(1)) {
        fail("PLA flags", "Zero flag should be clear");
        return;
    }
    if (!check_flag(7)) {
        fail("PLA flags", "Negative flag should be set");
        return;
    }

    pass("PLA flags");
}

/* Main test runner */
int main(void) {
    printf("6502 Emulator Test Suite\n");
    printf("========================\n\n");
    
    test_init();
    
    /* Run all tests */
    test_adc_binary();
    test_adc_bcd();
    test_sbc_binary();
    test_sbc_bcd();
    test_and();
    test_asl();
    test_bit();
    test_branches();
    test_compare_ops();
    test_dec_inc();
    test_eor();
    test_flags();
    test_jmp_jsr();
    test_load_store();
    test_logical_shifts();
    test_nop();
    test_ora();
    test_push_pull();
    test_rotates();
    test_transfers();
    test_cpu_variant_6502();
    test_cpu_variant_65c02();
    test_jsr_rts_stack_order();
    test_brk();
    test_rti();
    test_irq();
    test_nmi();
    test_irq_masking();
    test_interrupt_priority();
    test_shift_rotate_memory();
    test_zero_page_wrapping();
    test_pla_flags();

    test_cleanup();
    
    printf("\n========================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
