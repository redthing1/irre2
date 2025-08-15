; array_sum.asm - Calculate sum of array elements
; Demonstrates memory operations and address arithmetic

%entry: main

; Calculate sum of array
; Input: r0 = array address, r1 = array length
; Output: r0 = sum
array_sum:
    mov r2 r0            ; r2 = array address
    mov r3 r1            ; r3 = length
    set r0 0             ; sum = 0
    set r1 0             ; index = 0
    
sum_loop:
    ; Check if we've processed all elements
    tcu r4 r1 r3         ; Compare index with length
    set ad sum_done
    bve ad r4 0          ; If index >= length, we're done
    
    ; Calculate address of current element
    set r6 4             ; Word size
    mul r4 r1 r6         ; r4 = index * 4 (word size)
    add r4 r2 r4         ; r4 = base + (index * 4)
    
    ; Load current element and add to sum
    ldw r5 r4 0          ; Load array[index]
    add r0 r0 r5         ; sum += array[index]
    
    ; Move to next element
    adi r1 r1 1          ; index++
    jmi sum_loop
    
sum_done:
    ret

main:
    ; Set up test array: [10, 20, 30, 40, 50]
    ; Expected sum: 150
    set r0 2000          ; Array base address
    
    ; Store test data
    set r1 10
    stw r1 r0 0          ; array[0] = 10
    set r1 20  
    stw r1 r0 4          ; array[1] = 20
    set r1 30
    stw r1 r0 8          ; array[2] = 30
    set r1 40
    stw r1 r0 12         ; array[3] = 40
    set r1 50
    stw r1 r0 16         ; array[4] = 50
    
    ; Call array_sum
    set r0 2000          ; Array address
    set r1 5             ; Array length
    set ad array_sum
    cal ad               ; Result in r0
    
    ; Store result at address 3000
    set r1 3000
    stw r0 r1 0
    
    hlt