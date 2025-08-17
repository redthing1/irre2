/*
 * IRRE Code Generator
 * 
 * Generates IRRE assembly code from C AST
 * Target: IRRE v2.0 32-bit architecture
 */

#include "chibicc.h"

/* ================================================================
 * IRRE ABI (Application Binary Interface)
 * ================================================================
 * 
 * Register Convention:
 * r0:      Return value / First argument
 * r1-r7:   Arguments 2-8 (8 args max before stack)
 * r8-r15:  Temporaries (caller-saved)
 * r16-r27: Saved registers (callee must preserve)
 * r28:     Constant temp (tc) - for loading immediates
 * r29:     Address temp (ta) - for loading addresses/labels
 * r30:     Frame pointer (fp) - points to current frame base
 * r31:     Compiler temp (ct) - scratch for compiler operations
 * 
 * sp:      Stack pointer (grows downward)
 * lr:      Link register (return address)
 * pc:      Program counter
 * ad,at:   Assembler reserved (never use)
 * 
 * Stack Frame Layout:
 * +------------------+ <- high memory
 * | arg 9+           | <- args beyond r0-r7
 * +------------------+
 * | return address   | <- fp + 4
 * | saved fp         | <- fp points here
 * +------------------+
 * | saved r16-r27    | <- only saved if used
 * +------------------+
 * | local variables  | <- accessed via fp - offset
 * +------------------+
 * | temp spills      | <- for complex expressions
 * +------------------+ <- sp (low memory)
 * 
 * Calling Convention:
 * 1. Caller places args in r0-r7 (extras on stack)
 * 2. Caller saves r8-r15 if needed (caller-saved)
 * 3. Call instruction saves pc+4 to lr
 * 4. Callee saves fp, r16-r27 if it uses them
 * 5. Return value goes in r0
 * 6. Callee restores saved registers
 * 7. Return jumps to lr
 */

/* ================================================================
 * Register Definitions
 * ================================================================ */

// Register numbers
#define REG_R0    0
#define REG_R1    1
#define REG_R2    2
#define REG_R3    3
#define REG_R4    4
#define REG_R5    5
#define REG_R6    6
#define REG_R7    7
#define REG_R8    8
#define REG_R9    9
#define REG_R10   10
#define REG_R11   11
#define REG_R12   12
#define REG_R13   13
#define REG_R14   14
#define REG_R15   15
#define REG_R16   16
#define REG_R17   17
#define REG_R18   18
#define REG_R19   19
#define REG_R20   20
#define REG_R21   21
#define REG_R22   22
#define REG_R23   23
#define REG_R24   24
#define REG_R25   25
#define REG_R26   26
#define REG_R27   27
#define REG_R28   28  // tc (temp for constants)
#define REG_R29   29  // ta (temp for addresses)
#define REG_R30   30  // fp (frame pointer)
#define REG_R31   31  // ct (compiler temp)

// Special registers (encoded as 0x20+)
#define REG_PC    0x20
#define REG_LR    0x21
#define REG_AD    0x22  // assembler temp 1 (reserved)
#define REG_AT    0x23  // assembler temp 2 (reserved)
#define REG_SP    0x24  // stack pointer

// Register allocation ranges
#define ARG_REG_MIN     REG_R0
#define ARG_REG_MAX     REG_R7
#define TEMP_REG_MIN    REG_R8
#define TEMP_REG_MAX    REG_R15
#define SAVED_REG_MIN   REG_R16
#define SAVED_REG_MAX   REG_R27

/* ================================================================
 * Global State
 * ================================================================ */

static FILE *output_file;
static Obj *current_fn;
static int label_counter = 1;
static int temp_reg_counter = TEMP_REG_MIN;

/* ================================================================
 * Utility Functions
 * ================================================================ */

// Get node kind name for debugging
static const char *node_kind_name(NodeKind kind) {
    switch (kind) {
        case ND_NULL_EXPR: return "ND_NULL_EXPR";
        case ND_ADD: return "ND_ADD";
        case ND_SUB: return "ND_SUB";
        case ND_MUL: return "ND_MUL";
        case ND_DIV: return "ND_DIV";
        case ND_NEG: return "ND_NEG";
        case ND_MOD: return "ND_MOD";
        case ND_BITAND: return "ND_BITAND";
        case ND_BITOR: return "ND_BITOR";
        case ND_BITXOR: return "ND_BITXOR";
        case ND_SHL: return "ND_SHL";
        case ND_SHR: return "ND_SHR";
        case ND_EQ: return "ND_EQ";
        case ND_NE: return "ND_NE";
        case ND_LT: return "ND_LT";
        case ND_LE: return "ND_LE";
        case ND_ASSIGN: return "ND_ASSIGN";
        case ND_COND: return "ND_COND";
        case ND_COMMA: return "ND_COMMA";
        case ND_MEMBER: return "ND_MEMBER";
        case ND_ADDR: return "ND_ADDR";
        case ND_DEREF: return "ND_DEREF";
        case ND_NOT: return "ND_NOT";
        case ND_BITNOT: return "ND_BITNOT";
        case ND_LOGAND: return "ND_LOGAND";
        case ND_LOGOR: return "ND_LOGOR";
        case ND_RETURN: return "ND_RETURN";
        case ND_IF: return "ND_IF";
        case ND_FOR: return "ND_FOR";
        case ND_DO: return "ND_DO";
        case ND_SWITCH: return "ND_SWITCH";
        case ND_CASE: return "ND_CASE";
        case ND_BLOCK: return "ND_BLOCK";
        case ND_GOTO: return "ND_GOTO";
        case ND_LABEL: return "ND_LABEL";
        case ND_FUNCALL: return "ND_FUNCALL";
        case ND_EXPR_STMT: return "ND_EXPR_STMT";
        case ND_STMT_EXPR: return "ND_STMT_EXPR";
        case ND_VAR: return "ND_VAR";
        case ND_NUM: return "ND_NUM";
        case ND_CAST: return "ND_CAST";
        case ND_MEMZERO: return "ND_MEMZERO";
        default: return "UNKNOWN";
    }
}

/* ================================================================
 * Forward Declarations
 * ================================================================ */

static void gen_expr(Node *node);
static void gen_stmt(Node *node);
static void gen_for_stmt(Node *node);
static void gen_do_while_stmt(Node *node);
static void gen_address_of(Node *node);
static void gen_dereference(Node *node);
static void gen_member_access(Node *node);
static void gen_stmt_expr(Node *node);
static void gen_function_call(Node *node);

/* ================================================================
 * Basic Output Functions
 * ================================================================ */

static void emit(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(output_file, fmt, ap);
    va_end(ap);
    fprintf(output_file, "\n");
}

static void emit_comment(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(output_file, "    ; ");
    vfprintf(output_file, fmt, ap);
    va_end(ap);
    fprintf(output_file, "\n");
}

static void emit_section_comment(const char *section) {
    emit("");
    emit("; ============================================");
    emit("; %s", section);
    emit("; ============================================");
}

static void emit_label(const char *label) {
    emit("%s:", label);
}

// Sanitize label names for IRRE assembler (replace dots with underscores)
static char *sanitize_label(const char *label) {
    if (!label) return NULL;
    
    char *result = strdup(label);
    for (char *p = result; *p; p++) {
        if (*p == '.') {
            *p = '_';
        }
    }
    return result;
}

static int new_label(void) {
    return label_counter++;
}

static char *format_label(const char *prefix, int id) {
    static char buf[256];
    snprintf(buf, sizeof(buf), "_L_%s_%d", prefix, id);
    return buf;
}

/* ================================================================
 * Helper Functions - Constants and Immediates
 * ================================================================ */

