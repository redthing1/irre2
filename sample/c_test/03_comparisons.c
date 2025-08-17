// Comparison operations
int main() {
    int x = 15;
    int y = 10;
    
    // Test various comparisons
    int eq = (x == y);      // 0 (false)
    int ne = (x != y);      // 1 (true)
    int lt = (x < y);       // 0 (false)
    int le = (x <= y);      // 0 (false)
    
    return eq + ne + lt + le;  // Should return 1
}