#ifndef IR_READ_H
#define IR_READ_H

#include <ir.h>

struct VM;

struct ClassRelocation {
    unsigned local_id;
    unsigned runtime_id;
    unsigned parent_local_id;
};

struct IRReader {
    struct IRContext *irc;
    const byte *bytes;
    unsigned reader_cnt;
    unsigned byte_cnt;
    struct ClassRelocation *classes;
    unsigned class_count;
    unsigned class_capacity;
    unsigned string_base;
};

struct IRReader *gen_ir_reader(struct IRContext *irc);
void free_ir_reader(struct IRReader *reader);

bool read_ir(struct VM *vm, struct IRReader *ir_reader);
bool read_ir_files(struct VM *vm, struct IRReader **readers, unsigned count);

#endif