// Load a 32-bit constant into register
static void emit_load_const(int reg, uint32_t value) {
    if (value == 0) {
        emit("    set r%d #0           ; r%d = 0", reg, reg);
    } else if (value <= 0xFFFF) {
        emit("    set r%d $%04x       ; r%d = 0x%x", reg, value, reg, value);
    } else {
        uint16_t lo = value & 0xFFFF;
        uint16_t hi = value >> 16;
        emit("    set r%d $%04x       ; r%d = 0x%x (low 16 bits)", reg, lo, reg, value);
        emit("    sup r%d $%04x       ; r%d |= 0x%x << 16", reg, hi, reg, hi);
    }
}

// Load a label address into register
static void emit_load_label(int reg, const char *label) {
    char *clean_label = sanitize_label(label);
    emit("    set r%d %s         ; r%d = address of %s", reg, clean_label, reg, label);
    free(clean_label);
}

// Convert register number to IRRE register name
static const char *reg_name(int reg) {
    static char buf[16];
    
    if (reg >= 0 && reg <= 31) {
        snprintf(buf, sizeof(buf), "r%d", reg);
        return buf;
    }
    
    switch (reg) {
        case REG_PC: return "pc";
        case REG_LR: return "lr";
        case REG_AD: return "ad";
        case REG_AT: return "at";
        case REG_SP: return "sp";
        default:
            snprintf(buf, sizeof(buf), "reg%d", reg);
            return buf;
    }
}

/* ================================================================
 * Helper Functions - Stack Operations
 * ================================================================ */

// Push register onto stack (grows down)
static void emit_push(int reg) {
    emit_comment("Push %s", reg_name(reg));
    emit("    set r31 #4");
    emit("    sub sp sp r31");
    emit("    stw %s sp #0", reg_name(reg));
}

// Pop from stack into register
static void emit_pop(int reg) {
    emit_comment("Pop to %s", reg_name(reg));
    emit("    ldw %s sp #0", reg_name(reg));
    emit("    set r31 #4");
    emit("    add sp sp r31");
}

// Allocate stack space
static void emit_stack_alloc(int bytes) {
    if (bytes == 0) return;
    emit_comment("Allocate %d bytes on stack", bytes);
    emit_load_const(REG_R31, bytes);
    emit("    sub sp sp r31");
}

// Free stack space
static void emit_stack_free(int bytes) {
    if (bytes == 0) return;
    emit_comment("Free %d bytes from stack", bytes);
    emit_load_const(REG_R31, bytes);
    emit("    add sp sp r31");
}

/* ================================================================
 * Helper Functions - Arithmetic with Immediates
 * ================================================================ */

// Add immediate to register
static void emit_add_imm(int dst, int src, int32_t imm) {
    if (imm == 0) {
        if (dst != src) {
            emit("    mov r%d r%d        ; r%d = r%d + 0", dst, src, dst, src);
        }
        return;
    }
    
    emit_comment("r%d = r%d + %d", dst, src, imm);
    emit_load_const(REG_R28, imm);
    emit("    add r%d r%d r28", dst, src);
}

// Subtract immediate from register
static void emit_sub_imm(int dst, int src, int32_t imm) {
    if (imm == 0) {
        if (dst != src) {
            emit("    mov r%d r%d        ; r%d = r%d - 0", dst, src, dst, src);
        }
        return;
    }
    
    emit_comment("r%d = r%d - %d", dst, src, imm);
    emit_load_const(REG_R28, imm);
    emit("    sub r%d r%d r28", dst, src);
}

/* ================================================================
 * Helper Functions - Memory Access
 * ================================================================ */

// Load word from memory
static void emit_load_word(int dst, int base, int offset) {
    if (offset >= -128 && offset <= 127) {
        emit("    ldw r%d r%d #%d      ; load word from r%d + %d", dst, base, offset, base, offset);
    } else {
        emit_comment("Load word from r%d + %d (large offset)", base, offset);
        emit_load_const(REG_R31, offset);
        emit("    add r31 r%d r31", base);
        emit("    ldw r%d r31 #0", dst);
    }
}

// Store word to memory
static void emit_store_word(int src, int base, int offset) {
    if (offset >= -128 && offset <= 127) {
        emit("    stw r%d r%d #%d      ; store word to r%d + %d", src, base, offset, base, offset);
    } else {
        emit_comment("Store word to r%d + %d (large offset)", base, offset);
        emit_load_const(REG_R31, offset);
        emit("    add r31 r%d r31", base);
        emit("    stw r%d r31 #0", src);
    }
}

// Load byte from memory
static void emit_load_byte(int dst, int base, int offset) {
    if (offset >= -128 && offset <= 127) {
        emit("    ldb r%d r%d %d      ; load byte from r%d + %d", dst, base, offset, base, offset);
    } else {
        emit_comment("Load byte from r%d + %d (large offset)", base, offset);
        emit_load_const(REG_R31, offset);
        emit("    add r31 r%d r31", base);
        emit("    ldb r%d r31 0", dst);
    }
}

// Store byte to memory  
static void emit_store_byte(int src, int base, int offset) {
    if (offset >= -128 && offset <= 127) {
        emit("    stb r%d r%d %d      ; store byte to r%d + %d", src, base, offset, base, offset);
    } else {
        emit_comment("Store byte to r%d + %d (large offset)", base, offset);
        emit_load_const(REG_R31, offset);
        emit("    add r31 r%d r31", base);
        emit("    stb r%d r31 0", src);
    }
}

/* ================================================================
 * Helper Functions - Type Conversions
 * ================================================================ */

// Sign extend from 8 bits to 32 bits
static void emit_sign_extend_byte(int dst, int src) {
    emit_comment("Sign extend byte: r%d = sign_extend(r%d)", dst, src);
    emit_load_const(REG_R28, 24);                  // Shift amount
    emit("    lsh r%d r%d r28     ; shift left 24 bits", dst, src);
    emit_load_const(REG_R28, -24);                 // Negative shift amount  
    emit("    ash r%d r%d r28     ; arithmetic shift right 24 bits", dst, dst);
}

// Zero extend from 8 bits to 32 bits  
static void emit_zero_extend_byte(int dst, int src) {
    emit_comment("Zero extend byte: r%d = r%d & 0xFF", dst, src);
    emit_load_const(REG_R28, 0xFF);
    emit("    and r%d r%d r28", dst, src);
}

// Sign extend from 16 bits to 32 bits
static void emit_sign_extend_short(int dst, int src) {
    emit_comment("Sign extend short: r%d = sign_extend(r%d)", dst, src);
    emit_load_const(REG_R28, 16);                  // Shift amount
    emit("    lsh r%d r%d r28     ; shift left 16 bits", dst, src);
    emit_load_const(REG_R28, -16);                 // Negative shift amount
    emit("    ash r%d r%d r28     ; arithmetic shift right 16 bits", dst, dst);
}

// Zero extend from 16 bits to 32 bits
static void emit_zero_extend_short(int dst, int src) {
    emit_comment("Zero extend short: r%d = r%d & 0xFFFF", dst, src);
    emit_load_const(REG_R28, 0xFFFF);
    emit("    and r%d r%d r28", dst, src);
}

/* ================================================================
 * Helper Functions - Comparisons 
 * ================================================================ */

// Compare two registers for equality: dst = (a == b)
static void emit_compare_eq(int dst, int a, int b) {
    emit_comment("Compare equal: r%d = (r%d == r%d)", dst, a, b);
    emit("    sub r31 r%d r%d     ; r31 = r%d - r%d", a, b, a, b);
    emit("    seq r%d r31 0       ; r%d = (r31 == 0)", dst, dst);
}

