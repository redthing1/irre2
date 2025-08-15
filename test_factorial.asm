%entry: main

main:
    set r0 5
    set lr main_return
    jmi factorial
main_return:
    hlt

factorial:
    set r1 1
    set r2 1
    
factorial_loop:
    tcu r3 r2 r0
    set ad factorial_done
    bve ad r3 1
    
    mul r1 r1 r2
    adi r2 r2 1
    jmi factorial_loop
    
factorial_done:
    mov r0 r1
    jmp lr
