; bit_count.asm - Count number of 1 bits in a 32-bit integer
; Demonstrates bit manipulation and logical operations

%entry: main

; Count set bits in a number
; Input: r0 = number
; Output: r0 = count of 1 bits
count_bits:
    mov r1 r0            ; r1 = number to process
    set r0 0             ; count = 0
    set r2 32            ; bits remaining
    set r3 1             ; mask for checking LSB
    
count_loop:
    ; Check if we've processed all bits
    set r5 0             ; Zero for comparison
    tcu r4 r2 r5         ; Compare remaining bits with 0
    set ad count_done
    bve ad r4 0          ; If no bits left, done
    
    ; Check if current LSB is set
    and r4 r1 r3         ; r4 = number & 1
    set ad skip_increment
    bve ad r4 0          ; If bit is 0, skip increment
    
    ; Bit is set, increment count
    adi r0 r0 1
    
skip_increment:
    ; Shift number right by 1
    set r4 0
    sbi r4 r4 1          ; r4 = -1 (for right shift)
    lsh r1 r1 r4
    
    ; One less bit to process
    sbi r2 r2 1
    jmi count_loop
    
count_done:
    ret

main:
    ; Test with number 0x2A (binary: 00101010)
    ; Expected result: 3 bits set
    set r0 42            ; 42 = 0x2A
    set ad count_bits
    cal ad
    
    ; Store result at address 4000
    set r1 4000
    stw r0 r1 0
    
    ; Test with another number 0xFF (binary: 11111111)
    ; Expected result: 8 bits set
    set r0 255           ; 255 = 0xFF
    set ad count_bits
    cal ad
    
    ; Store second result at address 4004  
    set r1 4004
    stw r0 r1 0
    
    hlt