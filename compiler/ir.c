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

void print_bytes(IRContext *irc) {
  int i;
  for (i = 0; i < irc->byte_size; i++) {
    wprintf(L"%2x ", irc->bytes[i]);

    if ((i + 1) % 8 == 0) {
      wprintf(L"\n");
    }      
  }    
}  
