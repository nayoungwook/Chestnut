#include <stdio.h>
#include <stdlib.h>

#include <token.h>
#include <util.h>

int main(int arc, char *args[]){

  TokenizerContext* tc = gen_tc(L"if ( a == 3 ) { return 0; } \n var b: int = 0;\n");

  Token* tok = NULL;

  while(true){
    tok = pull(tc);

    wprintf(L"pull : %ls %d\n", tok->str, tok->type);

    if(tok->type == TokEOF)
      break;
  }
  
  return 0;
}
