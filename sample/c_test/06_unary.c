// Unary operations
int main() {
    int positive = 42;
    int negative = -positive;    // Should be -42
    
    int bits = 0x0F;            // 00001111
    int inverted = ~bits;       // 11110000 (bitwise NOT)
    
    int truth = 1;
    int falsity = !truth;       // Logical NOT: !1 = 0
    
    return negative + falsity;  // -42 + 0 = -42
}