
#include <token.h>
#include <parser.h>
#include <util.h>
#include <type.h>
#include <ir.h>

#define DEBUG

#include <assert.h>
#include <locale.h>

int main(int arc, char *args[]){

  setlocale(LC_ALL, "");  

  // front end  
  struct ParserContext *pc = gen_pc();
  struct TypeCheckContext *tcc = gen_tcc();

  compile_file(pc, gen_tc(read_file("test.chest")), tcc);
  
  resolve_tcq(pc, tcc);

  // back end
  struct IRContext *irc = gen_irc();
  
  init_irc(irc, NULL);

  gen_ir(irc, pc);  
  
  //  print_bytes(irc);
  
  return 0;
}
