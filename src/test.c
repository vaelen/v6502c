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
#include "test.h"

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
    
    test_cleanup();
    
    printf("\n========================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
