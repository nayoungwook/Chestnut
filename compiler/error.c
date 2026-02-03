#include <error.h>

void panic(char *msg, struct TokenizerContext *tc) {
    printf("PANIC!\n");
    printf("%s\n", msg);
    printf("%d | %s\n", tc->line_num, tc->cur_ch);
    exit(1);
}
