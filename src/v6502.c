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

#include "v6502.h"
#include "inst.h"

#define CARRY_FLAG 0
#define ZERO_FLAG 1
#define IRQ_DISABLE 2
#define BCD_FLAG 3
#define BREAK_FLAG 4
#define OVERFLOW_FLAG 6
#define NEGATIVE_FLAG 7

/** Helper method for setting a bit. */
void _set_bit(cpu *c, byte bit) {
  c->sr = c->sr | (1<<bit);
}

/** Helper method for clearing a bit. */
void _clear_bit(cpu *c, byte bit) {
  c->sr = c->sr & ~(1<<bit);
}

/** Helper method for toggling a bit. */
void _toggle_bit(cpu *c, byte bit) {
  c->sr = c->sr ^ (1<<bit);
}

/** Helper method for checking a bit. */
bool _check_bit(cpu *c, byte bit) {
  return c->sr & (1<<bit);
}

/** Helper method for setting the zero flag appropriately. */
void _set_zero_flag(cpu *c, byte value) {
  if (value == 0) {
    _set_bit(c, ZERO_FLAG);
  } else {
    _clear_bit(c, ZERO_FLAG);
  }
}

/** Helper method for setting the zero flag appropriately. */
void _set_negative_flag(cpu *c, byte value) {
  if (value & (1<<7)) {
    _set_bit(c, NEGATIVE_FLAG);
  } else {
    _clear_bit(c, NEGATIVE_FLAG);
  }
}

/** Helper method to push a value onto the stack. */
void _push(cpu *c, byte value) {
  cpu_write_byte(c, c->sp, value);
  c->sp--;
}

/** Helper method to pop a value off the stack. */
byte _pop(cpu *c) {
  c->sp++;
  return cpu_read_byte(c, c->sp);
}

bool _is_store(enum instruction_t i) {
  return i == I_STA ||
    i == I_STX ||
    i == I_STY;
}

/** Helper method to reset CPU status. */
void _reset(cpu *c) {
  if (c == NULL) return; 
  c->pc = cpu_read_address(c, RESET_VECTOR);
  c->a = 0;
  c->x = 0;
  c->y = 0;
  /*
    Based on the SP from Visual6502 after a reset.
   */
  c->sr = 0x36;
  /*
    In a real 6502, the reset sequence results in three stack push
    operations and therefore the SP will equal 0xFD after a reset.
    See: https://www.pagetable.com/?p=410
  */
  c->sp = 0xFD;
  c->halted = FALSE;
  c->reset = FALSE;
  c->irq = FALSE;
  c->nmi = FALSE;
}

void cpu_init(cpu *c) {
  if (c == NULL) return; 
  c->write = NULL;
  c->read = NULL;
  c->tick = NULL;
  _reset(c);
}

byte cpu_read_byte(cpu *c, address a) {
  byte b = 0;
  if (c == NULL || c->read == NULL) return 0;
  b = c->read(a);
  return b;
}

address cpu_read_address(cpu *c, address a) {
  byte hi = 0, lo = 0;
  address value;
  
  if (c == NULL) return 0;
  lo = cpu_read_byte(c, a++);
  hi = cpu_read_byte(c, a++);
  value = (hi << 8) | lo;
  return value;
}

void cpu_write_byte(cpu *c, address a, byte b) {
  if (c == NULL || c->write == NULL) return;
  c->write(a, b);
}

void cpu_write_address(cpu *c, address a, address value) {
  byte hi = 0, lo = 0;
  if (c == NULL) return;
  lo = (byte) value;
  hi = (byte) value >> 8;
  cpu_write_byte(c, a++, hi);
  cpu_write_byte(c, a++, lo);
}

byte cpu_next_byte(cpu *c) {
  if (c == NULL) return 0;
  return cpu_read_byte(c, c->pc++);
}

address cpu_next_address(cpu *c) {
  address a = 0;
  if (c == NULL) return 0;
  a = cpu_read_address(c, c->pc);
  c->pc += 2;
  return a;
}

