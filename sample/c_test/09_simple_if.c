// Simple if test without else
int main() {
    int x;
    x = 5;
    
    if (x > 3) {
        return 42;  // Should execute and return 42
    }
    
    return 99;  // Should not reach here
}