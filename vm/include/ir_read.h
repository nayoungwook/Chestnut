#ifndef IR_READ_H
#define IR_READ_H

#include <ir.h>

struct IRReader {
        struct IRContext *irc;
        const byte *bytes;
        unsigned reader_cnt;
};

struct IRReader *gen_ir_reader(struct IRContext *irc);

void read_ir(struct IRReader *ir_reader);

#endif
