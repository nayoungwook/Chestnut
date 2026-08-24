#include <ir_read.h>

#include <stdint.h>
#include <string.h>

struct IRReader *gen_ir_reader(struct IRContext *irc) {
        struct IRReader *reader =
            (struct IRReader *)S_malloc(sizeof(struct IRReader));
        reader->bytes = irc->bytes;
        reader->irc = irc;
        reader->reader_cnt = 0;
        return reader;
}

static bool has_bytes(const struct IRReader *reader, unsigned count) {
        return reader->reader_cnt <= reader->irc->byte_cnt &&
               count <= reader->irc->byte_cnt - reader->reader_cnt;
}

static bool read_byte(struct IRReader *reader, byte *value) {
        if (!has_bytes(reader, 1))
                return false;
        *value = reader->bytes[reader->reader_cnt++];
        return true;
}

static bool peek_byte(const struct IRReader *reader, byte *value) {
        if (!has_bytes(reader, 1))
                return false;
        *value = reader->bytes[reader->reader_cnt];
        return true;
}

static bool read_u32(struct IRReader *reader, uint32_t *value) {
        if (!has_bytes(reader, 4))
                return false;
        uint32_t result = 0;
        int i;
        for (i = 0; i < 4; i++)
                result |= (uint32_t)reader->bytes[reader->reader_cnt++] <<
                          (i * 8);
        *value = result;
        return true;
}

static bool print_i32(struct IRReader *reader) {
        uint32_t raw;
        int32_t value;
        if (!read_u32(reader, &raw))
                return false;
        memcpy(&value, &raw, sizeof(value));
        printf("%d", value);
        return true;
}

static bool print_f32(struct IRReader *reader) {
        uint32_t raw;
        float value;
        if (!read_u32(reader, &raw))
                return false;
        memcpy(&value, &raw, sizeof(value));
        printf("%.9g", value);
        return true;
}

static bool print_f64(struct IRReader *reader) {
        if (!has_bytes(reader, 8))
                return false;
        uint64_t raw = 0;
        double value;
        int i;
        for (i = 0; i < 8; i++)
                raw |= (uint64_t)reader->bytes[reader->reader_cnt++] <<
                       (i * 8);
        memcpy(&value, &raw, sizeof(value));
        printf("%.17g", value);
        return true;
}

static bool print_string(struct IRReader *reader) {
        byte value;
        while (read_byte(reader, &value)) {
                if (value == '\0')
                        return true;
                putchar((char)value);
        }
        return false;
}

static bool reader_error(struct IRReader *reader, const char *message) {
        fprintf(stderr, "IR read error at byte %u: %s\n", reader->reader_cnt,
                message);
        return false;
}

static bool read_expr_op(byte opcode) {
        switch (opcode) {
        case OP_ADD: printf("add"); break;
        case OP_SUB: printf("sub"); break;
        case OP_MUL: printf("mul"); break;
        case OP_DIV: printf("div"); break;
        case OP_EQUAL: printf("equal"); break;
        case OP_NOTEQUAL: printf("notequal"); break;
        case OP_GREATER: printf("greater"); break;
        case OP_LESS: printf("less"); break;
        case OP_EQUALGREATER: printf("eqgreater"); break;
        case OP_EQUALLESS: printf("eqless"); break;
        case OP_ASSIGN: printf("assign"); break;
        case OP_OR: printf("or"); break;
        case OP_AND: printf("and"); break;
        default: return false;
        }
        return true;
}

static bool print_one_int(struct IRReader *reader, const char *name) {
        printf("%s ", name);
        return print_i32(reader);
}

static bool print_two_ints(struct IRReader *reader, const char *name) {
        printf("%s ", name);
        if (!print_i32(reader))
                return false;
        putchar(' ');
        return print_i32(reader);
}

