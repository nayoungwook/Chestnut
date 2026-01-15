#include <ir.h>
#include <util.h>

struct IRContext *gen_irc() {
  struct IRContext *irc = (struct IRContext *)S_malloc(sizeof(struct IRContext));

  irc->node = NULL;
  irc->byte_cnt = 0;
  irc->byte_size = 0;
  
  return irc;
}

void init_irc(struct IRContext *irc, struct Node *node) {
  irc->node = node;
  irc->byte_cnt = 0;
  
  irc->byte_size = BYTE_CHUNK;
  irc->bytes = (byte *)S_malloc(sizeof(byte) * BYTE_CHUNK); // 4kb
}  

void emit_byte(struct IRContext *irc, byte _b) {
  irc->bytes[irc->byte_cnt++] = _b;

  if (irc->byte_cnt >= irc->byte_size) {
    irc->byte_size *= 2;    
    irc->bytes =
        (byte *)S_realloc(irc->bytes, sizeof(byte) * irc->byte_size);
  }
}

static void emit_str(struct IRContext *irc, wchar_t *str) {
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
static void emit_uint(struct IRContext *irc, unsigned ui) {
  int i;
  for (i = 0; i < sizeof(unsigned); i++) {
    emit_byte(irc, (ui & 0xFF));
    ui >>= 4;
  }    
}

static void emit_int(struct IRContext *irc, int si) {
  int i;
  for (i = 0; i < sizeof(int); i++) {
    emit_byte(irc, (si & 0xFF));
    si >>= 4;
  }
}

static void emit_float(struct IRContext *irc, float f) {
  unsigned fb; // bit data of float.
  memcpy(&fb, &f, sizeof(float));
  int i;
  for (i = 0; i < sizeof(float); i++) {
    emit_byte(irc, (fb & 0xFF));
    fb >>= 4;
  }    
}  

static void gen_func_metadata(struct IRContext *irc, struct ParserContext *pc, struct FuncData *fd) {
  emit_byte(irc, META_FUNC);

  emit_uint(irc, fd->id);
  emit_str(irc, fd->func_name);
  emit_str(irc, fd->return_type);
}

static void gen_var_metadata(struct IRContext *irc, struct ParserContext *pc, struct VarData *vd) {
  emit_byte(irc, META_VAR);

  emit_uint(irc, vd->id);
  emit_str(irc, vd->var_name);
  emit_str(irc, vd->type);
}

static void gen_class_metadata(struct IRContext *irc, struct ParserContext *pc,
                               struct ClassData *cd) {
  emit_byte(irc, META_CLASS);

  emit_uint(irc, cd->id);
  emit_str(irc, cd->class_name);

  int i;
  for (i = 0; i < HTABLE_BUFF; i++) {
    struct DataNode *node = cd->member_funcs->bucket[i];
    
    while (node != NULL) {
      gen_func_metadata(irc, pc, (struct FuncData *)node->ptr);
      node = node->next;
    }
  }
  
  for (i = 0; i < HTABLE_BUFF; i++) {
    struct DataNode *node = cd->member_vars->bucket[i];
    
    while (node != NULL) {
      gen_var_metadata(irc, pc, (struct VarData *)node->ptr);
      node = node->next;
    }
  }
  
  emit_byte(irc, META_TERM);
}

void gen_metadata(struct IRContext *irc, struct ParserContext *pc) {
  int i;  
  for (i = 0; i < pc->class_data_cnt; i++) {
    struct ClassData *cd = pc->class_data[i];
    
    gen_class_metadata(irc, pc, cd);
  }
}

void print_bytes(struct IRContext *irc) {
  int i;
  for (i = 0; i < irc->byte_size; i++) {
    wprintf(L"%2x", irc->bytes[i]);

    if ((i + 1) % 8 == 0) {
      wprintf(L"\n");
    }      
  }    
}

void gen_code(struct IRContext *irc, struct ParserContext *pc, struct Node **nodes, unsigned node_size) {
  int i;
  for (i = 0; i < node_size; i++) {
    //    struct Node *node = nodes[i];
  }
}  