// Compare two registers for inequality: dst = (a != b)
static void emit_compare_ne(int dst, int a, int b) {
    emit_comment("Compare not equal: r%d = (r%d != r%d)", dst, a, b);
    emit("    sub r31 r%d r%d     ; r31 = r%d - r%d", a, b, a, b);
    emit("    seq r31 r31 0       ; r31 = (r31 == 0)");
    emit("    set r28 #1");
    emit("    xor r%d r31 r28     ; r%d = NOT(r31)", dst, dst);
}

// Compare two registers: dst = (a < b), unsigned or signed
static void emit_compare_lt(int dst, int a, int b, bool is_signed) {
    if (is_signed) {
        emit_comment("Compare less than (signed): r%d = (r%d < r%d)", dst, a, b);
        emit("    tcs r31 r%d r%d     ; r31 = sign(r%d - r%d)", a, b, a, b);
    } else {
        emit_comment("Compare less than (unsigned): r%d = (r%d < r%d)", dst, a, b);
        emit("    tcu r31 r%d r%d     ; r31 = sign(r%d - r%d)", a, b, a, b);
    }
    emit("    set r28 #1");         // r28 = 1  
    emit("    add r31 r31 r28");   // r31 = r31 + 1 (same as r31 - (-1))
    emit("    seq r%d r31 0        ; r%d = (r31 == 0)", dst, dst);
}

// Compare two registers: dst = (a <= b), unsigned or signed  
static void emit_compare_le(int dst, int a, int b, bool is_signed) {
    emit_comment("Compare less or equal: r%d = (r%d <= r%d)", dst, a, b);
    // a <= b is equivalent to !(a > b), and a > b is b < a
    emit_compare_lt(dst, b, a, is_signed);  // dst = (b < a)
    emit("    set r28 #1");
    emit("    xor r%d r%d r28     ; r%d = NOT(r%d)", dst, dst, dst, dst);
}

// Test if register is zero: dst = (src == 0)
static void emit_test_zero(int dst, int src) {
    emit_comment("Test zero: r%d = (r%d == 0)", dst, src);
    emit("    seq r%d r%d 0", dst, src);
}

// Test if register is non-zero: dst = (src != 0)
static void emit_test_nonzero(int dst, int src) {
    emit_comment("Test non-zero: r%d = (r%d != 0)", dst, src);
    emit("    seq r31 r%d 0       ; r31 = (r%d == 0)", src, src);
    emit("    set r28 #1");
    emit("    xor r%d r31 r28     ; r%d = NOT(r31)", dst, dst);
}

/* ================================================================
 * Helper Functions - Branches and Jumps
 * ================================================================ */

// Unconditional jump to label
static void emit_jump(const char *label) {
    emit_comment("Jump to %s", label);
    emit("    jmi %s", label);
}

// Branch to label if register is zero
static void emit_branch_if_zero(int reg, const char *label) {
    emit_comment("Branch to %s if r%d == 0", label, reg);
    emit_load_label(REG_R29, label);              // Load target address
    emit("    bve r29 r%d 0       ; if r%d == 0, goto %s", reg, reg, label);
}

// Branch to label if register is non-zero  
static void emit_branch_if_nonzero(int reg, const char *label) {
    emit_comment("Branch to %s if r%d != 0", label, reg);
    emit_load_label(REG_R29, label);              // Load target address  
    emit("    bvn r29 r%d 0       ; if r%d != 0, goto %s", reg, reg, label);
}

// Branch to label if two registers are equal
static void emit_branch_if_equal(int a, int b, const char *label) {
    emit_comment("Branch to %s if r%d == r%d", label, a, b);
    emit("    sub r31 r%d r%d     ; r31 = r%d - r%d", a, b, a, b);
    emit_load_label(REG_R29, label);
    emit("    bve r29 r31 0       ; if r%d == r%d, goto %s", a, b, label);
}

// Branch to label if two registers are not equal
static void emit_branch_if_not_equal(int a, int b, const char *label) {
    emit_comment("Branch to %s if r%d != r%d", label, a, b);
    emit("    sub r31 r%d r%d     ; r31 = r%d - r%d", a, b, a, b);
    emit_load_label(REG_R29, label);
    emit("    bvn r29 r31 0       ; if r%d != r%d, goto %s", a, b, label);
}

/* ================================================================
 * Helper Functions - Function Management
 * ================================================================ */

// Call a function
static void emit_call(const char *func_name) {
    emit_comment("Call function %s", func_name);
    emit_load_label(REG_R29, func_name);
    emit("    cal r29             ; lr = pc + 4; pc = %s", func_name);
}

// Return from function
static void emit_return(void) {
    emit_comment("Return from function");
    emit("    ret                 ; pc = lr; lr = 0");
}

/* ================================================================
 * Utility Functions
 * ================================================================ */


// Get next temporary register
static int get_temp_reg(void) {
    int reg = temp_reg_counter;
    temp_reg_counter++;
    if (temp_reg_counter > TEMP_REG_MAX) {
        temp_reg_counter = TEMP_REG_MIN;
    }
    return reg;
}

// Round up to alignment (required by chibicc.h)
int align_to(int n, int align) {
    return (n + align - 1) / align * align;
}

/* ================================================================
 * Type Casting Functions 
 * ================================================================ */

// Helper function to get type name for debugging
static const char *type_name(Type *ty) {
    if (!ty) return "unknown";
    
    switch (ty->kind) {
        case TY_VOID: return "void";
        case TY_BOOL: return "_Bool";
        case TY_CHAR: return ty->is_unsigned ? "unsigned char" : "char";
        case TY_SHORT: return ty->is_unsigned ? "unsigned short" : "short";
        case TY_INT: return ty->is_unsigned ? "unsigned int" : "int";
        case TY_LONG: return ty->is_unsigned ? "unsigned long" : "long";
        case TY_FLOAT: return "float";
        case TY_DOUBLE: return "double";
        case TY_PTR: return "pointer";
        case TY_ARRAY: return "array";
        case TY_STRUCT: return "struct";
        case TY_UNION: return "union";
        case TY_FUNC: return "function";
        case TY_ENUM: return "enum";
        default: return "unknown";
    }
}

// Generate code to cast between types (value is in r0)
static void gen_type_cast(Type *from_ty, Type *to_ty) {
    if (!from_ty || !to_ty) return;
    
    // If types are the same, no conversion needed
    if (from_ty->kind == to_ty->kind && from_ty->size == to_ty->size && 
        from_ty->is_unsigned == to_ty->is_unsigned) {
        return;
    }
    
    emit_comment("Type cast from %s to %s", 
                 type_name(from_ty), type_name(to_ty));
    
    // Handle void - no conversion
    if (to_ty->kind == TY_VOID) {
        return;
    }
    
    // Handle boolean conversion
    if (to_ty->kind == TY_BOOL) {
        emit_comment("Convert to boolean (0 or 1)");
        emit_test_nonzero(REG_R0, REG_R0);              // r0 = (r0 != 0)
        return;
    }
    
    // Handle floating point (not supported in IRRE)
    if (from_ty->kind == TY_FLOAT || from_ty->kind == TY_DOUBLE ||
        to_ty->kind == TY_FLOAT || to_ty->kind == TY_DOUBLE) {
        error("Floating point types not supported in IRRE architecture");
    }
    
    // Size-based casting for integer types
    int from_size = from_ty->size;
    int to_size = to_ty->size;
    
    if (from_size == to_size) {
        // Same size, only signedness might differ - usually no-op for 32-bit
        return;
    }
    
    if (from_size > to_size) {
        // Truncation - mask to smaller size
        if (to_size == 1) {
            emit_zero_extend_byte(REG_R0, REG_R0);      // Keep only low 8 bits
        } else if (to_size == 2) {
            emit_zero_extend_short(REG_R0, REG_R0);     // Keep only low 16 bits
        }
        // 4-byte truncation from 8-byte would need special handling
    } else {
        // Extension - sign or zero extend
        if (from_size == 1) {
            if (from_ty->is_unsigned) {
                emit_zero_extend_byte(REG_R0, REG_R0);
            } else {
                emit_sign_extend_byte(REG_R0, REG_R0);
            }
        } else if (from_size == 2) {
            if (from_ty->is_unsigned) {
                emit_zero_extend_short(REG_R0, REG_R0);
            } else {
                emit_sign_extend_short(REG_R0, REG_R0);
            }
        }
    }
}

