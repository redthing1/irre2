; prime_check.asm - Check if a number is prime
; Demonstrates mathematical algorithms and division/modulo operations

%entry: main

; Check if number is prime  
; Input: r0 = number to test
; Output: r0 = 1 if prime, 0 if not prime
is_prime:
    ; Handle n <= 1 (not prime)
    set r1 1
    tcu r2 r0 r1         ; r2 = sign(n - 1)
    set ad not_prime
    bvn ad r2 1          ; If n <= 1, not prime
    
    ; Handle n == 2 (prime)
    set r1 2
    tcu r2 r0 r1         ; r2 = sign(n - 2)  
    set ad is_prime_yes
    bve ad r2 0          ; If n == 2, it's prime
    
    ; Handle even numbers > 2 (not prime)
    set r1 2
    mod r2 r0 r1         ; r2 = n % 2
    set ad not_prime  
    bve ad r2 0          ; If even, not prime
    
    ; Test odd divisors starting from 3
    set r1 3             ; divisor = 3
    
test_divisor:
    ; Check if divisor^2 > n (if so, n is prime)
    mul r2 r1 r1         ; r2 = divisor^2
    tcu r3 r2 r0         ; r3 = sign(divisor^2 - n)
    set ad is_prime_yes
    bve ad r3 1          ; If divisor^2 > n, it's prime
    
    ; Check if n is divisible by divisor
    mod r2 r0 r1         ; r2 = n % divisor
    set ad not_prime
    bve ad r2 0          ; If divisible, not prime
    
    ; Try next odd divisor
    adi r1 r1 2          ; divisor += 2
    jmi test_divisor
    
is_prime_yes:
    set r0 1
    ret
    
not_prime:
    set r0 0  
    ret

main:
    ; Test multiple numbers and store results
    set r10 5000         ; Base address for results (use r10 to avoid conflicts)
    
    ; Test 17 (should be prime = 1)
    set r0 17
    set ad is_prime
    cal ad
    stw r0 r10 0         ; Store at 5000
    
    ; Test 15 (should be not prime = 0) 
    set r0 15
    set ad is_prime
    cal ad
    stw r0 r10 4         ; Store at 5004
    
    ; Test 2 (should be prime = 1)
    set r0 2
    set ad is_prime
    cal ad  
    stw r0 r10 8         ; Store at 5008
    
    ; Test 1 (should be not prime = 0)
    set r0 1
    set ad is_prime
    cal ad
    stw r0 r10 12        ; Store at 5012
    
    hlt