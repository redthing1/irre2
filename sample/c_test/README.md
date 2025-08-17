# C Test Programs for IRRE Compiler

This directory contains simple C programs to test the IRRE compiler backend.

## Test Programs

1. **01_simple.c** - Basic function returning a constant
   - Tests: function structure, return statement, constants

2. **02_arithmetic.c** - Basic arithmetic operations  
   - Tests: variable declarations, addition, multiplication, subtraction

3. **03_comparisons.c** - Comparison operations
   - Tests: ==, !=, <, <= operators, boolean results

4. **04_bitwise.c** - Bitwise operations
   - Tests: &, |, ^, ~ operators, hexadecimal constants

5. **05_logical.c** - Logical operations with short-circuiting
   - Tests: &&, ||, ! operators, short-circuit evaluation

6. **06_unary.c** - Unary operations
   - Tests: unary minus, bitwise NOT, logical NOT

7. **07_assignments.c** - Assignment operations
   - Tests: simple assignment, nested assignments, assignment expressions

## Usage

To test the compiler:

```bash
# Compile a test program
./build-macos/bin/irre-cc -S sample/c_test/01_simple.c -o sample/c_test/01_simple.s

# View the generated assembly
cat test.s

# Assemble and run (when assembler supports the output)
./build-macos/bin/irre-asm sample/c_test/01_simple.s -o sample/c_test/01_simple.o
# Run emulator with instruction limit and view final register state
./build-macos/bin/irre-emu -d ./sample/asm/recursive_fib.o -L 1000
# Run emulator in trace/debug mode with instruction limit, and view register state
./build-macos/bin/irre-emu -d ./sample/c_test/01_simple.o -t -L 1000 --semantics
```

## Expected Behavior

Each test program should:
1. Compile without errors
2. Generate readable IRRE assembly code
3. Produce correct results when assembled and run

The return values are designed to be easily verified:
- 01_simple.c: returns 42
- 02_arithmetic.c: returns 55 (10+20)*2-5 = 60-5 = 55
- 03_comparisons.c: returns 1 (0+1+0+0 = 1)
- 04_bitwise.c: returns 510 (0+255+255 = 510)
- 05_logical.c: returns 4 (1+0+1+1+0+1 = 4)
- 06_unary.c: returns -42 (-42+0 = -42)
- 07_assignments.c: returns 40 (20+20 = 40)