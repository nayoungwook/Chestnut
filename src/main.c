
#include <token.h>
#include <parser.h>
#include <util.h>

int main(int arc, char *args[]){

  TokenizerContext* tc = gen_tc(L"if ( a == 3 ) { return 0; } \n var b: int = 0;\n");
  ParserContext* pc = gen_pc(tc);

  void *ast = NULL;

  while ((ast = parse(pc)) != NULL) {

  }    
  
  return 0;
}
