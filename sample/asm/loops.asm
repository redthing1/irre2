; loops.asm - Control flow and looping demonstration
; Shows branches, function calls, and loop structures

%entry: main

main:
    ; Initialize counter
    set r1 10             ; Loop counter
    set r2 0              ; Accumulator
    
    ; Call factorial function
    set r10 factorial
    cal r10
    
    ; Store result
    set r3 result
    stw r2 r3 0
    
    ; Exit
    hlt

; Factorial function: calculates factorial of r1, result in r2
factorial:
    ; Save return address
    mov r20 lr
    
    ; Base case: if r1 <= 1, return 1
    set r3 1
    tcu r4 r1 r3         ; Compare r1 with 1
    bif r4 factorial_base 2  ; If r1 <= 1, go to base case
    
    ; Recursive case: r2 = r1 * factorial(r1-1)
    mov r21 r1            ; Save current value
    sub r1 r1 r3         ; r1 = r1 - 1
    set r10 factorial
    cal r10               ; Recursive call
    mul r2 r21 r2         ; r2 = saved_value * factorial_result
    jmi cleanup
    
factorial_base:
    set r2 1              ; Base case result
    
cleanup:
    ; Restore and return
    mov lr r20
    ret

; Alternative iterative factorial using loops
iterative_factorial:
    mov r20 lr            ; Save return address
    set r2 1              ; Initialize result
    set r3 1              ; Counter
    
loop_start:
    ; Check if counter > input
    tcu r4 r3 r1
    bif r4 loop_end 1     ; If counter > input, exit loop
    
    ; Multiply result by counter
    mul r2 r2 r3
    
    ; Increment counter
    adi r3 r3 1
    jmi loop_start        ; Jump back to loop start
    
loop_end:
    mov lr r20            ; Restore return address
    ret

; Counting loop example
count_to_n:
    mov r20 lr            ; Save return address
    set r2 0              ; Counter
    
count_loop:
    ; Check if we've reached n
    tcu r3 r2 r1
    bif r3 count_done 0   ; If counter == n, we're done
    
    ; Increment counter
    adi r2 r2 1
    jmi count_loop
    
count_done:
    mov lr r20
    ret

; Data storage
result:
    %d 0