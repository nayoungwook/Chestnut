
#include <token.h>
#include <parser.h>
#include <util.h>

int main(int arc, char *args[]){

  TokenizerContext* tc = gen_tc(L"func main(): void { for(var i:int =0; i<10; i+1){} }");
  ParserContext* pc = gen_pc(tc);

  void *ast = NULL;
  
  while ((ast = parse(pc, false)) != NULL) {

  }    
  
  return 0;
}
