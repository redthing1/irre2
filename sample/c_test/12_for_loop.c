// Simple for loop test
int main() {
    int sum;
    int i;
    
    sum = 0;
    
    for (i = 1; i <= 4; i = i + 1) {
        sum = sum + i;
    }
    
    return sum;  // Should return 10 (1+2+3+4)
}