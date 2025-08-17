// Simple while loop test without initialization
int main() {
    int count;
    int result;
    
    count = 0;
    result = 0;
    
    while (count < 3) {
        result = result + 10;
        count = count + 1;
    }
    
    return result;  // Should return 30 (3 iterations * 10)
}