// While loop countdown test
int main() {
    int count;
    int sum;
    
    count = 5;
    sum = 0;
    
    while (count > 0) {
        sum = sum + count;
        count = count - 1;
    }
    
    return sum;  // Should return 15 (5+4+3+2+1)
}