/* ================================================================
 * Expression Generation
 * ================================================================ */

// Generate code for number literals
static void gen_number(Node *node) {
    emit_comment("Load constant %ld", node->val);
    emit_load_const(REG_R0, (uint32_t)node->val);
}

// Generate code for variable references  
static void gen_variable(Node *node) {
    Obj *var = node->var;
    
    // For array types, return the address (arrays decay to pointers)
    if (var->ty->kind == TY_ARRAY) {
        if (var->is_local) {
            // Local array - compute address on stack frame
            emit_comment("Load address of local array %s (offset %d)", var->name, var->offset);
            emit_load_const(REG_R0, var->offset);
            emit("    add r0 r30 r0         ; r0 = fp + offset");
        } else {
            // Global array - load address
            emit_comment("Load address of global array %s", var->name);
            emit_load_label(REG_R0, var->name);
        }
        return;
    }
    
    if (var->is_local) {
        // Local variable - load from stack frame
        emit_comment("Load local variable %s (offset %d)", var->name, var->offset);
        emit_load_word(REG_R0, REG_R30, var->offset);  // load from fp + offset
    } else {
        // Global variable - load address then dereference
        emit_comment("Load global variable %s", var->name);
        emit_load_label(REG_R0, var->name);            // load address
        emit_load_word(REG_R0, REG_R0, 0);             // dereference
    }
}

// Generate code for address-of operator (&)
static void gen_address_of(Node *node) {
    if (node->lhs->kind != ND_VAR) {
        error_tok(node->tok, "invalid operand to address-of operator");
    }
    
    Obj *var = node->lhs->var;
    
    if (var->is_local) {
        // Local variable - compute address as fp + offset
        emit_comment("Address of local variable %s (fp + %d)", var->name, var->offset);
        if (var->offset == 0) {
            emit("    mov r0 r30           ; r0 = fp");
        } else if (var->offset > 0) {
            emit_load_const(REG_R31, var->offset);
            emit("    add r0 r30 r31       ; r0 = fp + %d", var->offset);
        } else {
            emit_load_const(REG_R31, -var->offset);
            emit("    sub r0 r30 r31       ; r0 = fp - %d", -var->offset);
        }
    } else {
        // Global variable - address is the label itself
        emit_comment("Address of global variable %s", var->name);
        emit_load_label(REG_R0, var->name);            // load address
    }
}

// Generate code for dereference operator (*)
static void gen_dereference(Node *node) {
    // Generate the pointer expression
    gen_expr(node->lhs);
    
    // Dereference: load the value at the address in r0
    emit_comment("Dereference pointer (load from address in r0)");
    emit_load_word(REG_R0, REG_R0, 0);                 // r0 = *(r0 + 0)
}

// Generate code for struct member access
static void gen_member_access(Node *node) {
    emit_comment("Struct member access");
    
    // Generate address of the struct
    gen_expr(node->lhs);                               // Get struct address in r0
    
    // Add member offset to get member address
    Member *member = node->member;
    if (member && member->offset > 0) {
        emit_comment("Add member offset %d", member->offset);
        emit_load_const(REG_R28, member->offset);
        emit("    add r0 r0 r28        ; r0 = struct_addr + member_offset");
    }
    
    // Load the member value (for most cases - addresses handled by ND_ADDR)
    if (node->ty->kind != TY_ARRAY) {
        emit_comment("Load member value");
        emit_load_word(REG_R0, REG_R0, 0);             // Load value from member address
    }
    // For arrays, return the address (arrays decay to pointers)
}

// Generate code for statement expressions
static void gen_stmt_expr(Node *node) {
    emit_comment("Statement expression");
    
    // Execute all statements in the block
    for (Node *stmt = node->body; stmt; stmt = stmt->next) {
        gen_stmt(stmt);
    }
    
    // The last statement should leave its result in r0
    // No additional work needed - the final expression result is already in r0
}

// Generate code for function calls
static void gen_function_call(Node *node) {
    // Count arguments
    int arg_count = 0;
    for (Node *arg = node->args; arg; arg = arg->next) {
        arg_count++;
    }
    
    emit_comment("Function call with %d arguments", arg_count);
    
    // Generate arguments in reverse order (right to left evaluation)
    // Arguments go in r0-r7, rest on stack
    Node **args = calloc(arg_count, sizeof(Node*));
    int i = 0;
    for (Node *arg = node->args; arg; arg = arg->next) {
        args[i++] = arg;
    }
    
    // Generate arguments from right to left and place them
    for (int j = arg_count - 1; j >= 0; j--) {
        gen_expr(args[j]);                              // Generate argument expression
        
        if (j < 8) {
            // First 8 arguments go in registers r0-r7
            if (j != 0) {
                emit_comment("Move arg %d to r%d", j, j);
                emit("    mov r%d r0           ; arg %d", j, j);
            }
            // else: arg 0 already in r0
        } else {
            // Arguments 8+ go on stack (not implemented yet)
            error_tok(node->tok, "more than 8 function arguments not supported yet");
        }
    }
    
    free(args);
    
    // Call the function - handle both direct calls and function pointers
    if (node->lhs && node->lhs->kind == ND_VAR && node->lhs->var && node->lhs->var->is_function) {
        // Direct function call
        char *func_name = node->lhs->var->name;
        emit_comment("Direct call to function %s", func_name);
        emit_load_label(REG_R29, func_name);           // r29 = function address
        emit("    cal r29             ; lr = pc + 4; pc = %s", func_name);
        emit_comment("Function %s returned (result in r0)", func_name);
    } else {
        // Function pointer call - evaluate expression to get function address
        emit_comment("Function pointer call");
        gen_expr(node->lhs);                           // Function address in r0
        emit("    mov r29 r0          ; r29 = function address from pointer");
        emit("    cal r29             ; lr = pc + 4; pc = function_address");
        emit_comment("Function pointer returned (result in r0)");
    }
}

// Generate code for binary arithmetic operations
static void gen_binary_arithmetic(Node *node) {
    // Generate right operand first (to match evaluation order)
    gen_expr(node->rhs);
    emit_push(REG_R0);                                  // save right operand
    
    // Generate left operand
    gen_expr(node->lhs);
    
    // Pop right operand into temporary
    emit_pop(REG_R8);                                   // r8 = right operand, r0 = left operand
    
    switch (node->kind) {
        case ND_ADD:
            emit_comment("Add: r0 = r0 + r8");
            emit("    add r0 r0 r8        ; addition");
            break;
            
        case ND_SUB:
            emit_comment("Subtract: r0 = r0 - r8");
            emit("    sub r0 r0 r8        ; subtraction");
            break;
            
        case ND_MUL:
            emit_comment("Multiply: r0 = r0 * r8");
            emit("    mul r0 r0 r8        ; multiplication");
            break;
            
        case ND_DIV:
            emit_comment("Divide: r0 = r0 / r8");
            if (node->ty->is_unsigned) {
                emit("    div r0 r0 r8        ; unsigned division");
            } else {
                // For signed division, we need to handle signs properly
                // IRRE div is unsigned, so we need to implement signed division
                emit_comment("Signed division not yet implemented");
                emit("    div r0 r0 r8        ; unsigned division (temporary)");
            }
            break;
            
        case ND_MOD:
            emit_comment("Modulo: r0 = r0 %% r8");
            emit("    mod r0 r0 r8        ; modulo");
            break;
            
        default:
            error_tok(node->tok, "unsupported binary arithmetic operation");
    }
}

