// Test comma operator
int x;

int main() {
    // Test comma operator: (x = 10, x + 5)
    // Should evaluate x = 10 (side effect), then return x + 5 = 15
    return (x = 10, x + 5);  // Should return 15
}