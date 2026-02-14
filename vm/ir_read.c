#include <ir_read.h>

struct IRReader *gen_ir_reader(struct IRContext *irc) {
        struct IRReader *ir_reader =
            (struct IRReader *)S_malloc(sizeof(struct IRReader));

        ir_reader->bytes = irc->bytes;
        ir_reader->irc = irc;
        ir_reader->reader_cnt = 0;

        return ir_reader;
}

static void dump_op(struct IRReader *ir_reader) {}

static void consume_str(struct IRReader *ir_reader) {
        byte b;

        while ((b = CONSUME_BYTE(ir_reader)) != '\0') {
                printf("%c", b);
        }
}

static void consume_int(struct IRReader *ir_reader) {
        int i, ac = 1, val = 0;

        for (i = 0; i < 4; i++) {
                val += (int)CONSUME_BYTE(ir_reader) * ac;
                ac <<= 8;
        }

        printf("%d", val);
}

static void read_block_meta(struct IRReader *ir_reader) {
        switch (CONSUME_BYTE(ir_reader)) {
        case META_CLASS: {
                printf("[class] ");
                consume_int(ir_reader);
                printf(" ");
                consume_str(ir_reader);
                printf(" {\n");

                while (READ_BYTE(ir_reader) != META_TERM) {
                        read_block_meta(ir_reader);
                }

                printf("}\n\n");

                break;
        }

        case META_FUNC: {
                printf("[function] ");
                consume_int(ir_reader);
                printf(" ");
                consume_str(ir_reader);
                printf(" ");
                consume_str(ir_reader);
                printf("\n");
                break;
        }

        case META_VAR: {
                printf("[variable] ");
                consume_int(ir_reader);
                printf(" ");
                consume_str(ir_reader);
                printf("\n");
                break;
        }
        }
}

static void read_meta(struct IRReader *ir_reader) {
        printf("----- meta begin -----\n");

        while (READ_BYTE(ir_reader) != META_END) {
                read_block_meta(ir_reader);
        }

        printf("----- meta end -----\n\n");
}

static void read_func_ir(struct IRReader *ir_reader) {
        printf("func ");
        consume_int(ir_reader);
        printf(":\n");

        byte b;

        while ((b = READ_BYTE(ir_reader)) != CODE_TERM) {
                switch (CONSUME_BYTE(ir_reader)) {
                case OP_SP_PUSH:
                        printf("sp_push ");
                        consume_int(ir_reader);
                        break;

                case OP_SP_POP:
                        printf("sp_pop ");
                        consume_int(ir_reader);
                        break;

                case OP_CALL:
                        printf("call ");
                        consume_int(ir_reader);
                        printf(" ");
                        consume_int(ir_reader);
                        break;

                case OP_SYSCALL:
                        printf("syscall ");
                        consume_int(ir_reader);
                        printf(" ");
                        consume_int(ir_reader);
                        break;

                case OP_LOAD_STR:
                        printf("load_str ");
                        consume_int(ir_reader);
                        break;

                case OP_RET:
                        printf("ret");
                        break;

                default:
                        printf("nop : %2x", b);
                        break;
                }

                printf("\n");
        }
}

static void read_class_ir(struct IRReader *ir_reader) {
        while (READ_BYTE(ir_reader) != CODE_TERM) {
                switch (CONSUME_BYTE(ir_reader)) {
                case CODE_FUNC:
                        read_func_ir(ir_reader);
                        break;
                }
        }
}

static void read_block_code(struct IRReader *ir_reader) {
        switch (CONSUME_BYTE(ir_reader)) {
        case CODE_CLASS:
                read_class_ir(ir_reader);
                break;

        case CODE_FUNC:
                read_func_ir(ir_reader);
                break;
        }
}

static void read_code(struct IRReader *ir_reader) {

        printf("---- code begin ----\n");

        while (READ_BYTE(ir_reader) != CODE_END) {
                read_block_code(ir_reader);
        }

        CONSUME_BYTE(ir_reader);

        printf("---- code end ----\n\n");
}

static void read_rodata(struct IRReader *ir_reader) {
        printf("---- rodata begin ----\n");

        byte b;

        while ((b = CONSUME_BYTE(ir_reader)) != RODATA_END) {
                switch (b) {
                case RODATA_STR:
                        printf("str : ");
                        consume_int(ir_reader);
                        printf(" ");
                        consume_str(ir_reader);
                        printf("\n");
                        break;
                }
        }

        printf("---- rodata end ----\n\n");
}

void read_ir(struct IRReader *ir_reader) {
        assert(ir_reader != NULL);

        while (ir_reader->reader_cnt < ir_reader->irc->byte_cnt) {
                switch (CONSUME_BYTE(ir_reader)) {
                case META_BEGIN:
                        read_meta(ir_reader);
                        break;

                case CODE_BEGIN:
                        read_code(ir_reader);
                        break;

                case RODATA_BEGIN:
                        read_rodata(ir_reader);
                        break;
                }
        }
}
