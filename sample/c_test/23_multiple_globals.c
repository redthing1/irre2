// Multiple global variables test
int global_a;
int global_b;

void set_globals() {
    global_a = 10;
    global_b = 20;
}

int main() {
    set_globals();
    return global_a + global_b;  // Should return 30
}