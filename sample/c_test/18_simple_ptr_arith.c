// Simple pointer arithmetic test without arrays
int main() {
    int x;
    int y; 
    int *ptr;
    int *ptr2;
    
    x = 10;
    y = 20;
    
    ptr = &x;           // Point to x
    ptr2 = ptr + 1;     // Move pointer by 1 int (should point past x)
    
    return (int)ptr2;   // Return the address for verification
}