// Generate code for bitwise operations
static void gen_binary_bitwise(Node *node) {
    // Generate right operand first
    gen_expr(node->rhs);
    emit_push(REG_R0);
    
    // Generate left operand
    gen_expr(node->lhs);
    
    // Pop right operand
    emit_pop(REG_R8);
    
    switch (node->kind) {
        case ND_BITAND:
            emit_comment("Bitwise AND: r0 = r0 & r8");
            emit("    and r0 r0 r8        ; bitwise AND");
            break;
            
        case ND_BITOR:
            emit_comment("Bitwise OR: r0 = r0 | r8");
            emit("    orr r0 r0 r8        ; bitwise OR");
            break;
            
        case ND_BITXOR:
            emit_comment("Bitwise XOR: r0 = r0 ^ r8");
            emit("    xor r0 r0 r8        ; bitwise XOR");
            break;
            
        case ND_SHL:
            emit_comment("Left shift: r0 = r0 << r8");
            emit("    lsh r0 r0 r8        ; left shift");
            break;
            
        case ND_SHR:
            emit_comment("Right shift: r0 = r0 >> r8");
            // IRRE lsh with negative amount does right shift
            emit("    mov r31 r8          ; r31 = r8 (save shift amount)");
            emit("    set r8 #0            ; r8 = 0");
            emit("    sub r8 r8 r31       ; r8 = 0 - r31 = -r31 (negate)");
            emit("    lsh r0 r0 r8        ; right shift (lsh with negative)");
            break;
            
        default:
            error_tok(node->tok, "unsupported bitwise operation");
    }
}

// Generate code for unary operations
static void gen_unary(Node *node) {
    gen_expr(node->lhs);                               // Generate operand
    
    switch (node->kind) {
        case ND_NEG:
            emit_comment("Negate: r0 = -r0");
            emit("    mov r8 r0           ; r8 = r0 (save original)");
            emit("    set r0 #0            ; r0 = 0");
            emit("    sub r0 r0 r8        ; r0 = 0 - r8 (negate)");
            break;
            
        case ND_BITNOT:
            emit_comment("Bitwise NOT: r0 = ~r0");
            emit("    not r0 r0           ; bitwise NOT");
            break;
            
        case ND_NOT:
            emit_comment("Logical NOT: r0 = !r0");
            emit_test_zero(REG_R0, REG_R0);             // r0 = (r0 == 0)
            break;
            
        default:
            error_tok(node->tok, "unsupported unary operation");
    }
}

// Generate code for assignments
static void gen_assignment(Node *node) {
    // Generate the value to assign first
    gen_expr(node->rhs);                               // result in r0
    emit_push(REG_R0);                                 // save value on stack
    
    // Handle different assignment target types
    if (node->lhs->kind == ND_VAR) {
        // Direct variable assignment
        Obj *var = node->lhs->var;
        
        emit_pop(REG_R0);                              // restore value
        
        if (var->is_local) {
            // Store to local variable
            emit_comment("Assign to local variable %s", var->name);
            emit_store_word(REG_R0, REG_R30, var->offset);
        } else {
            // Store to global variable
            emit_comment("Assign to global variable %s", var->name);
            emit_load_label(REG_R8, var->name);        // load address into temp
            emit_store_word(REG_R0, REG_R8, 0);        // store value
        }
        
    } else if (node->lhs->kind == ND_DEREF) {
        // Pointer dereference assignment: *ptr = value
        emit_comment("Assign to pointer dereference");
        
        // Generate address to store to
        gen_expr(node->lhs->lhs);                      // address in r0
        emit("    mov r8 r0               ; r8 = address");
        
        // Restore value and store
        emit_pop(REG_R0);                              // restore value
        emit_store_word(REG_R0, REG_R8, 0);           // store value to *address
        
    } else if (node->lhs->kind == ND_MEMBER) {
        // Struct member assignment: struct.member = value
        emit_comment("Assign to struct member");
        
        // Generate address of the struct
        gen_expr(node->lhs->lhs);                      // Get struct address in r0
        
        // Add member offset to get member address
        Member *member = node->lhs->member;
        if (member && member->offset > 0) {
            emit_comment("Add member offset %d", member->offset);
            emit_load_const(REG_R28, member->offset);
            emit("    add r0 r0 r28        ; r0 = struct_addr + member_offset");
        }
        
        emit("    mov r8 r0               ; r8 = member address");
        
        // Restore value and store
        emit_pop(REG_R0);                              // restore value
        emit_store_word(REG_R0, REG_R8, 0);           // store value to member
        
    } else {
        error_tok(node->tok, "invalid assignment target");
    }
    
    // Assignment expressions return the assigned value (already in r0)
}

// Generate code for comparison operations
static void gen_comparison(Node *node) {
    // Generate right operand first
    gen_expr(node->rhs);
    emit_push(REG_R0);
    
    // Generate left operand
    gen_expr(node->lhs);
    
    // Pop right operand
    emit_pop(REG_R8);                                   // r8 = right, r0 = left
    
    bool is_signed = !node->lhs->ty->is_unsigned;
    
    switch (node->kind) {
        case ND_EQ:
            emit_compare_eq(REG_R0, REG_R0, REG_R8);
            break;
            
        case ND_NE:
            emit_compare_ne(REG_R0, REG_R0, REG_R8);
            break;
            
        case ND_LT:
            emit_compare_lt(REG_R0, REG_R0, REG_R8, is_signed);
            break;
            
        case ND_LE:
            emit_compare_le(REG_R0, REG_R0, REG_R8, is_signed);
            break;
            
        default:
            error_tok(node->tok, "unsupported comparison operation");
    }
}

// Generate code for logical operations
static void gen_logical(Node *node) {
    int end_label = new_label();
    
    switch (node->kind) {
        case ND_LOGAND: {
            // Short-circuit AND: if left is false, result is false
            int false_label = new_label();
            
            gen_expr(node->lhs);                       // Generate left operand
            emit_branch_if_zero(REG_R0, format_label("false", false_label));
            
            gen_expr(node->rhs);                       // Generate right operand
            emit_branch_if_zero(REG_R0, format_label("false", false_label));
            
            // Both are true
            emit_load_const(REG_R0, 1);
            emit_jump(format_label("end", end_label));
            
            // At least one is false
            emit_label(format_label("false", false_label));
            emit_load_const(REG_R0, 0);
            
            emit_label(format_label("end", end_label));
            break;
        }
        
        case ND_LOGOR: {
            // Short-circuit OR: if left is true, result is true
            int true_label = new_label();
            
            gen_expr(node->lhs);                       // Generate left operand
            emit_branch_if_nonzero(REG_R0, format_label("true", true_label));
            
            gen_expr(node->rhs);                       // Generate right operand
            emit_branch_if_nonzero(REG_R0, format_label("true", true_label));
            
            // Both are false
            emit_load_const(REG_R0, 0);
            emit_jump(format_label("end", end_label));
            
            // At least one is true
            emit_label(format_label("true", true_label));
            emit_load_const(REG_R0, 1);
            
            emit_label(format_label("end", end_label));
            break;
        }
        
        default:
            error_tok(node->tok, "unsupported logical operation");
    }
}