void cpu_step(cpu *c) {
  byte b = 0, i = 0, temp = 0, lo = 0, hi = 0;
  signed char offset = 0;
  address a = 0;
  enum instruction_t instruction = I_BRK;
  enum addressing_t addressing = A_IMP;
  
  if (c == NULL) return;

  /** Handle reset. */
  if (c->reset) {
    c->reset = FALSE;
    _reset(c);
    return;
  }
  
  b = cpu_next_byte(c);

  instruction = instructions[b];
  addressing = addressings[b];

  /** load data */
  switch (addressing) {
  case A_ACC:
    /* accumulator */
    b = c->a;
    break;
  case A_ABS:
    /* absolute */
    a = cpu_next_address(c);
    if (!_is_store(instruction)) {
      b = cpu_read_byte(c, a);
    }
    break;
  case A_ABX:
    /* absolute, x-indexed */
    a = cpu_next_address(c) + c->x;
    if (!_is_store(instruction)) {
      b = cpu_read_byte(c, a);
    }
    break;
  case A_ABY:
    /* absolute, y-indexed */
    a = cpu_next_address(c) + c->y;
    if (!_is_store(instruction)) {
      b = cpu_read_byte(c, a);
    }
    break;
  case A_IMM:
    /* immediate */
    b = cpu_next_byte(c);
    break;
  case A_IND:
    /* indirect */
    /* Only used for JMP, result is an address */
    a = cpu_next_address(c);
    a = cpu_read_address(c, a);
    break;
  case A_INX:
    /* pre-indexed indirect */
    a = (address) cpu_next_byte(c) + c->x;
    a = cpu_read_address(c, a);
    if (!_is_store(instruction)) {
      b = cpu_read_byte(c, a);
    }
    break;
  case A_INY:
    /* post-indexed indirect */
    a = (address) cpu_next_byte(c);
    a = cpu_read_address(c, a) + c->y;
    if (!_is_store(instruction)) {
      b = cpu_read_byte(c, a);
    }
    break;
  case A_REL:
    /* relative */
    /* used for branching, result is an address */
    offset = (signed char) cpu_next_byte(c);
    a = c->pc + offset;
    break;
  case A_ZPG:
    /* zero-page */
    a = (address) cpu_next_byte(c);
    if (!_is_store(instruction)) {
      b = cpu_read_byte(c, a);
    }
    break;
  case A_ZPX:
    /* zero-page x-indexed */
    a = (address) cpu_next_byte(c) + c->x;
    if (!_is_store(instruction)) {
      b = cpu_read_byte(c, a);
    }
    break;
  case A_ZPY:
    /* zero-page y-indexed */
    a = (address) cpu_next_byte(c) + c->y;
    if (!_is_store(instruction)) {
      b = cpu_read_byte(c, a);
    }
    break;
  case A_ZPI:
    /* zero-page indirect */
    /* WDC extension for W65C02 */
    a = (address) cpu_next_byte(c);
    a = cpu_read_address(c, a);
    if (!_is_store(instruction)) {
      b = cpu_read_byte(c, a);
    }
    break;
  case A_ABI:
    /* absolute indexed indirect */
    /* WDC extension for W65C02 */
    /* Only for JMP, result is an address */
    a = cpu_next_address(c) + c->x;
    a = cpu_read_address(c, a);
    break;
  case A_IMP:
  default:
    /* implied */
    /* No address needed */
    break;
  }

  /** perform instruction */
  switch (instruction) {
  case I_ADC:
    /* add with carry */
    temp = c->a;
    c->a += b;
    if (c->a < temp) {
      _set_bit(c, CARRY_FLAG);
      _set_bit(c, OVERFLOW_FLAG);
    } else {
      _clear_bit(c, CARRY_FLAG);
      _clear_bit(c, OVERFLOW_FLAG);
    }
    _set_zero_flag(c, c->a);
    _set_negative_flag(c, c->a);
    break;
  case I_AND:
    /* and */
    c->a = c->a & b;
    _set_zero_flag(c, c->a);
    _set_negative_flag(c, c->a);
    break;
  case I_ASL:
    /* arithmetic shift left */
    if (c->a & (1<<7)) {
      _set_bit(c, CARRY_FLAG);
    } else {
      _clear_bit(c, CARRY_FLAG);
    }
    c->a = b << 1;
    _set_zero_flag(c, c->a);
    _set_negative_flag(c, c->a);
    break;
  case I_BCC:
    /* branch on carry clear */
    if (!_check_bit(c, CARRY_FLAG)) {
      c->pc = a;
    }
    break;
  case I_BCS:
    /* branch on carry set */
    if (_check_bit(c, CARRY_FLAG)) {
      c->pc = a;
    }
    break;
  case I_BEQ:
    /* branch on equal (zero) */
    if (_check_bit(c, ZERO_FLAG)) {
      c->pc = a;
    }
    break;
  case I_BIT:
    if (b & (1<<7)) {
      _set_bit(c, 7);
    } else {
      _clear_bit(c, 7);
    }

    if (b & (1<<6)) {
      _set_bit(c, 6);
    } else {
      _clear_bit(c, 6);
    }
      
    temp = c->a & b;
    _set_zero_flag(c, temp);
    break;
  case I_BMI:
    /* branch on negative */
    if (_check_bit(c, NEGATIVE_FLAG)) {
      c->pc = a;
    }
    break;
  case I_BNE:
    /* branch on not equal (not zero) */
    if (!_check_bit(c, ZERO_FLAG)) {
      c->pc = a;
    }
    break;
  case I_BPL:
    /* branch on positive (not negative) */
    if (!_check_bit(c, NEGATIVE_FLAG)) {
      c->pc = a;
    }
    break;
  case I_BRK:
    /* break */
    /** TODO: Replace with IRQ */
    cpu_halt(c);
    break;
  case I_BVC:
    /* branch on overflow clear */
    if (!_check_bit(c, OVERFLOW_FLAG)) {
      c->pc = a;
    }
    break;
  case I_BVS:
    /* branch on overflow set */
    if (_check_bit(c, OVERFLOW_FLAG)) {
      c->pc = a;
    }
    break;
  case I_CLC:
    /* clear carry */
    _clear_bit(c, CARRY_FLAG);
    break;
  case I_CLD:
    /* clear decimal */
    _clear_bit(c, BCD_FLAG);
    break;
  case I_CLI:
    /* clear interrupt disable */
    _clear_bit(c, IRQ_DISABLE);
    break;
  case I_CLV:
    /* clear overflow */
    _clear_bit(c, OVERFLOW_FLAG);
    break;
  case I_CMP:
    /* compare memory to A */
    temp = c->a;
    temp = temp - b;
    if (temp > c->a) {
      _set_bit(c, CARRY_FLAG);
    } else {
      _clear_bit(c, CARRY_FLAG);
    }
    _set_zero_flag(c, temp);
    _set_negative_flag(c, temp);
    break;
  case I_CPX:
    /* compare memory to X */
    temp = c->x;
    temp = temp - b;
    if (temp > c->x) {
      _set_bit(c, CARRY_FLAG);
    } else {
      _clear_bit(c, CARRY_FLAG);
    }
    _set_zero_flag(c, temp);
    _set_negative_flag(c, temp);
    break;
  case I_CPY:
    /* compare memory to Y */
    temp = c->y;
    temp = temp - b;
    if (temp > c->y) {
      _set_bit(c, CARRY_FLAG);
    } else {
      _clear_bit(c, CARRY_FLAG);
    }
    _set_zero_flag(c, temp);
    _set_negative_flag(c, temp);
    break;
  case I_DEC:
    /* decrement memory */
    b--;
    cpu_write_byte(c, a, b);
    _set_zero_flag(c, b);
    _set_negative_flag(c, b);
    break;
  case I_DEX:
    /* decrement x */
    c->x--;
    _set_zero_flag(c, c->x);
    _set_negative_flag(c, c->x);
    break;
  case I_DEY:
    /* decrement y */
    c->y--;
    _set_zero_flag(c, c->y);
    _set_negative_flag(c, c->y);
    break;
  case I_EOR:
    /* exclusive or */
    c->a = c->a ^ b;
    _set_zero_flag(c, c->a);
    _set_negative_flag(c, c->a);
    break;
  case I_INC:
    /* increment memory */
    b++;
    cpu_write_byte(c, a, b);
    _set_zero_flag(c, b);
    _set_negative_flag(c, b);
    break;
  case I_INX:
    /* increment x */
    c->x++;
    _set_zero_flag(c, c->x);
    _set_negative_flag(c, c->x);
    break;
  case I_INY:
    /* increment y */
    c->y++;
    _set_zero_flag(c, c->y);
    _set_negative_flag(c, c->y);
    break;
  case I_JMP:
    /* jump to address */
    c->pc = a;
    break;
  case I_JSR:
    /* jump to subroutine */
    lo = (byte) c->pc;
    hi = (byte) c->pc >> 8;
    _push(c, lo);
    _push(c, hi);
    c->pc = a;
    break;
  case I_LDA:
    /* load A */
    c->a = b;
    _set_zero_flag(c, c->a);
    _set_negative_flag(c, c->a);
    break;
  case I_LDX:
    /* load X */
    c->x = b;
    _set_zero_flag(c, c->x);
    _set_negative_flag(c, c->x);
    break;
  case I_LDY:
    /* load Y */
    c->y = b;
    _set_zero_flag(c, c->y);
    _set_negative_flag(c, c->y);
    break;
  case I_LSR:
    /* shift one bit right */
    if (c->a & 1) {
      _set_bit(c, CARRY_FLAG);
    } else {
      _clear_bit(c, CARRY_FLAG);
    }
    c->a = b >> 1;
    _set_zero_flag(c, c->a);
    _set_negative_flag(c, c->a);
    break;
  case I_ORA:
    /* or */
    c->a = c->a | b;
    _set_zero_flag(c, c->a);
    _set_negative_flag(c, c->a);
    break;  
  case I_PHA:
    /* push A */
    _push(c, c->a);
    break;
  case I_PHP:
    /* push processor status */
    /* push SR with break flag and bit 5 set */
    b = c->sr;
    /* set break flag */
    b = b | (1<<BREAK_FLAG);
    /* set bit 5 */
    b = b | (1<<5);
    _push(c, b);
    break;
  case I_PLA:
    /* pull A */
    c->a = _pop(c);
    break;
  case I_PLP:
    /* pull processor status from stack */
    /* the break flag and bit 5 should be ignored */
    b = c->sr;
    c->sr = _pop(c);
    if (b & (1<<BREAK_FLAG)) {
      _set_bit(c, BREAK_FLAG);
    } else {
      _clear_bit(c, BREAK_FLAG);
    }
    if (b & (1<<5)) {
      _set_bit(c, 5);
    } else {
      _clear_bit(c, 5);
    }
    break;
  case I_ROL:
    /* rotate one bit left */
    temp = b & (1<<7);
    b = b << 1;
    if (temp) {
      b = b | 1;
      _set_bit(c, CARRY_FLAG);
    } else {
      _clear_bit(c, CARRY_FLAG);
    }
    _set_zero_flag(c, b);
    _set_negative_flag(c, b);
    break;
  case I_ROR:
    /* rotate one bit right */
    temp = b & 1;
    b = b >> 1;
    if (temp) {
      b = b | (1<<7);
      _set_bit(c, CARRY_FLAG);
    } else {
      _clear_bit(c, CARRY_FLAG);
    }
    _set_zero_flag(c, b);
    _set_negative_flag(c, b);
    break;
  case I_RTI:
    /* return from interrupt */
    /** TODO */
    break;
  case I_RTS:
    /* return from subroutine */
    hi = _pop(c);
    lo = _pop(c);
    c->pc = (hi << 8) | lo;
    break;
  case I_SBC:
    /* subtract memory from A with borrow */
    temp = c->a;
    c->a = c->a - b;
    if (_check_bit(c, CARRY_FLAG)) {
      c->a--;
    }
    if (c->a > temp) {
      _set_bit(c, CARRY_FLAG);
      _set_bit(c, OVERFLOW_FLAG);
    } else {
      _clear_bit(c, CARRY_FLAG);
      _clear_bit(c, OVERFLOW_FLAG);
    }
    _set_zero_flag(c, c->a);
    _set_negative_flag(c, c->a);
    break;
  case I_SEC:
    /* set carry flag */
    _set_bit(c, CARRY_FLAG);
    break;
  case I_SED:
    /* set decimal flag */
    _set_bit(c, BCD_FLAG);
    break;
  case I_SEI:
    /* set interrupt disable flag */
    _set_bit(c, IRQ_DISABLE);
    break;
  case I_STA:
    /* store A */
    cpu_write_byte(c, a, c->a);
    break;
  case I_STX:
    /* store X */
    cpu_write_byte(c, a, c->x);
    break;
  case I_STY:
    /* store Y */
    cpu_write_byte(c, a, c->y);
    break;
  case I_TAX:
    /* transfer A to X */
    c->x = c->a;
    _set_zero_flag(c, c->x);
    _set_negative_flag(c, c->x);
    break;
  case I_TAY:
    /* transfer A to Y */
    c->y = c->a;
    _set_zero_flag(c, c->y);
    _set_negative_flag(c, c->y);
    break;
  case I_TSX:
    /* transfer SP to X */
    c->x = c->sp;
    _set_zero_flag(c, c->x);
    _set_negative_flag(c, c->x);
    break;
  case I_TXA:
    /* transfer X to A */
    c->a = c->x;
    _set_zero_flag(c, c->a);
    _set_negative_flag(c, c->a);
    break;
  case I_TXS:
    /* transfer X to SP */
    c->sp = c->x;
    break;
  case I_TYA:
    /* transfer Y to A */
    c->a = c->y;
    _set_zero_flag(c, c->a);
    _set_negative_flag(c, c->a);
    break;
  case I_NOP:
  default:
    /* Do nothing */
    break;
  }

  /** Handle Interrupts */
  if (c->nmi) {
    c->nmi = FALSE;
  } else if (c->irq) {
    c->irq = FALSE;
  }
  
}

void cpu_run(cpu *c) {
  if (c == NULL) return;
  while (!c->halted) {
    cpu_step(c);
    if (c->tick != NULL) {
      c->tick();
    }
  }
}

/** Halt the CPU. */
void cpu_halt(cpu *c) {
  if (c == NULL) return;
  c->halted = TRUE;
}

/** Trigger a CPU interrupt request (IRQ). */
void cpu_irq(cpu *c) {
  if (c == NULL) return;
  c->irq = TRUE;
}

/** Trigger a CPU reset. */
void cpu_reset(cpu *c) {
  if (c == NULL) return;
  c->reset = TRUE;
}  

/** Trigger a CPU reset. */
void cpu_nmi(cpu *c) {
  if (c == NULL) return;
  c->nmi = TRUE;
}  
