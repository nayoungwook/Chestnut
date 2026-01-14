
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
  ParserContext *pc = gen_pc();
  TypeCheckContext *tcc = gen_tcc();

  compile_file(pc, gen_tc( read_file("test.chest") ), tcc);

  /*
  compile_file(pc, gen_tc(L"class Bar {\n"
                          L"    var b: int = 0;\n"
                          L"    var foo: Foo;\n"
                          L"    func bar(): void {\n"
                          L"        var foo: Foo;\n"
                          L"        foo.bar.foo.a = foo.foo();\n"
                          L"        var bar: Bar;\n"
                          L"        bar.b = foo.foo2();\n"

                          L"    }\n"
                          L"}\n"
                          ), tcc);
  */
  
  resolve_tcq(pc, tcc);

  // back end
  IRContext *irc = gen_irc();

  init_irc(irc, NULL);

  gen_metadata(irc, pc);
  
  //  print_bytes(irc);
  
  return 0;
}
