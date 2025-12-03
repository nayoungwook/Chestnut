#include <token.h>
#include <stdio.h>
#include <stdlib.h>

int main(int arc, char *args[]){

  TokenizerContext* tc = gen_tc(L"if(a == 3){return 0;}");
  
  Token* tok = NULL;
  
  while(true){
    tok = pull(tc);

    wprintf(L"pull : %ls %d\n", tok->str, tok->type);

    if(tok->type == TokEOF)
      break;
  }

  return 0;
}
