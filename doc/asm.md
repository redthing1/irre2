# IRRE Assembler Syntax

This document describes the syntax and features of the IRRE v2.x assembler.

## Overview

The IRRE v2.x assembler translates assembly language source code into IRRE object files containing executable machine code. It supports all IRRE v2.0 architecture instructions, labels, symbols, directives, and pseudo-instructions.

## Basic Syntax

### Comments
Comments begin with a semicolon (`;`) and continue to the end of the line:
```assembly
nop        ; This is a comment
add r1 r2 r3  ; Add r2 and r3, store in r1
```

### Whitespace
- Spaces and tabs separate tokens
- Empty lines are ignored
- Indentation is optional but recommended for readability

### Case Sensitivity
- Instruction mnemonics are case-sensitive and must be lowercase
- Register names are case-sensitive and must be lowercase
- Label names are case-sensitive

## Numbers and Literals

### Decimal Numbers
Plain numbers or numbers prefixed with `#` are treated as decimal:
```assembly
set r0 42      ; Decimal 42
set r1 #100    ; Decimal 100 (explicit)
```

### Hexadecimal Numbers
Numbers prefixed with `$` are treated as hexadecimal:
```assembly
set r0 $ff     ; Hexadecimal FF (255 decimal)
set r1 $1234   ; Hexadecimal 1234 (4660 decimal)
```

### Negative Numbers
Both decimal and hexadecimal numbers can be negative:
```assembly
set r0 #-10    ; Decimal -10
set r1 $-ff    ; Hexadecimal -FF
```

## Registers

### General Purpose Registers
```assembly
r0, r1, r2, ..., r31    ; 32 general-purpose registers
```

### Special Registers
```assembly
pc    ; Program counter
lr    ; Link register (return address)
ad    ; Assembler temporary 1
at    ; Assembler temporary 2
sp    ; Stack pointer
```

All registers can be used as operands in any instruction that accepts register arguments.

## Labels and Symbols

### Label Definition
Labels are defined by placing an identifier followed by a colon:
```assembly
main:           ; Define label 'main'
    nop
    
loop_start:     ; Define label 'loop_start'
    add r0 r0 r1
```

### Label References
Labels can be referenced by name in instructions that accept addresses:
```assembly
jmi main        ; Jump to label 'main'
set lr return_point  ; Load address of 'return_point' into lr
set ad loop_start    ; Load address of 'loop_start' into ad
bve ad r0 0          ; Branch to 'loop_start' if r0 == 0
```

### Symbol Resolution
- Forward references are supported (labels defined later in the source)
- Backward references are supported (labels defined earlier)
- For small programs, symbol addresses typically fit in 16-bit immediates
- The assembler automatically resolves label addresses during assembly

## Directives

### Entry Point Directive
Specify the program entry point:
```assembly
%entry: main

main:
    nop
    hlt
```

### Section Directive
```assembly
%section code    ; Mark beginning of code section
%section data    ; Mark beginning of data section
```

### Data Directive
The `%d` directive embeds data directly into the object file:

```assembly
; String data
%d "Hello, World!\n"

; Numeric data (stored as 32-bit little-endian words)
%d 42 100 $ff

; Mixed string and numeric data
%d "Message: " 0 999

; Multiple data blocks
strings:
    %d "First string"
numbers:
    %d 10 20 30
```

#### Data Types Supported:
- **String literals**: `"text"` with escape sequences (`\n`, `\t`, `\r`, `\\`, `\"`, `\0`)
- **Decimal numbers**: `42` or `#42`  
- **Hexadecimal numbers**: `$ff` or `$1234`
- **Mixed content**: Multiple values separated by spaces

#### Storage Format:
- Strings are stored as-is (UTF-8 bytes)
- Numbers are stored as 32-bit little-endian words
- Comments after `%d` are ignored: `%d 42 ; this is a comment`

## Instructions

### Format Examples

#### No Arguments
```assembly
nop             ; No operation
hlt             ; Halt
ret             ; Return
```

#### Single Register
```assembly
jmp r0          ; Jump to address in r0
cal r1          ; Call function at address in r1
```

#### 24-bit Immediate
```assembly
jmi main        ; Jump to label 'main'
jmi $1000       ; Jump to address 0x1000
int $123456     ; Interrupt with code 0x123456
```

#### Register + 16-bit Immediate
```assembly
set r0 42       ; Set r0 to 42
set r1 $ff00    ; Set r1 to 0xff00
sup r2 $1234    ; Set upper 16 bits of r2 to 0x1234
```

#### Two Registers
```assembly
mov r0 r1       ; Copy r1 to r0
not r2 r3       ; Bitwise NOT of r3 into r2
sxt r4 r5       ; Sign extend r5 into r4
```

#### Two Registers + 8-bit Immediate
```assembly
ldw r0 sp 0     ; Load word from sp+0 into r0
stw r1 sp 4     ; Store r1 to sp+4
ldb r2 r3 8     ; Load byte from r3+8 into r2
stb r4 r5 12    ; Store byte (r4) to r5+12
seq r6 r7 42    ; Set r6 to 1 if r7 == 42, else 0
bve r0 r1 0     ; Branch to r0 if r1 == 0
bvn r0 r1 1     ; Branch to r0 if r1 != 1
```

