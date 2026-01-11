#include <ir.h>

IRContext *gen_irc() {
  IRContext *irc = (IRContext *)S_malloc(sizeof(IRContext));

  irc->node = NULL;
  irc->byte_cnt = 0;
  irc->byte_size = 0;
  
  return irc;
}

void init_irc(IRContext* irc, Node *node) {
  irc->node = node;
  irc->byte_cnt = 0;
  
  irc->byte_size = BYTE_CHUNK;
  irc->bytes = (byte *)S_malloc(sizeof(byte) * BYTE_CHUNK); // 4kb
}  

void emit_byte(IRContext *irc, byte _b) {
  irc->bytes[irc->byte_cnt++] = _b;

  if (irc->byte_cnt >= irc->byte_size) {
    irc->byte_size *= 2;    
    irc->bytes =
        (byte *)S_realloc(irc->bytes, sizeof(byte) * irc->byte_size);
  }
}

static void emit_str(IRContext *irc, wchar_t *str) {
  wchar_t *ch = str;

  // wchar_t is 2byte. so we have to separate it.
  // [0xFF 0xAA] [0xCD 0xEF] ... [0xDF 0xER]
  
  while (*ch != L'\0') {
    emit_byte(irc, ((*ch >> 8) & 0xFF));
    emit_byte(irc, ((*ch) & 0xFF));

    ch++;
  }

  // emit null character  
  emit_byte(irc, 0x00);
  emit_byte(irc, 0x00);  
}

// uint will be stored as little endian.
static void emit_uint(IRContext *irc, unsigned ui) {
  int i;
  for (i = 0; i < sizeof(unsigned); i++) {
    emit_byte(irc, (ui & 0xFF));
    ui >>= 4;
  }    
}

static void gen_func_metadata(IRContext *irc, ParserContext *pc, FuncData *fd) {
  FuncDeclAST *func_ast = fd->node->ast;
  assert(func_ast != NULL && fd->node->type == AST_FunctionDeclaration);

  emit_byte(irc, META_FUNC);

  emit_uint(irc, fd->id);
  emit_str(irc, func_ast->func_name_tok->str);  
}

static void gen_var_metadata(IRContext *irc, ParserContext *pc, VarData *vd) {
  VarDeclAST *var_ast = vd->node->ast;
  assert(var_ast != NULL);

  emit_byte(irc, META_VAR);

  emit_uint(irc, vd->id);
  emit_str(irc, var_ast->var_name_tok->str);
}  

static void gen_class_metadata(IRContext *irc, ParserContext *pc, ClassData *cd) {
  ClassAST *class_ast = cd->node->ast;
  assert(class_ast != NULL && cd->node->type == AST_Class);

  emit_byte(irc, META_CLASS);

  emit_uint(irc, cd->id);
  emit_str(irc, class_ast->name_tok->str);

  int i;
  for (i = 0; i < HTABLE_BUFF; i++) {
    DataNode* node = cd->member_funcs->bucket[i];    
    while (node != NULL) {
      gen_func_metadata(irc, pc, (FuncData *) node->ptr);
      node = node->next;
    }
  }
  
  for (i = 0; i < HTABLE_BUFF; i++) {
    DataNode* node = cd->member_vars->bucket[i];    
    while (node != NULL) {
      //      print_var_metadata((VarData *)node->ptr, indent + 1);
      
      node = node->next;
    }
  }
  
  emit_byte(irc, META_TERM);
}

void gen_metadata(IRContext *irc, ParserContext *pc) {
  int i;  
  for (i = 0; i < pc->class_data_cnt; i++) {
    ClassData *cd = pc->class_data[i];
    
    gen_class_metadata(irc, pc, cd);
  }
}

void print_bytes(IRContext *irc) {
  int i;
  for (i = 0; i < irc->byte_size; i++) {
    wprintf(L"%2x", irc->bytes[i]);

    if ((i + 1) % 8 == 0) {
      wprintf(L"\n");
    }      
  }    
}  
