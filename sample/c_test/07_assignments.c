// Assignment operations
int main() {
    int x;
    int y;
    
    x = 10;              // Simple assignment
    y = x;               // Copy assignment
    x = y + 5;           // Assignment with expression
    y = (x = 20);        // Nested assignment (x=20, y=20)
    
    return x + y;        // Should return 40
}