#### Register + Two 8-bit Immediates
```assembly
sia r0 $12 $34  ; Shift and add: r0 = r0 + (0x12 << 0x34)
```

#### Three Registers
```assembly
add r0 r1 r2    ; r0 = r1 + r2
sub r3 r4 r5    ; r3 = r4 - r5
mul r6 r7 r8    ; r6 = r7 * r8
div r9 r10 r11  ; r9 = r10 / r11
mod r12 r13 r14 ; r12 = r13 % r14
and r15 r16 r17 ; r15 = r16 & r17
orr r18 r19 r20 ; r18 = r19 | r20
xor r21 r22 r23 ; r21 = r22 ^ r23
lsh r24 r25 r26 ; r24 = r25 << r26
ash r27 r28 r29 ; r27 = r28 << r29 (arithmetic)
tcu r30 r31 r0  ; r30 = sign(r31 - r0) unsigned
tcs r1 r2 r3    ; r1 = sign(r2 - r3) signed
snd r4 r5 r6    ; Send command r5 to device r4 with arg r6
```

## Pseudo-Instructions

The assembler provides pseudo-instructions that expand into multiple real instructions using temporary registers:

### `adi` - Add Immediate
Adds an immediate value to a register:
```assembly
adi r0 r1 42    ; r0 = r1 + 42
```
Expands to:
```assembly
set at 42       ; Load immediate into temporary
add r0 r1 at    ; Perform addition
```

### `sbi` - Subtract Immediate  
Subtracts an immediate value from a register:
```assembly
sbi r0 r1 10    ; r0 = r1 - 10
```
Expands to:
```assembly
set at 10       ; Load immediate into temporary
sub r0 r1 at    ; Perform subtraction
```


## Complete Program Examples

### Fibonacci Calculator
```assembly
%entry: main

; Fibonacci calculator program
main:
    set r0 0        ; fib(0) = 0
    set r1 1        ; fib(1) = 1  
    set r2 10       ; calculate fib(10)
    set r3 0        ; counter
    
fib_loop:
    ; Check if counter >= target
    tcu r4 r3 r2    ; r4 = sign(r3 - r2)
    set ad done     ; Load done address
    bve ad r4 1     ; if r4 == 1 (r3 >= r2), branch to done
    
    ; Calculate next fibonacci number
    add r4 r0 r1    ; r4 = r0 + r1
    mov r0 r1       ; r0 = r1 (previous becomes current)
    mov r1 r4       ; r1 = r4 (new value)
    
    ; Increment counter  
    adi r3 r3 1     ; r3 = r3 + 1
    
    jmi fib_loop    ; Loop back
    
done:
    hlt             ; Halt execution
```

### Program with Data Section
```assembly
%entry: main

main:
    ; Load string address
    set r0 message
    
    ; Load array and process
    set r1 numbers
    ldw r2 r1 0     ; Load first number
    ldw r3 r1 4     ; Load second number
    add r4 r2 r3    ; Add them
    
    ; Store result
    set r5 result
    stw r4 r5 0
    
    hlt

; Data section
message:
    %d "Hello, IRRE v2.0!\n"

numbers:
    %d 25 17        ; Two numbers to add

result:
    %d 0            ; Space for result
```

## Error Handling

The assembler performs comprehensive validation:

### Syntax Errors
- Invalid instruction mnemonics
- Incorrect operand counts
- Malformed number literals
- Invalid register names

### Semantic Errors
- Undefined label references
- Duplicate label definitions
- Immediate values out of range
- Invalid register operands for instruction format

### Range Validation
- 8-bit immediates: 0-255
- 16-bit immediates: 0-65535  
- 24-bit immediates: 0-16777215
- Register indices: 0-36 (0x00-0x24)

## Object File Output

The assembler produces object files containing:
- Machine code in IRRE binary format
- Entry point information
- Symbol table (for debugging)
- Section information

Object files use the RGVM format with magic bytes "RGVM" and are compatible with IRRE virtual machines and debuggers.

## Implementation Notes

### Multi-pass Assembly
1. **Parse**: Syntax analysis and token extraction
2. **Symbol Table**: Build label-to-address mappings
3. **Code Generation**: Generate machine code with symbol resolution
4. **Output**: Write object file with all sections

### Temporary Register Usage
- `at` (assembler temporary 2): Used by `adi`, `sbi` pseudo-instructions
- `ad` (assembler temporary 1): Used for address calculations
- User code should avoid modifying these during pseudo-instruction usage

### Symbol Resolution Strategy
- Small programs: Labels resolved as 16-bit immediates
- Large programs: May require additional address calculation sequences
- Forward references resolved in second pass

This assembler provides a complete toolchain for IRRE development with comprehensive error checking and modern assembly language features.