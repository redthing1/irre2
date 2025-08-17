// Simple while loop test
int main() {
    int count = 0;
    int result = 0;
    
    while (count < 3) {
        result = result + 10;
        count = count + 1;
    }
    
    return result;  // Should return 30 (3 iterations * 10)
}