static bool read_instruction(struct IRReader *reader, byte opcode) {
        bool ok = true;
        switch (opcode) {
        case OP_PUSH_NULL: printf("push_null"); break;
        case OP_EXPR_OP: {
                byte expr_opcode;
                if (!read_byte(reader, &expr_opcode))
                        return reader_error(reader, "truncated expression opcode");
                if (!read_expr_op(expr_opcode))
                        return reader_error(reader, "unknown expression opcode");
                break;
        }

        case OP_SP_PUSH: ok = print_one_int(reader, "sp_push"); break;
        case OP_SP_POP: ok = print_one_int(reader, "sp_pop"); break;
        case OP_SP_LOAD: ok = print_two_ints(reader, "sp_load"); break;
        case OP_SP_SAVE: ok = print_two_ints(reader, "sp_save"); break;
        case OP_SP_INCRE: ok = print_two_ints(reader, "sp_incre"); break;
        case OP_SP_DECRE: ok = print_two_ints(reader, "sp_decre"); break;

        case OP_LOAD_CLASS: ok = print_two_ints(reader, "load_class"); break;
        case OP_INCRE_CLASS: ok = print_two_ints(reader, "incre_class"); break;
        case OP_DECRE_CLASS: ok = print_two_ints(reader, "decre_class"); break;
        case OP_SAVE_CLASS: ok = print_two_ints(reader, "save_class"); break;

        case OP_LOAD_GLOBAL: ok = print_two_ints(reader, "load_global"); break;
        case OP_INCRE_GLOBAL: ok = print_two_ints(reader, "incre_global"); break;
        case OP_DECRE_GLOBAL: ok = print_two_ints(reader, "decre_global"); break;
        case OP_SAVE_GLOBAL: ok = print_two_ints(reader, "save_global"); break;

        case OP_LOAD_ATTR: ok = print_two_ints(reader, "load_attr"); break;
        case OP_INCRE_ATTR: ok = print_two_ints(reader, "incre_attr"); break;
        case OP_DECRE_ATTR: ok = print_two_ints(reader, "decre_attr"); break;
        case OP_SAVE_ATTR: ok = print_two_ints(reader, "save_attr"); break;

        case OP_SYSCALL: ok = print_two_ints(reader, "syscall"); break;
        case OP_CALL: ok = print_two_ints(reader, "call"); break;
        case OP_CALL_ATTR: ok = print_two_ints(reader, "call_attr"); break;
        case OP_CALL_CLASS: ok = print_two_ints(reader, "call_class"); break;
        case OP_CALL_GLOBAL: ok = print_two_ints(reader, "call_global"); break;
        case OP_LOAD_STR: ok = print_one_int(reader, "load_str"); break;

        case OP_RET: printf("ret"); break;
        case OP_RET_VAL: printf("ret_val"); break;
        case OP_GOTO: ok = print_one_int(reader, "goto"); break;
        case OP_LABEL: ok = print_one_int(reader, "label"); break;
        case OP_JE: ok = print_one_int(reader, "je"); break;
        case OP_JNE: ok = print_one_int(reader, "jne"); break;

        case OP_LDC_I4: ok = print_one_int(reader, "ldc_i4"); break;
        case OP_LDC_F4:
                printf("ldc_f4 ");
                ok = print_f32(reader);
                break;
        case OP_LDC_F8:
                printf("ldc_f8 ");
                ok = print_f64(reader);
                break;

        case OP_NEW_OBJECT:
                printf("new_object ");
                ok = print_i32(reader);
                if (ok) { putchar(' '); ok = print_i32(reader); }
                if (ok) { putchar(' '); ok = print_i32(reader); }
                if (ok) { putchar(' '); ok = print_i32(reader); }
                if (ok) { putchar(' '); ok = print_i32(reader); }
                break;
        case OP_NEW_ARRAY: ok = print_two_ints(reader, "new_array"); break;
        case OP_ARRAY_LOAD: ok = print_one_int(reader, "array_load"); break;
        case OP_ARRAY_SAVE: ok = print_one_int(reader, "array_save"); break;
        case OP_ARRAY_LENGTH: printf("array_length"); break;
        case OP_ARRAY_PUSH: ok = print_two_ints(reader, "array_push"); break;
        case OP_ARRAY_REMOVE: ok = print_two_ints(reader, "array_remove"); break;

        default:
                fprintf(stderr, "IR read error at byte %u: unknown opcode 0x%02x\n",
                        reader->reader_cnt - 1, opcode);
                return false;
        }

        if (!ok)
                return reader_error(reader, "truncated instruction operand");
        putchar('\n');
        return true;
}

