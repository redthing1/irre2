// Test break and continue statements in loops
int main() {
    int sum = 0;
    
    // Test continue: skip even numbers
    for(int i = 0; i < 10; i++) {
        if(i % 2 == 0) continue;
        sum = sum + i;  // Sum odd numbers: 1+3+5+7+9 = 25
    }
    
    // Test break: exit loop early
    for(int j = 0; j < 100; j++) {
        if(j == 3) break;
        sum = sum + j;  // Add 0+1+2 = 3
    }
    
    // Test nested loops with break
    for(int k = 0; k < 5; k++) {
        for(int l = 0; l < 5; l++) {
            if(l == 2) break;  // Inner break only
            sum = sum + 1;     // Add 2*5 = 10 times
        }
    }
    
    return sum;  // Should return 25 + 3 + 10 = 38
}