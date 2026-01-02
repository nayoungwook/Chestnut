
#include <token.h>
#include <parser.h>
#include <util.h>

int main(int arc, char *args[]){

  TokenizerContext* tc = gen_tc(L"class Foo { func foo(): void {} var a: int = 0; }");
  ParserContext* pc = gen_pc(tc);

  void *ast = NULL;

  while ((ast = parse(pc, false)) != NULL) {

  }
  
  return 0;
}
