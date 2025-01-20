#ifndef _INST_H_
#define _INST_H_

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

/**
   See: https://www.masswerk.at/6502/6502_instruction_set.html
*/

enum instruction_t {
  I_ADC, I_AND, I_ASL, I_BCC, I_BCS, I_BEQ, I_BIT, I_BMI, I_BNE, I_BPL,
  I_BRK, I_BVC, I_BVS, I_CLC, I_CLD, I_CLI, I_CLV, I_CMP, I_CPX, I_CPY,
  I_DEC, I_DEX, I_DEY, I_EOR, I_INC, I_INX, I_INY, I_JMP, I_JSR, I_LDA,
  I_LDX, I_LDY, I_LSR, I_NOP, I_ORA, I_PHA, I_PHP, I_PLA, I_PLP, I_ROL,
  I_ROR, I_RTI, I_RTS, I_SBC, I_SEC, I_SED, I_SEI, I_STA, I_STX, I_STY,
  I_TAX, I_TAY, I_TSX, I_TXA, I_TXS, I_TYA,
  /* 65C02 extended instructions */
  I_BBR0, I_BBR1, I_BBR2, I_BBR3, I_BBR4, I_BBR5, I_BBR6, I_BBR7,
  I_BBS0, I_BBS1, I_BBS2, I_BBS3, I_BBS4, I_BBS5, I_BBS6, I_BBS7,
  I_BRA, I_PHX, I_PHY, I_PLX, I_PLY,
  I_RMB0, I_RMB1, I_RMB2, I_RMB3, I_RMB4, I_RMB5, I_RMB6, I_RMB7,
  I_SMB0, I_SMB1, I_SMB2, I_SMB3, I_SMB4, I_SMB5, I_SMB6, I_SMB7,
  I_STP, I_STZ, I_TRB, I_TSB, I_WAI 
};

enum addressing_t {
  A_ACC, A_ABS, A_ABX, A_ABY, A_IMM, A_IMP, A_IND, A_INX, A_INY,
  A_REL, A_ZPG, A_ZPX, A_ZPY,
  /* 65C02 extended addressing modes */
  A_ZPI, A_ABI 
};

