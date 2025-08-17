// Do-while test with false condition (should execute once)
int main() {
    int result;
    
    result = 0;
    
    do {
        result = 42;  // Should execute once even though condition is false
    } while (0);      // Condition is always false
    
    return result;    // Should return 42
}