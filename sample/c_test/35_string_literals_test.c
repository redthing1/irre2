// Test string literals in data section
int main() {
    char *hello = "Hello";
    char *world = "World";
    char *empty = "";
    
    // Simple test: return length of first string manually
    // "Hello" has 5 characters
    int len = 0;
    char *p = hello;
    while(*p) {
        len = len + 1;
        p = p + 1;
    }
    
    return len;  // Should return 5
}