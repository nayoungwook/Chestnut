
#include <token.h>
#include <parser.h>
#include <util.h>

int main(int arc, char *args[]){

  TokenizerContext* tc = gen_tc(L"func main(): void {}");
  ParserContext* pc = gen_pc(tc);

  void *ast = NULL;
  
  while ((ast = parse(pc)) != NULL) {

  }    
  
  return 0;
}
