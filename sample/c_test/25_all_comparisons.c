// Test all comparison operators
int main() {
    int a;
    int b;
    a = 10;
    b = 5;
    
    // Test > (greater than)
    if (a > b) {
        return 1;  // Should return this (10 > 5 is true)
    }
    
    // Test >= (greater than or equal)
    if (a >= b) {
        return 2;  // Should not reach this due to return above
    }
    
    return 0;  // Should not reach this
}