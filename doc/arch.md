# IRRE Architecture Specification

This document defines the **IRRE** (Irrelevant Utility Architecture) v2.0 specification.

## Introduction

The Irrelevant Utility Architecture (**IRRE**) is a capable, general-purpose 32-bit architecture with a minimalist design. It features a clean instruction set designed for simplicity, efficiency, and ease of implementation.

IRRE is a little-endian, 32-bit architecture featuring 37 scalar registers and a deliberately minimal instruction set. The architecture prioritizes unambiguous instruction decoding, reduced implementation complexity, and extensibility.

## Registers

IRRE provides 37 32-bit registers to the programmer:

### General Purpose Registers (32 registers)
- **r0** through **r31**: General-purpose registers that can be used interchangeably
- Encoded as values `0x00` through `0x1f`

### Special Purpose Registers (5 registers)
- **pc** (`0x20`): Program counter
- **lr** (`0x21`): Link register (return address)
- **ad** (`0x22`): Assembler temporary 1
- **at** (`0x23`): Assembler temporary 2
- **sp** (`0x24`): Stack pointer

All registers are functionally identical for instruction operations. Writing to the program counter (`pc`) alters program flow.

## Instruction Encoding

Each IRRE instruction is exactly 32 bits (4 bytes) wide with the following layout:

```
Bits:  31-24  | 23-16 | 15-8  | 7-0
       opcode | arg1  | arg2  | arg3
```

- **Bits 31-24**: Opcode (instruction type)
- **Bits 23-16**: First argument (a1)
- **Bits 15-8**: Second argument (a2)
- **Bits 7-0**: Third argument (a3)

Register operands are encoded using their register number. Immediate values are embedded directly in the instruction word.

## Instruction Formats

IRRE supports 8 distinct instruction formats:

### 1. `op` - No Arguments
```
31-24: opcode | 23-0: ignored
```
Examples: `nop`, `hlt`, `ret`

### 2. `op rA` - Single Register
```
31-24: opcode | 23-16: rA | 15-0: ignored
```
Examples: `jmp rA`, `cal rA`

### 3. `op v0` - 24-bit Immediate
```
31-24: opcode | 23-0: v0 (24-bit immediate)
```
Examples: `jmi address`, `int code`

### 4. `op rA v0` - Register + 16-bit Immediate
```
31-24: opcode | 23-16: rA | 15-0: v0 (16-bit immediate)
```
Examples: `set rA immediate`, `sup rA immediate`

### 5. `op rA rB` - Two Registers
```
31-24: opcode | 23-16: rA | 15-8: rB | 7-0: ignored
```
Examples: `mov rA rB`, `not rA rB`, `sxt rA rB`

### 6. `op rA rB v0` - Two Registers + 8-bit Immediate
```
31-24: opcode | 23-16: rA | 15-8: rB | 7-0: v0 (8-bit immediate)
```
Examples: `ldw rA rB offset`, `stw rA rB offset`, `bve rA rB value`

### 7. `op rA v0 v1` - Register + Two 8-bit Immediates
```
31-24: opcode | 23-16: rA | 15-8: v0 | 7-0: v1
```
Examples: `sia rA shift_value shift_amount`

### 8. `op rA rB rC` - Three Registers
```
31-24: opcode | 23-16: rA | 15-8: rB | 7-0: rC
```
Examples: `add rA rB rC`, `sub rA rB rC`, `tcu rA rB rC`

## Instruction Set