// Generate code for ternary conditional operator (condition ? true_expr : false_expr)
static void gen_ternary(Node *node) {
    int false_label = label_counter++;
    int end_label = label_counter++;
    
    // Generate condition
    gen_expr(node->cond);
    
    // If condition is false, jump to false expression
    emit_comment("Ternary condition test");
    emit("    set r31 %s", format("_L_ternary_false_%d", false_label));
    emit("    bve r31 r0 #0       ; branch if condition is false");
    
    // Generate true expression
    emit_comment("Ternary true expression");
    gen_expr(node->then);
    emit("    jmi %s", format("_L_ternary_end_%d", end_label));
    
    // Generate false expression
    emit_label(format("_L_ternary_false_%d", false_label));
    emit_comment("Ternary false expression");
    gen_expr(node->els);
    
    // End label
    emit_label(format("_L_ternary_end_%d", end_label));
}

// Main expression generator
static void gen_expr(Node *node) {
    if (!node) return;
    
    emit_comment("Expression: %s (line %d)", 
                 node->tok ? "" : "unknown", 
                 node->tok ? node->tok->line_no : 0);
    
    switch (node->kind) {
        case ND_NUM:
            gen_number(node);
            break;
            
        case ND_VAR:
            gen_variable(node);
            break;
            
        case ND_ADDR:
            gen_address_of(node);
            break;
            
        case ND_DEREF:
            gen_dereference(node);
            break;
            
        case ND_MEMBER:
            gen_member_access(node);
            break;
            
        case ND_FUNCALL:
            gen_function_call(node);
            break;
            
        case ND_ADD:
        case ND_SUB:
        case ND_MUL:
        case ND_DIV:
        case ND_MOD:
            gen_binary_arithmetic(node);
            break;
            
        case ND_BITAND:
        case ND_BITOR:
        case ND_BITXOR:
        case ND_SHL:
        case ND_SHR:
            gen_binary_bitwise(node);
            break;
            
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
            gen_comparison(node);
            break;
            
        case ND_LOGAND:
        case ND_LOGOR:
            gen_logical(node);
            break;
            
        case ND_NEG:
        case ND_BITNOT:
        case ND_NOT:
            gen_unary(node);
            break;
            
        case ND_ASSIGN:
            gen_assignment(node);
            break;
            
        case ND_NULL_EXPR:
            // Do nothing expression
            emit_comment("Null expression");
            emit_load_const(REG_R0, 0);
            break;
            
        case ND_CAST:
            // Type cast - generate the expression and convert types
            gen_expr(node->lhs);                           // Generate the expression to cast
            gen_type_cast(node->lhs->ty, node->ty);        // Convert from source to dest type
            break;
            
        case ND_COND:
            gen_ternary(node);
            break;
            
        case ND_COMMA:
            // Comma operator: evaluate left, discard result, return right
            emit_comment("Comma operator: evaluate left expression");
            gen_expr(node->lhs);                           // Generate and discard left expression
            emit_comment("Comma operator: evaluate right expression");
            gen_expr(node->rhs);                           // Generate right expression (result)
            break;
            
        case ND_STMT_EXPR:
            // Statement expression: ({ statements; final_expression; })
            gen_stmt_expr(node);
            break;
            
        case ND_MEMZERO: {
            // Zero-initialize a stack variable
            emit_comment("Zero-initialize variable");
            int size = node->var->ty->size;
            
            // For small variables, just store zero directly
            if (size <= 4) {
                emit_load_const(REG_R0, 0);
                if (node->var->is_local) {
                    emit("    stw r0 r30 #%d      ; store word to r30 + %d", 
                         node->var->offset, node->var->offset);
                } else {
                    // Global variable
                    emit("    set r31 %s", node->var->name);
                    emit("    stw r0 r31 #0       ; store to global %s", node->var->name);
                }
            } else {
                // For larger variables, zero byte by byte
                emit_load_const(REG_R0, 0);                    // r0 = 0 (value to store)
                for (int i = 0; i < size; i++) {
                    if (node->var->is_local) {
                        emit("    stb r0 r30 #%d      ; zero byte at offset %d", 
                             node->var->offset + i, node->var->offset + i);
                    } else {
                        emit("    set r31 %s", node->var->name);
                        emit("    stb r0 r31 #%d      ; zero byte %d of global %s", 
                             i, i, node->var->name);
                    }
                }
            }
            break;
        }
            
        default:
            error_tok(node->tok, "unsupported expression type: %s", 
                     node_kind_name(node->kind));
    }
}

/* ================================================================
 * Statement Generation
 * ================================================================ */

// Generate code for expression statements
static void gen_expression_stmt(Node *node) {
    emit_comment("Expression statement");
    gen_expr(node->lhs);                               // Generate expression, ignore result
}

// Generate code for return statements
static void gen_return_stmt(Node *node) {
    emit_comment("Return statement");
    
    if (node->lhs) {
        gen_expr(node->lhs);                           // Generate return value into r0
    } else {
        emit_load_const(REG_R0, 0);                    // Default return value is 0
    }
    
    // Jump to function epilogue  
    static char return_label[256];
    snprintf(return_label, sizeof(return_label), "_L_return_%s", current_fn->name);
    emit_jump(return_label);
}

// Generate if statement
static void gen_if_stmt(Node *node) {
    int id = label_counter++;
    char *else_label = format("_L_if_else_%d", id);
    char *end_label = format("_L_if_end_%d", id);
    
    emit_comment("If statement");
    
    // Generate condition
    emit_comment("Evaluate if condition");
    gen_expr(node->cond);                               // Condition result in r0
    
    // Jump to else if condition is false (r0 == 0)
    emit_comment("Jump to else if condition is false");
    emit("    set r31 %s       ; r31 = address of %s", else_label, else_label);
    emit("    bve r31 r0 #0    ; if (r0 == 0) goto %s", else_label);
    
    // Generate then block
    emit_comment("Then block");
    gen_stmt(node->then);
    
    // Jump to end (skip else block)
    emit_comment("Skip else block");
    emit("    jmi %s           ; goto %s", end_label, end_label);
    
    // Else label
    emit_label(else_label);
    
    // Generate else block (if it exists)
    if (node->els) {
        emit_comment("Else block");
        gen_stmt(node->els);
    }
    
    // End label
    emit_label(end_label);
    emit_comment("End of if statement");
}

// Generate for/while statement
static void gen_for_stmt(Node *node) {
    int id = label_counter++;
    char *begin_label = format("_L_for_begin_%d", id);
    
    // Use parser-provided labels if available, otherwise generate our own
    char *end_label;
    char *continue_label;
    
    if (node->brk_label) {
        end_label = sanitize_label(node->brk_label);
    } else {
        end_label = format("_L_for_end_%d", id);
    }
    
    if (node->cont_label) {
        continue_label = sanitize_label(node->cont_label);
    } else {
        continue_label = format("_L_for_continue_%d", id);
    }
    
    emit_comment("For/While loop");
    
    // Generate initialization (for loops only, while loops have init=NULL)
    if (node->init) {
        emit_comment("Loop initialization");
        gen_stmt(node->init);
    }
    
    // Begin label - loop starts here
    emit_label(begin_label);
    
    // Generate condition check (if present)
    if (node->cond) {
        emit_comment("Loop condition check");
        gen_expr(node->cond);                           // Condition result in r0
        
        // Jump to end if condition is false (r0 == 0)
        emit("    set r31 %s       ; r31 = address of %s", end_label, end_label);
        emit("    bve r31 r0 #0    ; if (r0 == 0) goto %s", end_label);
    }
    
    // Generate loop body
    emit_comment("Loop body");
    gen_stmt(node->then);
    
    // Continue label - continue statements jump here
    emit_label(continue_label);
    
    // Generate increment (for loops only)
    if (node->inc) {
        emit_comment("Loop increment");
        gen_expr(node->inc);
    }
    
    // Jump back to beginning
    emit("    jmi %s           ; goto %s", begin_label, begin_label);
    
    // End label - break statements and failed conditions jump here
    emit_label(end_label);
    emit_comment("End of for/while loop");
    
    // Cleanup allocated labels
    if (node->brk_label) free(end_label);
    if (node->cont_label) free(continue_label);
}

