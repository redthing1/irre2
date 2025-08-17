// Do-while loop test (executes at least once)
int main() {
    int count;
    int result;
    
    count = 0;
    result = 0;
    
    do {
        result = result + 5;
        count = count + 1;
    } while (count < 3);
    
    return result;  // Should return 15 (3 iterations * 5)
}