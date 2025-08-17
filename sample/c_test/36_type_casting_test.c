// Test type casting between different integer types  
int main() {
    int i = 300;
    char c = (char)i;       // 300 -> 44 (truncation)
    int j = (int)c;         // 44 -> 44 (sign extension) 
    
    short s = (short)i;     // 300 -> 300 (no change)
    int k = (int)s;         // 300 -> 300 (sign extension)
    
    // Test negative values
    int neg = -100;
    char neg_c = (char)neg; // -100 -> 156 (as unsigned byte)
    int neg_back = (int)neg_c; // 156 -> 156 (zero extension for unsigned)
    
    return j + k;           // Should return 44 + 300 = 344
}