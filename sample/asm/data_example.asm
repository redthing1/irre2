; data_example.asm - Comprehensive example of code and data interaction
; Demonstrates string processing, array operations, and mixed data types

%entry: main

main:
    ; Part 1: String length calculation
    set r0 greeting
    set r1 0                ; character counter
    
count_loop:
    ldb r2 r0 0            ; load byte from string
    seq r4 r2 0            ; check if byte == 0 (null terminator)
    set ad count_done
    bve ad r4 1            ; if byte == 0, we're done
    
    adi r1 r1 1            ; increment counter
    adi r0 r0 1            ; move to next byte
    jmi count_loop
    
count_done:
    ; Store string length
    set r5 str_length
    stw r1 r5 0
    
    ; Part 2: Array sum calculation
    set r6 numbers
    set r7 array_size
    ldw r8 r7 0            ; load array size
    set r9 0               ; sum accumulator
    set r10 0              ; index counter
    
sum_loop:
    ; Check if index >= size
    tcu r11 r10 r8
    set ad sum_done
    bve ad r11 0           ; if index >= size, done
    
    ; Calculate element address and load value
    set r12 4              ; word size
    mul r13 r10 r12        ; index * 4
    add r14 r6 r13         ; base + offset
    ldw r15 r14 0          ; load array element
    add r9 r9 r15          ; add to sum
    
    adi r10 r10 1          ; increment index
    jmi sum_loop
    
sum_done:
    ; Store array sum
    set r20 array_sum
    stw r9 r20 0
    
    ; Part 3: Build result message
    set r21 result_area
    
    ; Copy message template
    set r22 msg_template
    set r23 0              ; byte index
    
copy_loop:
    ldb r24 r22 0          ; load template byte
    stb r24 r21 0          ; store to result
    seq r26 r24 0          ; check for null terminator
    set ad copy_done
    bve ad r26 1
    
    adi r22 r22 1          ; next source byte
    adi r21 r21 1          ; next dest byte
    jmi copy_loop
    
copy_done:
    hlt

; String data with null terminator
greeting:
    %d "Hello, IRRE!" 0

; Numeric array data
numbers:
    %d 10 25 37 42 56      ; 5 numbers

array_size:
    %d 5                   ; size of numbers array

; Template message
msg_template:
    %d "Processing complete" 0

; Results storage area
str_length:
    %d 0                   ; will store string length

array_sum:
    %d 0                   ; will store array sum  

result_area:
    %d 0 0 0 0 0 0 0 0     ; space for result message (32 bytes)