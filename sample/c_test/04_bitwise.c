// Bitwise operations
int main() {
    int a = 0xAA;    // 10101010
    int b = 0x55;    // 01010101
    
    int and_result = a & b;   // 00000000 = 0
    int or_result = a | b;    // 11111111 = 255
    int xor_result = a ^ b;   // 11111111 = 255
    int not_result = ~a;      // 01010101 (inverted)
    
    return and_result + or_result + xor_result;  // 0 + 255 + 255 = 510
}