| Mnemonic | Opcode | Format | Description |
|----------|--------|---------|-------------|
| `nop` | 0x00 | `op` | No operation |
| `add` | 0x01 | `op rA rB rC` | Unsigned addition: rA = rB + rC |
| `sub` | 0x02 | `op rA rB rC` | Unsigned subtraction: rA = rB - rC |
| `and` | 0x03 | `op rA rB rC` | Bitwise AND: rA = rB & rC |
| `orr` | 0x04 | `op rA rB rC` | Bitwise OR: rA = rB \| rC |
| `xor` | 0x05 | `op rA rB rC` | Bitwise XOR: rA = rB ^ rC |
| `not` | 0x06 | `op rA rB` | Bitwise NOT: rA = ~rB |
| `lsh` | 0x07 | `op rA rB rC` | Logical shift: rA = rB << rC (left if rC > 0, right if rC < 0) |
| `ash` | 0x08 | `op rA rB rC` | Arithmetic shift: rA = rB << rC (sign-extending on right shift) |
| `tcu` | 0x09 | `op rA rB rC` | Test compare unsigned: rA = sign(rB - rC) |
| `tcs` | 0x0a | `op rA rB rC` | Test compare signed: rA = sign(rB - rC) |
| `set` | 0x0b | `op rA v0` | Set register: rA = v0 (16-bit immediate) |
| `mov` | 0x0c | `op rA rB` | Move register: rA = rB |
| `ldw` | 0x0d | `op rA rB v0` | Load word: rA = memory[rB + v0] |
| `stw` | 0x0e | `op rA rB v0` | Store word: memory[rB + v0] = rA |
| `ldb` | 0x0f | `op rA rB v0` | Load byte: rA = memory[rB + v0] (byte) |
| `stb` | 0x10 | `op rA rB v0` | Store byte: memory[rB + v0] = rA (lower 8 bits) |
| `jmi` | 0x20 | `op v0` | Jump immediate: pc = v0 (24-bit address) |
| `jmp` | 0x21 | `op rA` | Jump register: pc = rA |
| `bve` | 0x24 | `op rA rB v0` | Branch if equal: if rB == v0 then pc = rA |
| `bvn` | 0x25 | `op rA rB v0` | Branch if not equal: if rB != v0 then pc = rA |
| `cal` | 0x2a | `op rA` | Call: lr = pc + 4; pc = rA |
| `ret` | 0x2b | `op` | Return: pc = lr; lr = 0 |
| `mul` | 0x30 | `op rA rB rC` | Unsigned multiplication: rA = rB * rC |
| `div` | 0x31 | `op rA rB rC` | Unsigned division: rA = rB / rC |
| `mod` | 0x32 | `op rA rB rC` | Unsigned modulus: rA = rB % rC |
| `sia` | 0x40 | `op rA v0 v1` | Shift and add: rA = rA + (v0 << v1) |
| `sup` | 0x41 | `op rA v0` | Set upper 16 bits: rA = (rA & 0xFFFF) \| (v0 << 16) |
| `sxt` | 0x42 | `op rA rB` | Sign extend: rA = sign_extend(rB) |
| `seq` | 0x43 | `op rA rB v0` | Set if equal: rA = (rB == v0) ? 1 : 0 |
| `int` | 0xf0 | `op v0` | Interrupt: raise interrupt with code v0 |
| `snd` | 0xfd | `op rA rB rC` | Send to device: send command rB to device rA with argument rC; result in rC |
| `hlt` | 0xff | `op` | Halt execution |

## Instruction Semantics

### Arithmetic Operations
- All arithmetic operations are performed on 32-bit unsigned values unless otherwise specified
- Overflow behavior wraps around (modulo 2^32)
- Division by zero behavior is implementation-defined

### Shift Operations
- `lsh`: Logical shift - vacant bits filled with zeros
- `ash`: Arithmetic shift - left shift fills with zeros, right shift preserves sign bit
- Shift amounts outside range (-32, 32) produce undefined results

### Comparison Operations
- `tcu`/`tcs`: Store the sign of subtraction result (-1, 0, or 1) in destination register
- These are NOT boolean operations - they return the mathematical sign

### Memory Operations
- All memory addresses are 32-bit values
- Word operations (`ldw`/`stw`) should be 4-byte aligned for optimal performance
- Unaligned access behavior is implementation-defined
- Byte operations access single bytes regardless of alignment

### Control Flow
- Jump instructions update the program counter immediately
- `cal` (call) saves the return address (pc + 4) in the link register
- `ret` jumps to the link register and clears it
- Branch instructions (`bve`/`bvn`) jump to the address in the first register operand

## Endianness

IRRE uses **little-endian** byte ordering:
- Least significant byte stored at the lowest memory address
- Instructions and data follow this convention
- Multi-byte values are stored with LSB first

## Implementation Notes

- Register validation: All register operands must be in range 0x00-0x24
- Immediate value ranges must be respected for each format
- Implementation may define behavior for undefined operations
- The architecture supports both hardware and software implementations

## Version History

- **v2.0**: Current specification as implemented