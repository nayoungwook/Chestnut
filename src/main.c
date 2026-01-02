
#include <token.h>
#include <parser.h>
#include <util.h>
#include <assert.h>

#define DEBUG

#ifdef DEBUG
static void print_var_metadata(VarData *vd, int indent);
static void print_func_metadata(FuncData *fd, int indent);
static void print_class_metadata(ClassData *cd, int indent);
static void print_metadata(ParserContext *pc, int indent);

static void print_class_metadata(ClassData *cd, int indent) {
  ClassAST *class_ast = cd->node->ast;
  assert(class_ast != NULL && cd->node->type == AST_Class);

  int i;

  for (i = 0; i < indent; i++) {
    wprintf(L"    ");
  }    
  
  wprintf(L"Class : %S(%d)\n", class_ast->name_tok->str, cd->id);
  
  for (i = 0; i < HTABLE_BUFF; i++) {
    DataNode* node = cd->member_funcs->bucket[i];    
    while (node != NULL) {
      print_func_metadata((FuncData *)node->ptr, indent + 1);
      
      node = node->next;
    }
  }

  for (i = 0; i < HTABLE_BUFF; i++) {
    DataNode* node = cd->member_vars->bucket[i];    
    while (node != NULL) {
      print_var_metadata((VarData *)node->ptr, indent + 1);
      
      node = node->next;
    }      
  }      
}

static void print_var_metadata(VarData *vd, int indent) {

  int i;
  for (i = 0; i < indent; i++) {
    wprintf(L"    ");
  }

  VarDeclAST *var_ast = vd->node->ast;
  assert(var_ast != NULL);
    
  wprintf(L"Variable : %S(%d)\n", var_ast->var_name_tok->str, vd->id);

}  

static void print_func_metadata(FuncData *fd, int indent) {
  FuncDeclAST *func_ast = fd->node->ast;
  assert(func_ast != NULL && fd->node->type == AST_FunctionDeclaration);
  
  int i;
  for (i = 0; i < indent; i++) {
    wprintf(L"    ");
  }

  wprintf(L"Function : %S(%d)\n", func_ast->func_name_tok->str, fd->id);
}  

static void print_metadata(ParserContext* pc, int indent) {
  int i;

  for (i = 0; i < pc->class_data_cnt; i++) {
    ClassData *cd = pc->class_data[i];

    print_class_metadata(cd, indent);
  }

  for (i = 0; i < pc->func_data_cnt; i++) {
    FuncData* fd = pc->func_data[i];

    print_func_metadata(fd, indent);
  }  
}
#endif

int main(int arc, char *args[]){

  ParserContext* pc = gen_pc();
  compile_file(pc,
               gen_tc(L"class Foo { func foo(): void {} var a: int = 0; } func main(): void {}"));
  compile_file(pc,
               gen_tc(L"class Bar { func bar(): void {var foo: Foo;} var b: int = 0; }"));

#ifdef DEBUG
  print_metadata(pc, 0);
#endif
  
  return 0;
}
