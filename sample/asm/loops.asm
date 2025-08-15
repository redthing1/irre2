; loops.asm - Control flow and looping demonstration
; Shows branches, function calls, and loop structures

%entry: main

main:
    ; Test iterative factorial
    set r1 5              ; Calculate factorial of 5
    set r10 iterative_factorial
    cal r10
    
    ; Store factorial result
    set r3 result
    stw r2 r3 0
    
    ; Test counting loop
    set r1 3              ; Count to 3
    set r10 count_to_n
    cal r10
    
    ; Store count result
    set r3 result
    stw r2 r3 4           ; Store at result+4
    
    ; Test simple loop with break condition
    set r1 0              ; Start counter
    set r4 10             ; End value
    
simple_loop:
    ; Check if done
    tcu r5 r1 r4          ; r5 = sign(r1 - r4)
    set ad loop_done
    bve ad r5 0           ; If r1 == r4, exit loop
    
    ; Do some work (square the counter)
    mul r6 r1 r1          ; r6 = r1 * r1
    
    ; Increment counter
    adi r1 r1 1
    jmi simple_loop
    
loop_done:
    ; Exit
    hlt

; Alternative iterative factorial using loops
iterative_factorial:
    mov r20 lr            ; Save return address
    set r2 1              ; Initialize result
    set r3 1              ; Counter
    
loop_start:
    ; Check if counter > input
    tcu r4 r3 r1          ; r4 = sign(r3 - r1)
    set ad loop_end
    bve ad r4 1           ; If r4 == 1 (counter > input), exit loop
    
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
    tcu r3 r2 r1          ; r3 = sign(r2 - r1)
    set ad count_done
    bve ad r3 0           ; If r3 == 0 (counter == n), we're done
    
    ; Increment counter
    adi r2 r2 1
    jmi count_loop
    
count_done:
    mov lr r20
    ret

; Data storage
result:
    %d 0