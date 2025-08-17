// Test current compiler features
int main() {
    // Basic variables and arithmetic - WORKS
    int a = 10;
    int b = 20;
    int c = a + b;
    
    // Comparisons - WORKS  
    int result;
    if (a < b) {
        result = 100;
    } else {
        result = 200;
    }
    
    // While loop - NOT IMPLEMENTED
    // while (a < 15) {
    //     a = a + 1;
    // }
    
    // For loop - NOT IMPLEMENTED
    // for (int i = 0; i < 5; i++) {
    //     result = result + 1;
    // }
    
    // Pointers - NOT IMPLEMENTED
    // int *ptr = &a;
    // int val = *ptr;
    
    // Function calls - NOT IMPLEMENTED
    // result = add(10, 20);
    
    // Arrays - NOT IMPLEMENTED
    // int arr[10];
    // arr[0] = 42;
    
    return result;
}