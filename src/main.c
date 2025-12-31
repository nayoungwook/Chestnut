
#include <token.h>
#include <parser.h>
#include <util.h>

int main(int arc, char *args[]){

  Queue* queue = gen_queue();
  int a = 5, b = 3, c = 7, d = 10;

  
  q_push(queue, &a);
  q_push(queue, &b);
  q_push(queue, &c);
  q_push(queue, &d);
  
  printf("%d %d\n", *((int*) q_pop(queue)), queue->size);
  printf("%d %d\n", *((int*) q_pop(queue)), queue->size);
  printf("%d %d\n", *((int*) q_pop(queue)), queue->size);
  printf("%d %d\n", *((int*) q_pop(queue)), queue->size);

  /*
  TokenizerContext* tc = gen_tc(L"class Foo { func foo(): void {} var a: int = 0; }");
  ParserContext* pc = gen_pc(tc);

  void *ast = NULL;

  while ((ast = parse(pc, false)) != NULL) {

  }
  */
  
  return 0;
}
