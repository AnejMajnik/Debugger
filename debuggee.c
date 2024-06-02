#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Child> Begin\n");

    for (int i = 0; i < 10; i++) {
        printf("i = %d\n", i);
        sleep(1);
    }

    printf("Child> End\n");

    return 0;
}
