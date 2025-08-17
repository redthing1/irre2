// Dereference operator test  
int main() {
    int x;
    int *ptr;
    int value;
    
    x = 42;
    ptr = &x;        // Get address of x
    value = *ptr;    // Dereference ptr to get value of x
    
    return value;    // Should return 42
}