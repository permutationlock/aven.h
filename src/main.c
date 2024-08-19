int printf(const char *fmt, ...);

int collatz(int n);

int main(void) {
    printf("Hello, World!\n");
    printf("collatz(%d) = %d", 5, collatz(5));
    return 0;
}