static bool read_meta_block(struct IRReader *reader) {
        byte kind;
        if (!read_byte(reader, &kind))
                return reader_error(reader, "truncated metadata block");

        switch (kind) {
        case META_CLASS: {
                printf("[class] ");
                if (!print_i32(reader) || (putchar(' '), !print_string(reader)))
                        return reader_error(reader, "truncated class metadata");
                printf(" {\n");
                byte next;
                while (peek_byte(reader, &next) && next != META_TERM)
                        if (!read_meta_block(reader))
                                return false;
                if (!read_byte(reader, &next) || next != META_TERM)
                        return reader_error(reader, "unterminated class metadata");
                printf("}\n\n");
                return true;
        }
        case META_FUNC:
                printf("[function] ");
                if (!print_i32(reader) || (putchar(' '), !print_string(reader)) ||
                    (putchar(' '), !print_string(reader)))
                        return reader_error(reader, "truncated function metadata");
                putchar('\n');
                return true;
        case META_CONSTRUCTOR:
                printf("[constructor] ");
                if (!print_i32(reader) || (putchar(' '), !print_string(reader)))
                        return reader_error(reader, "truncated constructor metadata");
                putchar('\n');
                return true;
        case META_VAR:
                printf("[variable] ");
                if (!print_i32(reader) || (putchar(' '), !print_string(reader)) ||
                    (putchar(' '), !print_string(reader)))
                        return reader_error(reader, "truncated variable metadata");
                putchar('\n');
                return true;
        default:
                return reader_error(reader, "unknown metadata block");
        }
}

static bool read_meta(struct IRReader *reader) {
        printf("----- meta begin -----\n");
        byte next;
        while (peek_byte(reader, &next) && next != META_END)
                if (!read_meta_block(reader))
                        return false;
        if (!read_byte(reader, &next) || next != META_END)
                return reader_error(reader, "unterminated metadata section");
        printf("----- meta end -----\n\n");
        return true;
}

static bool read_function(struct IRReader *reader) {
        printf("func ");
        if (!print_i32(reader))
                return reader_error(reader, "truncated function id");
        printf(":\n");

        byte next;
        while (peek_byte(reader, &next) && next != CODE_TERM) {
                if (!read_byte(reader, &next) || !read_instruction(reader, next))
                        return false;
        }
        if (!read_byte(reader, &next) || next != CODE_TERM)
                return reader_error(reader, "unterminated function code");
        putchar('\n');
        return true;
}

static bool read_class(struct IRReader *reader) {
        printf("class ");
        if (!print_i32(reader))
                return reader_error(reader, "truncated class id");
        printf(":\n");

        byte next;
        while (peek_byte(reader, &next) && next != CODE_TERM) {
                if (!read_byte(reader, &next))
                        return reader_error(reader, "truncated class code");
                if (next == CODE_FUNC) {
                        if (!read_function(reader))
                                return false;
                } else if (!read_instruction(reader, next)) {
                        return false;
                }
        }
        if (!read_byte(reader, &next) || next != CODE_TERM)
                return reader_error(reader, "unterminated class code");
        putchar('\n');
        return true;
}

static bool read_code(struct IRReader *reader) {
        printf("---- code begin ----\n");
        byte kind;
        while (peek_byte(reader, &kind) && kind != CODE_END) {
                if (!read_byte(reader, &kind))
                        return reader_error(reader, "truncated code block");
                if (kind == CODE_CLASS) {
                        if (!read_class(reader))
                                return false;
                } else if (kind == CODE_FUNC) {
                        if (!read_function(reader))
                                return false;
                } else {
                        return reader_error(reader, "unknown top-level code block");
                }
        }
        if (!read_byte(reader, &kind) || kind != CODE_END)
                return reader_error(reader, "unterminated code section");
        printf("---- code end ----\n\n");
        return true;
}

static bool read_rodata(struct IRReader *reader) {
        printf("---- rodata begin ----\n");
        byte kind;
        while (peek_byte(reader, &kind) && kind != RODATA_END) {
                if (!read_byte(reader, &kind))
                        return reader_error(reader, "truncated rodata block");
                if (kind != RODATA_STR)
                        return reader_error(reader, "unknown rodata block");
                printf("str : ");
                if (!print_i32(reader) || (putchar(' '), !print_string(reader)))
                        return reader_error(reader, "truncated string rodata");
                putchar('\n');
        }
        if (!read_byte(reader, &kind) || kind != RODATA_END)
                return reader_error(reader, "unterminated rodata section");
        printf("---- rodata end ----\n\n");
        return true;
}

void read_ir(struct IRReader *reader) {
        assert(reader != NULL);
        assert(reader->irc != NULL);

        byte section;
        while (read_byte(reader, &section)) {
                bool ok;
                switch (section) {
                case META_BEGIN: ok = read_meta(reader); break;
                case CODE_BEGIN: ok = read_code(reader); break;
                case RODATA_BEGIN: ok = read_rodata(reader); break;
                default:
                        reader_error(reader, "unknown IR section");
                        return;
                }
                if (!ok)
                        return;
        }
}
