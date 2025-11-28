# v6502c Project Overview

## Introduction

v6502c is a portable 6502 CPU emulator written in ANSI C. It is a port of the author's v6502 Rust project, designed specifically for embedding in retro computing projects targeting systems like the classic Macintosh. The emulator prioritizes portability over cycle accuracy, making it suitable for educational purposes and running software like Microsoft BASIC or EhBASIC.

## Project Structure

```
v6502c/
├── Makefile              # Build configuration (clang, ANSI mode)
├── README.md             # User documentation
├── LICENSE               # MIT License
├── bin/                  # Compiled binaries
├── obj/                  # Object files
├── src/
│   ├── v6502.h           # Public API header
│   ├── v6502.c           # Core CPU implementation (~750 lines)
│   ├── inst.h            # Instruction/addressing mode lookup tables
│   ├── main.h            # Interactive emulator header
│   ├── main.c            # Wozmon-compatible debugger (~610 lines)
│   ├── hello.c           # Embedded emulator example
│   ├── hello.h           # Generated machine code for hello example
│   ├── hello.s           # Hello World assembly source
│   └── infloop.s         # Infinite loop test program
└── msbasic/              # Microsoft BASIC port (test suite)
```

## Architecture

### CPU Structure

The emulator models the 6502 CPU with the following state:

| Component | Size | Description |
|-----------|------|-------------|
| PC | 16-bit | Program Counter |
| A | 8-bit | Accumulator |
| X | 8-bit | X Index Register |
| Y | 8-bit | Y Index Register |
| SR | 8-bit | Status Register (flags) |
| SP | 8-bit | Stack Pointer |

### Memory Model

- 64KB address space (standard 6502)
- Custom read/write function pointers allow flexible memory mapping
- Special I/O handling at address `0xFF00` for character device emulation
- Standard vectors: Reset (`0xFFFC`), IRQ (`0xFFFE`), NMI (`0xFFFA`)

### Callback Interface

The emulator uses function pointers for hardware abstraction:
- `ReadFn` - Memory read callback
- `WriteFn` - Memory write callback
- `TickFn` - Called each cycle for timing control

## Implemented Features

### Addressing Modes (15 total)

Standard 6502:
- Implied, Accumulator, Immediate
- Zero-page, Zero-page X, Zero-page Y
- Absolute, Absolute X, Absolute Y
- Indirect (JMP only)
- Pre-indexed indirect (X), Post-indexed indirect (Y)
- Relative (branches)

65C02 extensions:
- Zero-page indirect, Absolute indexed indirect

### Instructions (~90 opcodes)

**Data Movement:** LDA, LDX, LDY, STA, STX, STY, TAX, TAY, TSX, TXA, TXS, TYA, PHA, PHP, PLA, PLP

**Arithmetic/Logic:** ADC, SBC, AND, ORA, EOR, ASL, LSR, ROL, ROR, INC, DEC, INX, INY, DEX, DEY, CMP, CPX, CPY, BIT

**Control Flow:** JMP, JSR, RTS, RTI, BRK, NOP, BEQ, BNE, BCS, BCC, BMI, BPL, BVS, BVC

**Status Flags:** CLC, CLD, CLI, CLV, SEC, SED, SEI

**65C02 Extensions:** BRA, STZ, PHX, PHY, PLX, PLY, STP, WAI (partial support)

### Interactive Debugger

The main program provides a Wozmon-compatible interface:
- Register inspection and modification
- Memory examination and editing
- Single-step execution
- Execution tracing with register change display
- Load/save programs in Wozmon format
- Signal handling for graceful interruption (Ctrl+C)

## Current Status

### Working

- Core instruction execution for standard 6502 opcodes
- All addressing modes
- Stack operations
- Branch instructions
- Arithmetic operations (non-BCD)
- Interactive debugger with Wozmon compatibility
- Embeddable design with callback interface
- Execution tracing for debugging
- Can run Microsoft BASIC

### Partially Working

- 65C02 extensions (lookup tables defined, some instructions not implemented)
- Interrupt flags (can be set, but vectors not serviced)

## What Remains To Be Done

### High Priority

1. **Interrupt Handling**
   - IRQ vectoring when `irq` flag is set
   - NMI vectoring when `nmi` flag is set
   - Proper status register handling during interrupt service
   - RTI instruction needs testing with actual interrupts

2. **Binary Coded Decimal (BCD) Mode**
   - ADC needs BCD addition when decimal flag is set
   - SBC needs BCD subtraction when decimal flag is set
   - Proper flag handling in decimal mode

### Medium Priority

3. **65C02 Instruction Completion**
   - BBR0-7 (Branch on Bit Reset)
   - BBS0-7 (Branch on Bit Set)
   - RMB0-7 (Reset Memory Bit)
   - SMB0-7 (Set Memory Bit)
   - TSB (Test and Set Bits)
   - TRB (Test and Reset Bits)

4. **Bug Fixes**
   - Review ASL/LSR memory addressing behavior
   - Verify compare instruction carry flag semantics
   - Consider implementing JMP indirect page-boundary bug for accuracy

### Low Priority

5. **Testing**
   - Comprehensive test suite for all instructions
   - Automated regression testing
   - Klaus Dormann's 6502 functional test

6. **Documentation**
   - API documentation for embedding
   - More example programs

## Design Decisions

### Portability Over Accuracy

The emulator is intentionally not cycle-accurate. All instructions execute in a single step rather than modeling individual cycles. This simplifies the implementation and improves performance at the cost of demo compatibility.

### Minimal Dependencies

The code uses ANSI C and minimal standard library functions to maximize portability to older and exotic systems.

### Callback-Based I/O

Rather than hardcoding memory layout, the emulator uses function pointers for all memory access. This allows host programs to implement:
- Memory-mapped I/O
- Bank switching
- ROM/RAM partitioning
- Hardware peripheral emulation

## Building

```bash
# Build everything
make

# Build just the emulator
make v6502c

# Build the hello example
make hello

# Clean build artifacts
make clean
```

Requirements:
- clang (or modify Makefile for gcc)
- vasm6502 (optional, for assembling .s files)

## Usage Examples

### Embedded Usage

```c
#include "v6502.h"

byte memory[0x10000];

byte read(address addr) { return memory[addr]; }
void write(address addr, byte val) { memory[addr] = val; }

int main() {
    cpu c;
    cpu_init(&c);
    c.read = read;
    c.write = write;

    // Load program into memory...
    // Set reset vector...

    cpu_reset(&c);
    cpu_run(&c);  // Run until BRK
    return 0;
}
```

### Interactive Session

```
$ ./bin/v6502c
v6502c v1.0

=> load hello.woz
=> ?
PC: 1000  A: 00  X: 00  Y: 00  SR: 36  SP: FD
=> g
Hello, World!
=> q
```

## License

MIT License - see LICENSE file for details.

## Author

Andrew C. Young <andrew@vaelen.org>
