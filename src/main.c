int printf(const char *fmt, ...);

int collatz(int n);

int main(void) {
    printf("test -> collatz(%d) = %d", 5, collatz(5));
    return 0;
}

