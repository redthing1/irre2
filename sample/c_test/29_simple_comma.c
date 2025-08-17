// Simpler comma test without global variables
int main() {
    int a;
    int b;
    a = 10;
    b = 5;
    
    // Simple comma: (a, b) should return b
    return (a, b);  // Should return 5
}