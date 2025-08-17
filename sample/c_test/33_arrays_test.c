// Test array declarations and subscripting
int main() {
    int arr[3];
    arr[0] = 10;
    arr[1] = 20; 
    arr[2] = 30;
    
    int sum = 0;
    sum = sum + arr[0];
    sum = sum + arr[1]; 
    sum = sum + arr[2];
    
    return sum;  // Should return 60
}