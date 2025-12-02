#include <token.h>
#include <stdio.h>
#include <stdlib.h>

int main(int arc, char *args[]){

  TokenizerContext* tc = gen_tc(L"var a: int += 0.04.2;");
  
  Token* tok = NULL;

  while(1){
    tok = pull(tc);

    wprintf(L"%ls %d\n", tok->str, tok->type);
    
    if(tok->type == TokEOF)
      break;
    free(tok);
  }
  
  return 0;
}
