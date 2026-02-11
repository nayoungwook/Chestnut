#ifndef IR_READ_H
#define IR_READ_H

#define CONSUME_BYTE(ir_reader) ir_reader->bytes[ir_reader->reader_cnt++]
#define READ_BYTE(ir_reader) ir_reader->bytes[ir_reader->reader_cnt]

#define NEXT_BYTE(ir_reader) ir_reader->reader_cnt++;

#include <ir.h>

struct IRReader {
        struct IRContext *irc;
        byte *bytes;
        unsigned reader_cnt;
};

struct IRReader *gen_ir_reader(struct IRContext *irc);

void read_ir(struct IRReader *ir_reader);

#endif