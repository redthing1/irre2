; simple.asm - Basic instruction test
; Simple program to test assembler/disassembler

%entry: start

start:
    ; Basic register operations
    set r1 42
    set r2 17
    add r3 r1 r2
    mov r4 r3
    not r5 r4
    
    ; Branch test
    seq r7 r3 59
    set r6 success
    bve r6 r7 1
    hlt

success:
    set r8 255
    set r9 240
    hlt