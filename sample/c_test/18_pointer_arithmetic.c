// Basic pointer arithmetic test
int main() {
    int arr[3];
    int *ptr;
    int *ptr2;
    
    arr[0] = 10;
    arr[1] = 20; 
    arr[2] = 30;
    
    ptr = &arr[0];      // Point to first element
    ptr2 = ptr + 1;     // Should point to second element
    
    return *ptr2;       // Should return 20 (second element)
}