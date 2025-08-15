; math.asm - Arithmetic operations demonstration
; Shows various ALU operations and register usage

%entry: calculate

calculate:
    ; Initialize some values
    set r1 42             ; First operand
    set r2 17             ; Second operand
    
    ; Basic arithmetic
    add r3 r1 r2         ; r3 = 42 + 17 = 59
    sub r4 r1 r2         ; r4 = 42 - 17 = 25
    mul r5 r1 r2         ; r5 = 42 * 17 = 714
    div r6 r1 r2         ; r6 = 42 / 17 = 2
    mod r7 r1 r2         ; r7 = 42 % 17 = 8
    
    ; Bitwise operations
    set r8 $FF           ; Bit pattern
    and r9 r1 r8         ; r9 = r1 & 0xFF
    orr r10 r1 r8        ; r10 = r1 | 0xFF
    xor r11 r1 r8        ; r11 = r1 ^ 0xFF
    not r12 r1            ; r12 = ~r1
    
    ; Shift operations
    set r13 2             ; Shift amount
    lsh r14 r1 r13       ; r14 = r1 << 2
    ash r15 r1 r13       ; r15 = r1 >> 2 (arithmetic)
    
    ; Comparisons
    tcu r16 r1 r2        ; r16 = unsigned_compare(r1, r2)
    tcs r17 r1 r2        ; r17 = signed_compare(r1, r2)
    
    ; Conditional operation
    seq r18 r1 42        ; r18 = (r1 == 42 ? 1 : 0)
    
    ; Store results to memory
    set r19 result_area
    stw r3 r19 0         ; Store sum
    stw r4 r19 4         ; Store difference
    stw r5 r19 8         ; Store product
    stw r6 r19 12        ; Store quotient
    stw r7 r19 16        ; Store remainder
    
    ; Exit
    hlt

; Memory area for results
result_area:
    %d 0 0 0 0 0              ; 5 words = 20 bytes