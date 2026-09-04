#ifndef IR_READ_H
#define IR_READ_H

#include <ir.h>

struct VM;

struct IRReader {
    struct IRContext *irc;
    const byte *bytes;
    unsigned reader_cnt;
    unsigned byte_cnt;
};

struct IRReader *gen_ir_reader(struct IRContext *irc);
void free_ir_reader(struct IRReader *reader);

bool read_ir(struct VM *vm, struct IRReader *ir_reader);

#endif