// Generate do-while statement  
static void gen_do_while_stmt(Node *node) {
    int id = label_counter++;
    char *begin_label = format("_L_do_begin_%d", id);
    
    // Use parser-provided labels if available, otherwise generate our own
    char *end_label;
    char *continue_label;
    
    if (node->brk_label) {
        end_label = sanitize_label(node->brk_label);
    } else {
        end_label = format("_L_do_end_%d", id);
    }
    
    if (node->cont_label) {
        continue_label = sanitize_label(node->cont_label);
    } else {
        continue_label = format("_L_do_continue_%d", id);
    }
    
    emit_comment("Do-while loop");
    
    // Begin label - loop body starts here (executed at least once)
    emit_label(begin_label);
    
    // Generate loop body first (this is the key difference from while loops)
    emit_comment("Loop body (executes at least once)");
    gen_stmt(node->then);
    
    // Continue label - continue statements jump here
    emit_label(continue_label);
    
    // Generate condition check
    emit_comment("Do-while condition check");
    gen_expr(node->cond);                           // Condition result in r0
    
    // Jump back to beginning if condition is true (r0 != 0)
    emit("    set r31 %s       ; r31 = address of %s", begin_label, begin_label);
    emit("    bvn r31 r0 #0    ; if (r0 != 0) goto %s", begin_label);
    
    // End label - break statements jump here
    emit_label(end_label);
    emit_comment("End of do-while loop");
    
    // Cleanup allocated labels
    if (node->brk_label) free(end_label);
    if (node->cont_label) free(continue_label);
}

// Generate goto statement
static void gen_goto_stmt(Node *node) {
    emit_comment("Goto statement");
    char *label = NULL;
    
    if (node->unique_label) {
        label = sanitize_label(node->unique_label);
    } else if (node->label) {
        label = sanitize_label(node->label);
    }
    
    if (label) {
        emit("    jmi %s", label);
        free(label);
    } else {
        error_tok(node->tok, "goto statement without label");
    }
}

// Generate label statement
static void gen_label_stmt(Node *node) {
    emit_comment("Label statement");
    char *label = NULL;
    
    if (node->unique_label) {
        label = sanitize_label(node->unique_label);
    } else if (node->label) {
        label = sanitize_label(node->label);
    }
    
    if (label) {
        emit_label(label);
        free(label);
    }
    
    // Generate the statement after the label
    if (node->lhs) {
        gen_stmt(node->lhs);
    }
}

// Generate switch statement
static void gen_switch_stmt(Node *node) {
    emit_comment("Switch statement");
    
    // Evaluate switch expression once and save in r16 (saved register, won't conflict)
    gen_expr(node->cond);                               // Switch value in r0
    emit("    mov r16 r0          ; save switch value in r16");
    
    // Generate comparisons for each case
    for (Node *case_node = node->case_next; case_node; case_node = case_node->case_next) {
        emit_comment("Compare with case %ld", case_node->val);
        
        // Load case value into r28
        emit_load_const(REG_R28, (uint32_t)case_node->val);
        
        // Compare switch value (r16) with case value (r28) -> result in r29  
        emit("    tcu r29 r16 r28   ; r29 = sign(r16 - r28), 0 if equal");
        
        // Load case label address into r31 (compiler temp)
        char *case_label = sanitize_label(case_node->label);
        emit("    set r31 %s       ; r31 = address of %s", case_label, case_label);
        
        // Branch if equal (r29 == 0)
        emit("    bve r31 r29 #0   ; if (r29 == 0) goto %s", case_label);
        free(case_label);
    }
    
    // Jump to default case if provided
    if (node->default_case) {
        char *default_label = sanitize_label(node->default_case->label);
        emit("    jmi %s           ; goto default case", default_label);
        free(default_label);
    }
    
    // Jump to break label (end of switch)
    if (node->brk_label) {
        char *break_label = sanitize_label(node->brk_label);
        emit("    jmi %s           ; goto end of switch", break_label);
        free(break_label);
    }
    
    // Generate switch body
    gen_stmt(node->then);
    
    // Generate break label
    if (node->brk_label) {
        char *break_label = sanitize_label(node->brk_label);
        emit_label(break_label);
        free(break_label);
    }
}

// Generate case statement  
static void gen_case_stmt(Node *node) {
    emit_comment("Case statement");
    
    // Generate case label
    if (node->label) {
        char *case_label = sanitize_label(node->label);
        emit_label(case_label);
        free(case_label);
    }
    
    // Generate the statement after the case
    if (node->lhs) {
        gen_stmt(node->lhs);
    }
}

// Main statement generator
static void gen_stmt(Node *node) {
    if (!node) return;
    
    emit_comment("Statement: line %d", 
                 node->tok ? node->tok->line_no : 0);
    
    switch (node->kind) {
        case ND_EXPR_STMT:
            gen_expression_stmt(node);
            break;
            
        case ND_RETURN:
            gen_return_stmt(node);
            break;
            
        case ND_BLOCK:
            // Generate all statements in the block
            for (Node *stmt = node->body; stmt; stmt = stmt->next) {
                gen_stmt(stmt);
            }
            break;
            
        case ND_IF:
            gen_if_stmt(node);
            break;
            
        case ND_FOR:
            gen_for_stmt(node);
            break;
            
        case ND_DO:
            gen_do_while_stmt(node);
            break;
            
        case ND_GOTO:
            gen_goto_stmt(node);
            break;
            
        case ND_LABEL:
            gen_label_stmt(node);
            break;
            
        case ND_SWITCH:
            gen_switch_stmt(node);
            break;
            
        case ND_CASE:
            gen_case_stmt(node);
            break;
            
        default:
            error_tok(node->tok, "unsupported statement type");
    }
}

/* ================================================================
 * Function Generation
 * ================================================================ */

// Generate function prologue
static void emit_function_prologue(Obj *func) {
    emit_comment("=== Function Prologue ===");
    
    int frame_size = func->stack_size;
    if (frame_size > 0) {
        emit_comment("Save caller's frame pointer and link register");
        emit_push(REG_R30);                             // Push old fp
        emit_push(REG_LR);                              // Push return address
        
        emit_comment("Set up new frame pointer");
        emit("    mov r30 sp          ; fp = sp (points to saved lr)");
        
        emit_comment("Allocate %d bytes for locals", frame_size);
        emit_stack_alloc(frame_size);                   // Allocate space for locals
    } else {
        // No locals, but still need to save lr if we might call functions
        emit_comment("No locals, minimal prologue");
        emit_push(REG_R30);                             // Push old fp  
        emit_push(REG_LR);                              // Push return address
        emit("    mov r30 sp          ; fp = sp");
    }
}

// Generate function epilogue
static void emit_function_epilogue(Obj *func) {
    emit_comment("=== Function Epilogue ===");
    
    int frame_size = func->stack_size;
    if (frame_size > 0) {
        emit_comment("Deallocate %d bytes for locals", frame_size);
        emit_stack_free(frame_size);                     // Free local storage
    }
    
    emit_comment("Restore caller's frame pointer and return");
    emit_pop(REG_LR);                                    // Restore return address
    emit_pop(REG_R30);                                   // Restore old fp
    emit_return();
}


/* ================================================================
 * Function Generation
 * ================================================================ */

