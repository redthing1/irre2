# IRRE C Compiler Control Flow Tests

This directory contains comprehensive tests for control flow statements implemented in the IRRE C compiler.

## Test Coverage

### If/Else Statements ✅
- **09_simple_if.c**: Basic if statement without else clause → returns 42
- **09_if_else.c**: If-else with both branches → returns 100 or 200

### While Loops ✅  
- **10_simple_while.c**: Basic counting while loop → returns 30 (3×10)
- **11_while_countdown.c**: Countdown while loop → returns 15 (5+4+3+2+1)

### For Loops ✅
- **12_for_loop.c**: Standard for loop with init/condition/increment → returns 10 (1+2+3+4)

### Do-While Loops ✅
- **13_do_while.c**: Standard do-while loop → returns 15 (3×5)
- **14_do_while_once.c**: Tests "execute at least once" behavior → returns 42

### Combined Control Flow ✅
- **15_control_flow_combined.c**: Complex program using for, while, do-while, and if statements → returns 30

## Implementation Notes

All control flow statements are fully implemented in `src/irre_cc/codegen.c`:

- **ND_IF**: Implemented in `gen_if_stmt()` with proper branch labels
- **ND_FOR**: Implemented in `gen_for_stmt()` handles both for and while loops  
- **ND_DO**: Implemented in `gen_do_while_stmt()` with correct "execute at least once" semantics

## Testing Methodology

Each test follows this pattern:
1. Compile with `irre-cc -S` to generate IRRE assembly
2. Assemble with `irre-asm` to create object file
3. Execute with `irre-emu --debug` to verify results
4. Check return value matches expected calculation

## Architecture Compliance

All generated code follows IRRE ABI:
- Uses r0-r7 for arguments and return values  
- Uses r8-r15 for temporaries
- Uses r16-r27 for saved registers
- Uses r30 as frame pointer
- Proper stack frame management with neat documentation