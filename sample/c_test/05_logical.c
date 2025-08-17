// Logical operations with short-circuiting
int main() {
    int a = 5;
    int b = 0;
    int c = 10;
    
    // Test logical AND with short-circuit
    int and1 = (a > 0) && (c > 5);     // 1 && 1 = 1
    int and2 = (b > 0) && (c > 5);     // 0 && 1 = 0 (short-circuit)
    
    // Test logical OR with short-circuit  
    int or1 = (a > 0) || (b > 0);      // 1 || 0 = 1 (short-circuit)
    int or2 = (b > 0) || (c > 0);      // 0 || 1 = 1
    
    // Test logical NOT
    int not1 = !(a > 0);               // !1 = 0
    int not2 = !(b > 0);               // !0 = 1
    
    return and1 + and2 + or1 + or2 + not1 + not2;  // 1+0+1+1+0+1 = 4
}