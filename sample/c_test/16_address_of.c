// Address-of operator test
int main() {
    int x;
    int *ptr;
    
    x = 42;
    ptr = &x;     // Get address of x
    
    // We can't dereference yet, so just return the address
    // The address should be a valid stack address
    return (int)ptr;  // Return the pointer value as int
}