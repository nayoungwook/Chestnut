#include <stdio.h>
#include <stdlib.h>

#include <token.h>
#include <util.h>

int main(int arc, char *args[]){

  TokenizerContext* tc = gen_tc(L"if(a == 3){return 0;}");

  Token* tok = NULL;

  while(true){
    tok = pull(tc);

    wprintf(L"pull : %ls %d\n", tok->str, tok->type);

    if(tok->type == TokEOF)
      break;
  }

  struct HTable *table = gen_htable();
  int a = 5, b = 7;

  HT_insert(table, L"a", &a);
  HT_insert(table, L"b", &b);
  
  wprintf(L"a : %d\n", *((int*) HT_find(table, L"a")->ptr));
  wprintf(L"b : %d\n", *((int*) HT_find(table, L"b")->ptr));
  
  return 0;
}
