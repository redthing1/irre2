; recursive_fib.asm - Recursive Fibonacci

%entry: main

; Recursive fibonacci function
; Input: r0 = n
; Output: r0 = fib(n)
fib:
    ; Set up stack frame
    sbi sp sp 20
    stw lr sp 16         ; Save return address
    stw r0 sp 12         ; Save n for later
    
    ; Check base cases (n <= 1)
    set r1 1
    tcu r2 r0 r1         ; Compare n with 1
    set ad base_case
    bvn ad r2 1          ; Branch if n <= 1
    
    ; Recursive case: fib(n-1) + fib(n-2)
    ldw r0 sp 12         ; Get n
    sbi r0 r0 1          ; n - 1
    set ad fib
    cal ad               ; Call fib(n-1)
    stw r0 sp 8          ; Save result
    
    ldw r0 sp 12         ; Get n again
    sbi r0 r0 2          ; n - 2
    set ad fib
    cal ad               ; Call fib(n-2)
    stw r0 sp 4          ; Save result
    
    ; Add the results
    ldw r1 sp 8          ; fib(n-1)
    ldw r2 sp 4          ; fib(n-2)
    add r0 r1 r2         ; fib(n-1) + fib(n-2)
    jmi done
    
base_case:
    ldw r0 sp 12         ; Return n (correct for fib(0)=0, fib(1)=1)
    
done:
    ; Clean up and return
    ldw lr sp 16
    adi sp sp 20
    ret

main:
    ; Calculate fib(6) = 8
    set r0 6
    set ad fib
    cal ad
    
    ; Store result at address 1000
    set r1 1000
    stw r0 r1 0
    
    hlt