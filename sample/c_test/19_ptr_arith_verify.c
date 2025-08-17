// Verify pointer arithmetic scaling
int main() {
    int x;
    int *ptr;
    int *ptr_plus_2;
    int diff;
    
    ptr = &x;
    ptr_plus_2 = ptr + 2;   // Should be ptr + 8 bytes
    
    // Calculate difference (should be 8)
    diff = (int)ptr_plus_2 - (int)ptr;
    
    return diff;            // Should return 8
}