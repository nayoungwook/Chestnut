#include <error.h>

void panic(wchar_t *msg, TokenizerContext* tc) {
  wprintf(L"PANIC!\n");
  wprintf(L"%d | %ls %ls\n", tc->line_num, msg, tc->cur_ch);
  exit(1);
}
