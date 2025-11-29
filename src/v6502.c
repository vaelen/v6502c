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

bool V6502C_TRACE = FALSE;
bool V6502C_VERBOSE = FALSE;

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
  cpu_write_byte(c, 0x0100 + c->sp, value);
  c->sp--;
}

/** Helper method to pop a value off the stack. */
byte _pop(cpu *c) {
  c->sp++;
  return cpu_read_byte(c, 0x0100 + c->sp);
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

/**
 * Helper method to service an interrupt.
 * @param c - CPU pointer
 * @param vector - The vector address to fetch the handler from
 * @param is_brk - TRUE if this is a BRK instruction (sets BREAK_FLAG in pushed SR)
 */
void _service_interrupt(cpu *c, address vector, bool is_brk) {
  byte pushed_sr;
  byte lo, hi;

  /* Push PC (high byte first, then low - authentic 6502 order) */
  hi = (byte)(c->pc >> 8);
  lo = (byte)(c->pc & 0xFF);
  _push(c, hi);
  _push(c, lo);

  /* Push SR with bit 5 always set, BREAK_FLAG set only for BRK */
  pushed_sr = c->sr | (1 << 5);
  if (is_brk) {
    pushed_sr |= (1 << BREAK_FLAG);
  } else {
    pushed_sr &= ~(1 << BREAK_FLAG);
  }
  _push(c, pushed_sr);

  /* Set IRQ_DISABLE flag */
  _set_bit(c, IRQ_DISABLE);

  /* Load PC from vector */
  c->pc = cpu_read_address(c, vector);
}

void cpu_init(cpu *c) {
  if (c == NULL) return; 
  c->write = NULL;
  c->read = NULL;
  c->tick = NULL;
  c->variant = CPU_65C02;  /* Default to 65C02 */
  _reset(c);
}

void cpu_set_variant(cpu *c, enum cpu_variant_t variant) {
  if (c == NULL) return;
  c->variant = variant;
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
    /*
     * Note: The NMOS 6502 has a bug where JMP ($xxFF) wraps within the
     * same page when reading the high byte (e.g., JMP ($10FF) reads the
     * low byte from $10FF but the high byte from $1000, not $1100).
     * This implementation uses 65C02 behavior which correctly crosses
     * page boundaries. This is intentional as few programs rely on
     * the bug and the correct behavior is more useful.
     */
    a = cpu_next_address(c);
    a = cpu_read_address(c, a);
    break;
  case A_INX:
    /* pre-indexed indirect - wraps within zero page */
    a = ((address) cpu_next_byte(c) + c->x) & 0xFF;
    /* Read pointer from zero page (may wrap at page boundary) */
    lo = cpu_read_byte(c, a);
    hi = cpu_read_byte(c, (a + 1) & 0xFF);
    a = (hi << 8) | lo;
    if (!_is_store(instruction)) {
      b = cpu_read_byte(c, a);
    }
    break;
  case A_INY:
    /* post-indexed indirect - pointer wraps within zero page */
    a = (address) cpu_next_byte(c);
    /* Read pointer from zero page (may wrap at page boundary) */
    lo = cpu_read_byte(c, a);
    hi = cpu_read_byte(c, (a + 1) & 0xFF);
    a = ((hi << 8) | lo) + c->y;
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
    /* zero-page x-indexed - wraps within zero page */
    a = ((address) cpu_next_byte(c) + c->x) & 0xFF;
    if (!_is_store(instruction)) {
      b = cpu_read_byte(c, a);
    }
    break;
  case A_ZPY:
    /* zero-page y-indexed - wraps within zero page */
    a = ((address) cpu_next_byte(c) + c->y) & 0xFF;
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
    {
      int carry_in = _check_bit(c, CARRY_FLAG) ? 1 : 0;
      
      if (_check_bit(c, BCD_FLAG)) {
        /* BCD (Decimal) Mode */
        byte original_a = c->a;
        int lo_nibble = (c->a & 0x0F) + (b & 0x0F) + carry_in;
        int hi_nibble = (c->a >> 4) + (b >> 4);
        int binary_result = c->a + b + carry_in;
        
        /* Adjust low nibble if > 9 */
        if (lo_nibble > 9) {
          lo_nibble += 6;    /* Add 6 to convert to BCD */
          hi_nibble++;       /* Carry to high nibble */
        }
        
        /* Adjust high nibble if > 9 */
        if (hi_nibble > 9) {
          hi_nibble += 6;    /* Add 6 to convert to BCD */
          _set_bit(c, CARRY_FLAG);  /* Set carry out */
        } else {
          _clear_bit(c, CARRY_FLAG);
        }
        
        c->a = ((hi_nibble & 0x0F) << 4) | (lo_nibble & 0x0F);
        
        /* In BCD mode, N and Z flags reflect the binary result on 6502 */
        _set_zero_flag(c, binary_result & 0xFF);
        _set_negative_flag(c, binary_result);
        
        /* Overflow flag behavior differs between CPU variants */
        if (c->variant == CPU_65C02) {
          /* 65C02: V flag reflects signed overflow like in binary mode */
          if (((original_a ^ binary_result) & (b ^ binary_result) & 0x80) != 0) {
            _set_bit(c, OVERFLOW_FLAG);
          } else {
            _clear_bit(c, OVERFLOW_FLAG);
          }
        } else {
          /* Original 6502: V flag undefined in BCD mode */
          _clear_bit(c, OVERFLOW_FLAG);
        }
        
      } else {
        /* Binary Mode */
        int result = c->a + b + carry_in;
        
        /* Set carry flag if result > 255 (unsigned overflow) */
        if (result > 0xFF) {
          _set_bit(c, CARRY_FLAG);
        } else {
          _clear_bit(c, CARRY_FLAG);
        }
        
        /* Set overflow flag if signed overflow occurred */
        /* Overflow happens when: (+) + (+) = (-) or (-) + (-) = (+) */
        if (((c->a ^ result) & (b ^ result) & 0x80) != 0) {
          _set_bit(c, OVERFLOW_FLAG);
        } else {
          _clear_bit(c, OVERFLOW_FLAG);
        }
        
        c->a = result & 0xFF;
        _set_zero_flag(c, c->a);
        _set_negative_flag(c, c->a);
      }
    }
    break;
  case I_AND:
    /* and */
    c->a = c->a & b;
    _set_zero_flag(c, c->a);
    _set_negative_flag(c, c->a);
    break;
  case I_ASL:
    /* arithmetic shift left */
    if (b & (1<<7)) {
      _set_bit(c, CARRY_FLAG);
    } else {
      _clear_bit(c, CARRY_FLAG);
    }
    b = b << 1;
    if (addressing == A_ACC) {
      c->a = b;
    } else {
      cpu_write_byte(c, a, b);
    }
    _set_zero_flag(c, b);
    _set_negative_flag(c, b);
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
    /* software interrupt */
    c->pc++;  /* Skip padding byte after BRK opcode */
    _service_interrupt(c, IRQ_VECTOR, TRUE);
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
    temp = c->a - b;
    
    /* Set carry if A >= operand (no borrow needed) */
    if (c->a >= b) {
      _set_bit(c, CARRY_FLAG);
    } else {
      _clear_bit(c, CARRY_FLAG);
    }
    
    _set_zero_flag(c, temp);
    _set_negative_flag(c, temp);
    break;
  case I_CPX:
    /* compare memory to X */
    temp = c->x - b;
    
    /* Set carry if X >= operand (no borrow needed) */
    if (c->x >= b) {
      _set_bit(c, CARRY_FLAG);
    } else {
      _clear_bit(c, CARRY_FLAG);
    }
    
    _set_zero_flag(c, temp);
    _set_negative_flag(c, temp);
    break;
  case I_CPY:
    /* compare memory to Y */
    temp = c->y - b;
    
    /* Set carry if Y >= operand (no borrow needed) */
    if (c->y >= b) {
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
    /* jump to subroutine - push return address minus 1 */
    c->pc--;
    hi = (byte)(c->pc >> 8);
    lo = (byte)(c->pc & 0xFF);
    _push(c, hi);
    _push(c, lo);
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
    if (b & 1) {
      _set_bit(c, CARRY_FLAG);
    } else {
      _clear_bit(c, CARRY_FLAG);
    }
    b = b >> 1;
    if (addressing == A_ACC) {
      c->a = b;
    } else {
      cpu_write_byte(c, a, b);
    }
    _set_zero_flag(c, b);
    _set_negative_flag(c, b);
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
    _set_zero_flag(c, c->a);
    _set_negative_flag(c, c->a);
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
    temp = b & (1<<7);        /* Save bit 7 */
    b = b << 1;               /* Shift left */
    if (_check_bit(c, CARRY_FLAG)) {  /* Rotate carry into bit 0 */
      b = b | 1;
    }

    /* Set new carry from old bit 7 */
    if (temp) {
      _set_bit(c, CARRY_FLAG);
    } else {
      _clear_bit(c, CARRY_FLAG);
    }

    if (addressing == A_ACC) {
      c->a = b;
    } else {
      cpu_write_byte(c, a, b);
    }
    _set_zero_flag(c, b);
    _set_negative_flag(c, b);
    break;
  case I_ROR:
    /* rotate one bit right */
    temp = b & 1;             /* Save bit 0 for new carry */
    b = b >> 1;               /* Shift right */
    if (_check_bit(c, CARRY_FLAG)) {  /* Rotate old carry into bit 7 */
      b = b | (1<<7);
    }

    /* Set new carry from old bit 0 */
    if (temp) {
      _set_bit(c, CARRY_FLAG);
    } else {
      _clear_bit(c, CARRY_FLAG);
    }

    if (addressing == A_ACC) {
      c->a = b;
    } else {
      cpu_write_byte(c, a, b);
    }
    _set_zero_flag(c, b);
    _set_negative_flag(c, b);
    break;
  case I_RTI:
    /* return from interrupt */
    
    /* pull SR minus bit 5 and break flag */
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

    /* pull pc */
    lo = _pop(c);
    hi = _pop(c);
    c->pc = (hi << 8) | lo;
    break;
  case I_RTS:
    /* return from subroutine - add 1 to popped address */
    lo = _pop(c);
    hi = _pop(c);
    c->pc = ((hi << 8) | lo) + 1;
    break;
  case I_SBC:
    /* subtract memory from A with borrow */
    {
      int borrow = 1 - (_check_bit(c, CARRY_FLAG) ? 1 : 0);
      
      if (_check_bit(c, BCD_FLAG)) {
        /* BCD (Decimal) Mode */
        byte original_a = c->a;
        int lo_nibble = (c->a & 0x0F) - (b & 0x0F) - borrow;
        int hi_nibble = (c->a >> 4) - (b >> 4);
        int binary_result = c->a - b - borrow;
        
        /* Adjust low nibble if < 0 (borrow from high nibble) */
        if (lo_nibble < 0) {
          lo_nibble += 10;   /* Add 10 for decimal borrow */
          hi_nibble--;       /* Borrow from high nibble */
        }
        
        /* Adjust high nibble if < 0 */
        if (hi_nibble < 0) {
          hi_nibble += 10;   /* Add 10 for decimal borrow */
          _clear_bit(c, CARRY_FLAG);  /* Set borrow flag */
        } else {
          _set_bit(c, CARRY_FLAG);    /* No borrow occurred */
        }
        
        c->a = ((hi_nibble & 0x0F) << 4) | (lo_nibble & 0x0F);
        
        /* In BCD mode, N and Z flags reflect the binary result on 6502 */
        _set_zero_flag(c, binary_result & 0xFF);
        _set_negative_flag(c, binary_result);
        
        /* Overflow flag behavior differs between CPU variants */
        if (c->variant == CPU_65C02) {
          /* 65C02: V flag reflects signed overflow like in binary mode */
          if (((original_a ^ b) & (original_a ^ binary_result) & 0x80) != 0) {
            _set_bit(c, OVERFLOW_FLAG);
          } else {
            _clear_bit(c, OVERFLOW_FLAG);
          }
        } else {
          /* Original 6502: V flag undefined in BCD mode */
          _clear_bit(c, OVERFLOW_FLAG);
        }
        
      } else {
        /* Binary Mode */
        int result = c->a - b - borrow;
        
        /* Set carry flag if NO borrow occurred (result >= 0) */
        if (result >= 0) {
          _set_bit(c, CARRY_FLAG);
        } else {
          _clear_bit(c, CARRY_FLAG);
        }
        
        /* Set overflow flag if signed overflow occurred */
        /* Overflow in subtraction: (+) - (-) = (-) or (-) - (+) = (+) */
        if (((c->a ^ b) & (c->a ^ result) & 0x80) != 0) {
          _set_bit(c, OVERFLOW_FLAG);
        } else {
          _clear_bit(c, OVERFLOW_FLAG);
        }
        
        c->a = result & 0xFF;
        _set_zero_flag(c, c->a);
        _set_negative_flag(c, c->a);
      }
    }
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

  /* Handle Interrupts - checked after each instruction */
  if (c->nmi) {
    c->nmi = FALSE;
    _service_interrupt(c, NMI_VECTOR, FALSE);
  } else if (c->irq && !_check_bit(c, IRQ_DISABLE)) {
    c->irq = FALSE;
    _service_interrupt(c, IRQ_VECTOR, FALSE);
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
