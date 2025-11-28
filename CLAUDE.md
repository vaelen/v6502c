# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
make              # Build both v6502c and hello example
make v6502c       # Build interactive emulator only
make hello        # Build hello world example only
make clean        # Remove binaries and object files
```

Compiler: clang-18 with `-ansi -Wpedantic` flags (strict ANSI C compliance required).

Assembler: vasm6502 (oldstyle) for assembling .s files to binary or Wozmon format.

## Running

```bash
./bin/v6502c              # Interactive emulator with Wozmon-compatible debugger
./bin/hello               # Run embedded hello world example
```

## Architecture

### Core Files

- `src/v6502.h` - Public API: CPU struct, type definitions (byte, address), function pointer types (ReadFn, WriteFn, TickFn)
- `src/v6502.c` - CPU implementation: instruction execution, addressing modes, stack operations
- `src/inst.h` - Lookup tables mapping opcodes to instructions and addressing modes

### Embedding Interface

The emulator uses callback functions for memory access:
```c
cpu c;
cpu_init(&c);
c.read = my_read_fn;    // byte read(address)
c.write = my_write_fn;  // void write(address, byte)
c.tick = my_tick_fn;    // void tick(void) - optional, called each cycle
cpu_reset(&c);
cpu_run(&c);            // runs until BRK or cpu_halt()
```

### Interactive Emulator

- `src/main.c` - Wozmon-compatible debugger with register/memory inspection
- Uses address `0xFF00` as character I/O device (getchar on read, putchar on write)
- SIGINT handler allows graceful interruption of running programs
- **CPU variant selection** - Switch between 6502 and 65C02 BCD behavior with `CPU [6502|65C02]` command

### Test Suite

- `src/test.c` - Comprehensive 6502 instruction test framework
- **22 test categories** covering arithmetic, logic, memory, stack, and CPU variant operations  
- Color-coded pass/fail output with detailed failure descriptions
- **BCD mode validation** - Tests both addition and subtraction in decimal mode
- **CPU variant testing** - Verifies different overflow flag behavior between 6502 and 65C02

## Features

### ✅ **Implemented:**
- **Complete 6502 instruction set** - All basic instructions work correctly
- **BCD Mode** - Full Binary Coded Decimal support for ADC/SBC operations
- **CPU Variants** - Support for both original 6502 and 65C02 BCD flag behavior
- **Comprehensive test suite** - 22 test cases covering all instruction types

### **BCD Mode Implementation:**
- **ADC BCD**: Proper decimal adjustment with nibble overflow handling
- **SBC BCD**: Correct decimal borrow with +10 adjustment algorithm  
- **Flag behavior**: N/Z flags reflect binary result (authentic 6502 behavior)
- **CPU variants**: 6502 clears V flag in BCD, 65C02 maintains overflow detection

## Incomplete Features

1. **Interrupts** - IRQ/NMI flags exist but vectoring not implemented
2. **Some 65C02 opcodes** - BBR, BBS, RMB, SMB, TSB, TRB defined in tables but not in execution switch

## Code Style

- ANSI C only (no C99+ features)
- Minimize standard library dependencies for portability to classic systems
- Not cycle-accurate by design