enum instruction_t instructions[] = {
  /* 00     01     02     03     04     05     06     07 */ 
  I_BRK, I_ORA, I_NOP, I_NOP, I_TSB, I_ORA, I_ASL, I_RMB0,
  /* 08     09     0A     0B     0C     0D     0E     0F */ 
  I_PHP, I_ORA, I_ASL, I_NOP, I_TSB, I_ORA, I_ASL, I_BBR0,
  /* 10     11     12     13     14     15     16     17 */ 
  I_BPL, I_ORA, I_ORA, I_NOP, I_TRB, I_ORA, I_ASL, I_RMB1,
  /* 18     19     1A     1B     1C     1D     1E     1F */ 
  I_CLC, I_ORA, I_INC, I_TRB, I_TRB, I_ORA, I_ASL, I_BBR1,
  /* 20     21     22     23     24     25     26     27 */ 
  I_JSR, I_AND, I_NOP, I_NOP, I_BIT, I_AND, I_ROL, I_RMB2,
  /* 28     29     2A     2B     2C     2D     2E     2F */ 
  I_PHP, I_AND, I_ROL, I_NOP, I_BIT, I_AND, I_ROL, I_BBR2,
  /* 30     31     32     33     34     35     36     37 */ 
  I_BMI, I_AND, I_AND, I_NOP, I_BIT, I_AND, I_ROL, I_RMB3,
  /* 38     39     3A     3B     3C     3D     3E     3F */ 
  I_SEC, I_AND, I_DEC, I_NOP, I_BIT, I_AND, I_ROL, I_BBR3,
  /* 40     41     42     43     44     45     46     47 */ 
  I_RTI, I_EOR, I_NOP, I_NOP, I_NOP, I_EOR, I_LSR, I_RMB4,
  /* 48     49     4A     4B     4C     4D     4E     4F */ 
  I_PHA, I_EOR, I_LSR, I_NOP, I_JMP, I_EOR, I_LSR, I_BBR4,
  /* 50     51     52     53     54     55     56     57 */ 
  I_BVC, I_EOR, I_EOR, I_NOP, I_NOP, I_EOR, I_LSR, I_RMB5,
  /* 58     59     5A     5B     5C     5D     5E     5F */ 
  I_CLI, I_EOR, I_PHY, I_NOP, I_NOP, I_EOR, I_LSR, I_BBR5,
  /* 60     61     62     63     64     65     66     67 */ 
  I_RTS, I_ADC, I_NOP, I_NOP, I_STZ, I_ADC, I_ROR, I_RMB6,
  /* 68     69     6A     6B     6C     6D     6E     6F */ 
  I_PLA, I_ADC, I_ROR, I_NOP, I_JMP, I_ADC, I_ROR, I_BBR6,
  /* 70     71     72     73     74     75     76     77 */ 
  I_BVS, I_ADC, I_ADC, I_NOP, I_STZ, I_ADC, I_ROR, I_RMB7,
  /* 78     79     7A     7B     7C     7D     7E     7F */ 
  I_SEI, I_ADC, I_PLY, I_NOP, I_JMP, I_ADC, I_ROR, I_BBR7,
  /* 80     81     82     83     84     85     86     87 */ 
  I_BRA, I_STA, I_NOP, I_NOP, I_STY, I_STA, I_STX, I_SMB0,
  /* 88     89     8A     8B     8C     8D     8E     8F */ 
  I_DEY, I_BIT, I_TXA, I_NOP, I_STY, I_STA, I_STX, I_BBS0,
  /* 90     91     92     93     94     95     96     97 */ 
  I_BCC, I_STA, I_STA, I_NOP, I_STY, I_STA, I_STX, I_SMB1,
  /* 98     99     9A     9B     9C     9D     9E     9F */ 
  I_TYA, I_STA, I_TXS, I_NOP, I_STZ, I_STA, I_STZ, I_BBS1,
  /* A0     A1     A2     A3     A4     A5     A6     A7 */ 
  I_LDY, I_LDA, I_LDX, I_NOP, I_LDY, I_LDA, I_LDX, I_SMB2,
  /* A8     A9     AA     AB     AC     AD     AE     AF */ 
  I_TAY, I_LDA, I_TAX, I_NOP, I_LDY, I_LDA, I_LDX, I_BBS2,
  /* B0     B1     B2     B3     B4     B5     B6     B7 */ 
  I_BCS, I_LDA, I_LDA, I_NOP, I_LDY, I_LDA, I_LDX, I_SMB3,
  /* B8     B9     BA     BB     BC     BD     BE     BF */ 
  I_CLV, I_LDA, I_TSX, I_NOP, I_LDY, I_LDA, I_LDX, I_BBS3,
  /* C0     C1     C2     C3     C4     C5     C6     C7 */ 
  I_CPY, I_CMP, I_NOP, I_NOP, I_CPY, I_CMP, I_DEC, I_SMB4,
  /* C8     C9     CA     CB     CC     CD     CE     CF */ 
  I_INY, I_CMP, I_DEX, I_WAI, I_CPY, I_CMP, I_DEC, I_BBS4,
  /* D0     D1     D2     D3     D4     D5     D6     D7 */ 
  I_BNE, I_CMP, I_CMP, I_NOP, I_NOP, I_CMP, I_DEC, I_SMB5,
  /* D8     D9     DA     DB     DC     DD     DE     DF */ 
  I_CLD, I_CMP, I_PHX, I_STP, I_NOP, I_CMP, I_DEC, I_BBS5,
  /* E0     E1     E2     E3     E4     E5     E6     E7 */ 
  I_CPX, I_SBC, I_NOP, I_NOP, I_CPX, I_SBC, I_INC, I_SMB6,
  /* E8     E9     EA     EB     EC     ED     EE     EF */ 
  I_INX, I_SBC, I_NOP, I_NOP, I_CPX, I_SBC, I_INC, I_BBS6,
  /* F0     F1     F2     F3     F4     F5     F6     F7 */ 
  I_BEQ, I_SBC, I_SBC, I_NOP, I_NOP, I_SBC, I_INC, I_SMB7,
  /* F8     F9     FA     FB     FC     FD     FE     FF */ 
  I_SED, I_SBC, I_PLX, I_NOP, I_NOP, I_SBC, I_INC, I_BBS7
};

