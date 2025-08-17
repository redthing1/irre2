// Test increment and decrement operators
int main() {
    int i = 5;
    int a = ++i;    // Pre-increment: i becomes 6, a = 6
    int b = i++;    // Post-increment: b = 6, i becomes 7  
    int c = --i;    // Pre-decrement: i becomes 6, c = 6
    int d = i--;    // Post-decrement: d = 6, i becomes 5
    
    // Should return: 6 + 6 + 6 + 6 + 5 = 29
    return a + b + c + d + i;
}