#include <error.h>

void panic(char *msg, struct TokenizerContext *tc) {
	printf("PANIC!\n");
	printf("%d | %s %s\n", tc->line_num, msg, tc->cur_ch);
	exit(1);
}
