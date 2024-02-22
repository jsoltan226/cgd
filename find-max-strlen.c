#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) 
{
    unsigned int max_strlen = 0;
    for (int i = 1; i < argc; i++) {
        unsigned int current_len = strlen(argv[i]);
        max_strlen = current_len > max_strlen ? current_len : max_strlen;
    }
    printf("%u\n", max_strlen);
    return 0;
}
