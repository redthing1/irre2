; hello.asm - Simple hello world program for IRRE v2.0
; Demonstrates basic system calls and string output

%entry: main

; Main entry point
main:
    ; Load string address into r1
    set r1 hello_str
    
    ; Set up system call for string output
    set r2 1              ; stdout device
    set r3 hello_len      ; string length
    
    ; Send string to output device
    snd r1 r2 r3         ; device_send(string_addr, device, length)
    
    ; Exit program
    hlt                   ; halt execution

; String data
hello_str:
    %d "Hello, IRRE v2.0!\n"

hello_len:
    %d 18               ; length of hello string