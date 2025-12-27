
#include <token.h>
#include <parser.h>
#include <util.h>

int main(int arc, char *args[]){

  TokenizerContext* tc = gen_tc(L"var a: int =0; var b: float = 3 + 5 * 2, c: int;");
  ParserContext* pc = gen_pc(tc);

  void *ast = NULL;
  
  while ((ast = parse(pc)) != NULL) {

  }    
  
  return 0;
}
