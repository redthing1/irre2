// Test if/else statements
int main() {
    int x;
    int result;
    
    x = 5;
    
    if (x > 3) {
        result = 100;  // Should execute this
    } else {
        result = 200;  // Should not execute this
    }
    
    return result;  // Should return 100
}