// Generate a complete function
static void gen_function(Obj *func) {
    if (!func->is_function || !func->is_definition) {
        return;
    }
    
    current_fn = func;
    label_counter = 1;
    
    emit("");
    emit_section_comment(func->name);
    emit_label(func->name);
    
    // Function prologue
    // Generate stack frame documentation
    emit_comment("Stack Frame Layout:");
    emit_comment("  [fp+8] = saved lr (return address)");
    emit_comment("  [fp+4] = saved fp (caller's frame pointer)");
    emit_comment("  [fp+0] = current fp");
    
    // Document parameters
    bool has_params = false;
    for (Obj *var = func->params; var; var = var->next) {
        if (!has_params) {
            emit_comment("  Parameters:");
            has_params = true;
        }
        emit_comment("    [fp%d] = %s (%d bytes)", var->offset, 
                    var->name ? var->name : "(unnamed)", var->ty->size);
    }
    
    // Document local variables
    bool has_locals = false;
    for (Obj *var = func->locals; var; var = var->next) {
        if (var->offset == -1) continue;  // Skip __va_area__
        if (!has_locals) {
            emit_comment("  Local variables:");
            has_locals = true;
        }
        emit_comment("    [fp%d] = %s (%d bytes)", var->offset, 
                    var->name ? var->name : "(unnamed)", var->ty->size);
    }
    
    emit_comment("Frame size: %d bytes", func->stack_size);
    
    emit_comment("=== Function Prologue ===");
    emit_comment("Save caller's frame pointer and link register");
    emit_push(REG_R30);                             // Push old fp
    emit_push(REG_LR);                              // Push return address
    emit_comment("Set up new frame pointer");
    emit("    mov r30 sp          ; fp = sp (points to saved lr)");
    
    if (func->stack_size > 0) {
        emit_comment("Allocate %d bytes for locals", func->stack_size);
        emit_stack_alloc(func->stack_size);         // Allocate space for locals
    }
    
    // Copy register parameters to local variables (Phase 3.2 implementation)
    int reg_param = 0;
    for (Obj *param = func->params; param; param = param->next) {
        if (reg_param < 8) {
            emit_comment("Copy parameter %s from r%d to stack", param->name, reg_param);
            emit_store_word(reg_param, REG_R30, param->offset);
            reg_param++;
        } else {
            // Stack parameters - handle later if needed
            error("Stack parameters not yet implemented");
        }
    }
    
    // Function body
    emit_comment("=== Function Body ===");
    gen_stmt(func->body);
    
    // Default return (in case no explicit return)
    emit_label(format("_L_return_%s", func->name));
    
    // Function epilogue
    emit_comment("=== Function Epilogue ===");
    if (func->stack_size > 0) {
        emit_comment("Deallocate %d bytes for locals", func->stack_size);
        emit_stack_free(func->stack_size);          // Free local storage
    }
    emit_comment("Restore caller's frame pointer and return");
    emit_pop(REG_LR);                               // Restore return address
    emit_pop(REG_R30);                              // Restore old fp
    emit_return();
}

/* ================================================================
 * Stack Frame Management
 * ================================================================ */

// Assign stack offsets to local variables and calculate frame size
static void assign_lvar_offsets(Obj *prog) {
    for (Obj *fn = prog; fn; fn = fn->next) {
        if (!fn->is_function)
            continue;
        
        int bottom = 0;  // Current stack offset (grows negative from fp)
        
        // Count parameters that go in registers vs stack
        int reg_params = 0;
        
        // Assign offsets to parameters (they need stack storage too)
        for (Obj *var = fn->params; var; var = var->next) {
            if (reg_params < 8) {
                // Parameter comes from register but needs stack storage
                bottom = align_to(bottom, var->align);
                var->offset = -(bottom + var->ty->size);
                bottom += var->ty->size;
                reg_params++;
            } else {
                // Parameter passed on stack (positive offset from fp)
                // TODO: Implement stack parameters later
                error("Stack parameters not yet implemented");
            }
        }
        
        // Assign offsets to local variables
        for (Obj *var = fn->locals; var; var = var->next) {
            if (var->offset != 0)  // Already assigned (shouldn't happen)
                continue;
            
            // Skip __va_area__ for now - we don't support variadic functions yet
            if (var->name && strcmp(var->name, "__va_area__") == 0) {
                var->offset = -1;  // Mark as handled but don't allocate space
                continue;
            }
            
            // Align the stack for this variable
            bottom = align_to(bottom, var->align);
            
            // Assign negative offset from frame pointer
            var->offset = -(bottom + var->ty->size);
            bottom += var->ty->size;
        }
        
        // Calculate total frame size (aligned to 4 bytes for IRRE)
        fn->stack_size = align_to(bottom, 4);
    }
}

/* ================================================================
 * Global Variable Generation
 * ================================================================ */

// Generate global variables in data section
static void gen_globals(Obj *prog) {
    bool has_globals = false;
    
    // Check if we have any global variables
    for (Obj *var = prog; var; var = var->next) {
        if (!var->is_function && !var->is_local) {
            has_globals = true;
            break;
        }
    }
    
    if (!has_globals) {
        return;  // No globals to generate
    }
    
    emit("");
    emit_section_comment("Global Variables Data Section");
    emit("%%section data");
    emit("");
    
    // Generate each global variable and string literal
    for (Obj *var = prog; var; var = var->next) {
        if (!var->is_function && !var->is_local) {
            char *clean_name = var->name;
            
            // Handle string literals with .L.. names
            if (var->name && strncmp(var->name, ".L..", 4) == 0) {
                emit_comment("String literal: %s", var->name);
                clean_name = sanitize_label(var->name);
                emit_label(clean_name);
                
                if (var->init_data) {
                    // String literal data
                    emit("    %%d \"%s\"", var->init_data);
                } else {
                    emit("    %%d \"\"            ; empty string");
                }
                
                if (clean_name != var->name) {
                    free(clean_name);
                }
            } else {
                // Regular global variable
                emit_comment("Global variable: %s (%d bytes)", var->name, var->ty->size);
                emit_label(var->name);
                
                if (var->init_data) {
                    // TODO: Handle initialized global variables properly
                    emit_comment("Initialized data not yet implemented");
                    emit("    %%d 0               ; placeholder for initialized data");
                } else {
                    // Zero-initialized global variable
                    emit("    %%d 0               ; %d-byte global variable", var->ty->size);
                }
            }
            emit("");
        }
    }
}

/* ================================================================
 * Entry Point
 * ================================================================ */

void codegen(Obj *prog, FILE *out) {
    output_file = out;
    
    // CRITICAL: Assign stack offsets before generating any code
    assign_lvar_offsets(prog);
    
    emit_section_comment("IRRE Assembly Generated by chibicc");
    emit_comment("Target: IRRE v2.0 32-bit Architecture");
    emit_comment("ABI: r0-r7 args, r8-r15 temps, r16-r27 saved");
    emit("");
    
    // Add entry point directive and startup code
    bool has_main = false;
    for (Obj *func = prog; func; func = func->next) {
        if (func->is_function && strcmp(func->name, "main") == 0) {
            has_main = true;
            break;
        }
    }
    
    if (has_main) {
        emit_comment("Program entry point");
        emit("%%entry: _start");
        emit("");
        emit_label("_start");
        emit_comment("Call main function");
        emit_call("main");
        emit_comment("Halt after main returns");
        emit("    hlt                 ; halt execution");
        emit("");
    }
    
    // Generate all functions FIRST
    for (Obj *func = prog; func; func = func->next) {
        if (func->is_function) {
            gen_function(func);
        }
    }
    
    // Generate global variables data section LAST
    // CRITICAL: Data section MUST come after ALL code for correct symbol resolution
    gen_globals(prog);
    
    emit("");
    emit_comment("End of generated code");
}