// Comprehensive control flow test combining all constructs
int main() {
    int total;
    int i;
    int j;
    
    total = 0;
    
    // For loop with nested if
    for (i = 1; i <= 3; i = i + 1) {
        if (i < 3) {
            total = total + i;        // 1 + 2 = 3
        } else {
            total = total + 10;       // 10
        }
    }
    // total should be 13 after for loop
    
    // While loop
    j = 0;
    while (j < 2) {
        total = total + 5;            // 5 + 5 = 10
        j = j + 1;
    }
    // total should be 23 after while loop
    
    // Do-while (always executes once)
    do {
        total = total + 7;            // 7
    } while (0);                      // false condition
    // total should be 30
    
    return total;                     // Should return 30
}