enum addressing_t addressings[] = {
  /* 00     01     02     03     04     05     06     07 */ 
  A_IMP, A_INX, A_IMP, A_IMP, A_ZPG, A_ZPG, A_ZPG, A_ZPG,
  /* 08     09     0A     0B     0C     0D     0E     0F */ 
  A_IMP, A_IMM, A_ACC, A_IMP, A_ABS, A_ABS, A_ABS, A_REL,
  /* 10     11     12     13     14     15     16     17 */ 
  A_REL, A_INY, A_ZPI, A_IMP, A_ZPG, A_ZPX, A_ZPX, A_ZPG,
  /* 18     19     1A     1B     1C     1D     1E     1F */ 
  A_IMP, A_ABY, A_ACC, A_IMP, A_ABS, A_ABX, A_ABX, A_REL,
  /* 20     21     22     23     24     25     26     27 */ 
  A_ABS, A_INX, A_IMP, A_IMP, A_ZPG, A_ZPG, A_ZPG, A_ZPG,
  /* 28     29     2A     2B     2C     2D     2E     2F */ 
  A_IMP, A_IMM, A_ACC, A_IMP, A_ABS, A_ABS, A_ABS, A_REL,
  /* 30     31     32     33     34     35     36     37 */ 
  A_REL, A_INY, A_ZPI, A_IMP, A_ZPX, A_ZPX, A_ZPX, A_ZPG,
  /* 38     39     3A     3B     3C     3D     3E     3F */ 
  A_IMP, A_ABY, A_ACC, A_IMP, A_ABX, A_ABX, A_ABX, A_REL,
  /* 40     41     42     43     44     45     46     47 */ 
  A_IMP, A_INX, A_IMP, A_IMP, A_IMP, A_ZPG, A_ZPG, A_ZPG,
  /* 48     49     4A     4B     4C     4D     4E     4F */ 
  A_IMP, A_IMM, A_ACC, A_IMP, A_ABS, A_ABS, A_ABS, A_REL,
  /* 50     51     52     53     54     55     56     57 */ 
  A_REL, A_INY, A_ZPI, A_IMP, A_IMP, A_ZPX, A_ZPX, A_ZPG,
  /* 58     59     5A     5B     5C     5D     5E     5F */ 
  A_IMP, A_ABY, A_IMP, A_IMP, A_IMP, A_ABX, A_ABX, A_REL,
  /* 60     61     62     63     64     65     66     67 */ 
  A_IMP, A_INX, A_IMP, A_IMP, A_ZPG, A_ZPG, A_ZPG, A_ZPG,
  /* 68     69     6A     6B     6C     6D     6E     6F */ 
  A_IMP, A_IMM, A_ACC, A_IMP, A_IND, A_ABS, A_ABS, A_REL,
  /* 70     71     72     73     74     75     76     77 */ 
  A_REL, A_INY, A_ZPI, A_IMP, A_ZPX, A_ZPX, A_ZPX, A_ZPG,
  /* 78     79     7A     7B     7C     7D     7E     7F */ 
  A_IMP, A_ABY, A_IMP, A_IMP, A_ABX, A_ABX, A_ABX, A_REL,
  /* 80     81     82     83     84     85     86     87 */ 
  A_REL, A_INX, A_IMP, A_IMP, A_ZPG, A_ZPG, A_ZPG, A_ZPG,
  /* 88     89     8A     8B     8C     8D     8E     8F */ 
  A_IMP, A_IMM, A_IMP, A_IMP, A_ABS, A_ABS, A_ABS, A_REL,
  /* 90     91     92     93     94     95     96     97 */ 
  A_REL, A_INY, A_ZPI, A_IMP, A_ZPX, A_ZPX, A_ZPY, A_ZPG,
  /* 98     99     9A     9B     9C     9D     9E     9F */ 
  A_IMP, A_ABY, A_IMP, A_IMP, A_ABS, A_ABX, A_ABX, A_REL,
  /* A0     A1     A2     A3     A4     A5     A6     A7 */ 
  A_IMM, A_INX, A_IMM, A_IMP, A_ZPG, A_ZPG, A_ZPG, A_ZPG,
  /* A8     A9     AA     AB     AC     AD     AE     AF */ 
  A_IMP, A_IMM, A_IMP, A_IMP, A_ABS, A_ABS, A_ABS, A_REL,
  /* B0     B1     B2     B3     B4     B5     B6     B7 */ 
  A_REL, A_INY, A_ZPI, A_IMP, A_ZPX, A_ZPX, A_ZPY, A_ZPG,
  /* B8     B9     BA     BB     BC     BD     BE     BF */ 
  A_IMP, A_ABY, A_IMP, A_IMP, A_ABX, A_ABX, A_ABX, A_REL,
  /* C0     C1     C2     C3     C4     C5     C6     C7 */ 
  A_IMM, A_INX, A_IMP, A_IMP, A_ZPG, A_ZPG, A_ZPG, A_ZPG,
  /* C8     C9     CA     CB     CC     CD     CE     CF */ 
  A_IMP, A_IMM, A_IMP, A_IMP, A_ABS, A_ABS, A_ABS, A_REL,
  /* D0     D1     D2     D3     D4     D5     D6     D7 */ 
  A_REL, A_INY, A_ZPI, A_IMP, A_IMP, A_ZPX, A_ZPX, A_ZPG,
  /* D8     D9     DA     DB     DC     DD     DE     DF */ 
  A_IMP, A_ABY, A_IMP, A_IMP, A_IMP, A_ABX, A_ABX, A_REL,
  /* E0     E1     E2     E3     E4     E5     E6     E7 */ 
  A_IMM, A_INX, A_IMP, A_IMP, A_ZPG, A_ZPG, A_ZPG, A_ZPG,
  /* E8     E9     EA     EB     EC     ED     EE     EF */ 
  A_IMP, A_IMM, A_IMP, A_IMP, A_ABS, A_ABS, A_ABS, A_REL,
  /* F0     F1     F2     F3     F4     F5     F6     F7 */ 
  A_REL, A_INY, A_ZPI, A_IMP, A_IMP, A_ZPX, A_ZPX, A_ZPG,
  /* F8     F9     FA     FB     FC     FD     FE     FF */ 
  A_IMP, A_ABY, A_IMP, A_IMP, A_IMP, A_ABX, A_ABX, A_REL
};

#endif
