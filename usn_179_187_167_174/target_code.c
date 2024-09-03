#include <stdio.h>

int add(int a, int b) {
    return a + b;
}

int max(int a, int b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

int mul(int a,int b)
{
    return a*b;
}

void print_message(int x) {
    if (x > 0) {
        printf("Positive\n");
        return;
    } else if (x < 0) {
        printf("Negative\n");
        return;
    }
    printf("Zero\n");
}

int main() {
    int sum = add(3, 4);
    int maximum = max(10, 20);
    int m=mul(2,3);
    print_message(sum);
    print_message(-5);
    print_message(0);
    print_message(m);
    return 0;
}


