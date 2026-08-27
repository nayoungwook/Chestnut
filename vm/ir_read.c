#include <ir_read.h>
#include <vm.h>
#include <code_data.h>

#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define VM_LABEL_CAPACITY (1024 * 4)

struct IRReader *gen_ir_reader(struct IRContext *irc) {
        struct IRReader *reader =
                (struct IRReader *)S_malloc(sizeof(struct IRReader));
        reader->bytes = irc->bytes;
        reader->irc = irc;
        reader->reader_cnt = 0;
        reader->byte_cnt = irc->byte_cnt;
        return reader;
}

static bool has_bytes(const struct IRReader *reader, unsigned count) {
        return reader->reader_cnt <= reader->byte_cnt &&
               count <= reader->byte_cnt - reader->reader_cnt;
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
        uint32_t result = 0;
        int i;

        if (!has_bytes(reader, 4))
                return false;
        for (i = 0; i < 4; i++)
                result |= (uint32_t)reader->bytes[reader->reader_cnt++] <<
                          (i * 8);
        *value = result;
        return true;
}

static bool read_u64(struct IRReader *reader, uint64_t *value) {
        uint64_t result = 0;
        int i;

        if (!has_bytes(reader, 8))
                return false;
        for (i = 0; i < 8; i++)
                result |= (uint64_t)reader->bytes[reader->reader_cnt++] <<
                          (i * 8);
        *value = result;
        return true;
}

static int32_t read_i32(struct IRReader *reader, bool *ok) {
        uint32_t raw;
        int32_t value;

        if (!read_u32(reader, &raw)) {
                *ok = false;
                return 0;
        }

        memcpy(&value, &raw, sizeof(value));
        return value;
}

static const char *read_string(struct IRReader *reader, bool *ok) {

        byte byte_value;

        char *value = (char *)&reader->bytes[reader->reader_cnt];
	
        while (read_byte(reader, &byte_value)) {
                if (byte_value == '\0'){
                        return value;
		}
        }

        *ok = false;
        return NULL;
}

static bool reader_error(struct IRReader *reader, const char *message) {
        fprintf(stderr, "IR read error at byte %u: %s\n",
                reader->reader_cnt, message);
        return false;
}

static bool is_operandless_instruction(byte opcode) {
        switch (opcode) {
        case OP_PUSH_NULL:
        case OP_RET:
        case OP_RET_VAL:
        case OP_NEG:
        case OP_ARRAY_LENGTH:
                return true;
        default:
                return false;
        }
}

static int get_i32_operand_count(byte opcode) {
        switch (opcode) {
        case OP_SP_PUSH:
        case OP_SP_POP:
        case OP_LOAD_STR:
        case OP_GOTO:
        case OP_JE:
        case OP_JNE:
        case OP_LDC_I4:
        case OP_ARRAY_LOAD:
        case OP_ARRAY_SAVE:
                return 1;
        case OP_SP_LOAD:
        case OP_SP_SAVE:
        case OP_SP_INCRE:
        case OP_SP_DECRE:
        case OP_LOAD_CLASS:
        case OP_INCRE_CLASS:
        case OP_DECRE_CLASS:
        case OP_SAVE_CLASS:
        case OP_LOAD_GLOBAL:
        case OP_INCRE_GLOBAL:
        case OP_DECRE_GLOBAL:
        case OP_SAVE_GLOBAL:
        case OP_LOAD_ATTR:
        case OP_INCRE_ATTR:
        case OP_DECRE_ATTR:
        case OP_SAVE_ATTR:
        case OP_SYSCALL:
        case OP_CALL:
        case OP_CALL_ATTR:
        case OP_CALL_CLASS:
        case OP_CALL_GLOBAL:
        case OP_NEW_ARRAY:
        case OP_ARRAY_PUSH:
        case OP_ARRAY_REMOVE:
                return 2;
        case OP_NEW_OBJECT:
                return 5;
        default:
                return -1;
        }
}

static bool decode_function_instructions(
                const byte *code, unsigned code_size,
                const struct IRReader *source_reader,
                struct VMInstruction **decoded_instructions,
                unsigned *decoded_instruction_count) {
        struct IRReader code_reader = *source_reader;
        struct VMInstruction *instructions = NULL;
        unsigned label_targets[VM_LABEL_CAPACITY];
        unsigned instruction_count = 0;
        const char *error_message = NULL;
        unsigned i;

        code_reader.bytes = code;
        code_reader.reader_cnt = 0;
        code_reader.byte_cnt = code_size;

        memset(label_targets, 0xff, sizeof(label_targets));
        if (code_size != 0)
                instructions = (struct VMInstruction *)S_malloc(
                        sizeof(struct VMInstruction) * code_size);

        while (has_bytes(&code_reader, 1)) {
                byte opcode;
                struct VMInstruction *instruction;
                int operand_count;

                if (!read_byte(&code_reader, &opcode)) {
                        error_message = "truncated instruction opcode";
                        goto fail;
                }

                if (opcode == OP_LABEL) {
                        bool ok = true;
                        int32_t label_id = read_i32(&code_reader, &ok);

                        if (!ok || label_id < 0 ||
                            (unsigned)label_id >= VM_LABEL_CAPACITY) {
                                error_message = "invalid label id";
                                goto fail;
                        }
                        if (label_targets[label_id] != UINT_MAX) {
                                error_message = "duplicate label id";
                                goto fail;
                        }
                        label_targets[label_id] = instruction_count;
                        continue;
                }

                instruction = &instructions[instruction_count];
                memset(instruction, 0, sizeof(*instruction));
                instruction->opcode = opcode;

                if (is_operandless_instruction(opcode)) {
                        operand_count = 0;
                } else if (opcode == OP_EXPR_OP) {
                        operand_count = 1;
                        if (!read_byte(&code_reader,
                                       &instruction->operands.u8)) {
                                error_message =
                                        "truncated expression operand";
                                goto fail;
                        }
                } else if (opcode == OP_LDC_F4) {
                        uint32_t raw;

                        operand_count = 1;
                        if (!read_u32(&code_reader, &raw)) {
                                error_message = "truncated float operand";
                                goto fail;
                        }
                        memcpy(&instruction->operands.f32, &raw,
                               sizeof(raw));
                } else if (opcode == OP_LDC_F8) {
                        uint64_t raw;

                        operand_count = 1;
                        if (!read_u64(&code_reader, &raw)) {
                                error_message = "truncated double operand";
                                goto fail;
                        }
                        memcpy(&instruction->operands.f64, &raw,
                               sizeof(raw));
                } else {
                        bool ok = true;

                        operand_count = get_i32_operand_count(opcode);
                        if (operand_count < 0) {
                                error_message = "unknown instruction opcode";
                                goto fail;
                        }
                        for (i = 0; i < (unsigned)operand_count; i++)
                                instruction->operands.i32[i] =
                                        read_i32(&code_reader, &ok);
                        if (!ok) {
                                error_message =
                                        "truncated instruction operand";
                                goto fail;
                        }
                }

                instruction->operand_count = (uint8_t)operand_count;
                instruction_count++;
        }

        for (i = 0; i < instruction_count; i++) {
                struct VMInstruction *instruction = &instructions[i];

                if (instruction->opcode == OP_GOTO ||
                    instruction->opcode == OP_JE ||
                    instruction->opcode == OP_JNE) {
                        int32_t label_id = instruction->operands.i32[0];

                        if (label_id < 0 ||
                            (unsigned)label_id >= VM_LABEL_CAPACITY ||
                            label_targets[label_id] == UINT_MAX) {
                                code_reader.reader_cnt = code_size;
                                error_message = "unresolved branch label";
                                goto fail;
                        }
                        instruction->operands.u32[0] =
                                label_targets[label_id];
                }
        }

        if (instruction_count != 0 && instruction_count < code_size)
                instructions = (struct VMInstruction *)S_realloc(
                        instructions,
                        sizeof(struct VMInstruction) * instruction_count);

        *decoded_instructions = instructions;
        *decoded_instruction_count = instruction_count;
        return true;

fail:
        reader_error(&code_reader, error_message);
        free(instructions);
        return false;
}

static bool read_function_metadata(struct VM *vm, struct IRReader *reader,
                                   struct VMClassData *owner,
                                   bool is_constructor) {
        bool ok = true;
        int32_t id;
        int32_t argument_count;
        const char *name;
        const char *return_type = "void";
        const char **argument_types = NULL;
        unsigned i;

        id = read_i32(reader, &ok);
        name = ok ? read_string(reader, &ok) : NULL;
        if (!is_constructor)
                return_type = ok ? read_string(reader, &ok) : NULL;
        argument_count = ok ? read_i32(reader, &ok) : 0;
        if (!ok || id < 0 || argument_count < 0 ||
            (unsigned)argument_count >
                    reader->byte_cnt - reader->reader_cnt)
                return reader_error(reader, "invalid function metadata");

        if (argument_count != 0)
                argument_types = (const char **)S_malloc(
                        sizeof(char *) * (unsigned)argument_count);
        for (i = 0; i < (unsigned)argument_count; i++) {
                argument_types[i] = read_string(reader, &ok);
                if (!ok) {
                        free(argument_types);
                        return reader_error(reader,
                                            "truncated function arguments");
                }
        }

        if (vm_find_function_data(vm, owner, (unsigned)id) != NULL) {
                free(argument_types);
                return reader_error(reader, "duplicate function metadata");
        }
        vm_add_function_data(vm, owner, (unsigned)id, name, return_type,
                             argument_types, (unsigned)argument_count,
                             is_constructor);
        free(argument_types);
        return true;
}

static bool read_meta_block(struct VM *vm, struct IRReader *reader,
                            struct VMClassData *owner) {
        byte kind;
        bool ok = true;
        int32_t id;
        const char *name;

        if (!read_byte(reader, &kind))
                return reader_error(reader, "truncated metadata block");

        switch (kind) {
        case META_CLASS: {
                byte next;
                int32_t parent_id;
                int32_t size;
                struct VMClassData *class_data;

                id = read_i32(reader, &ok);
                name = ok ? read_string(reader, &ok) : NULL;
                parent_id = ok ? read_i32(reader, &ok) : 0;
                size = ok ? read_i32(reader, &ok) : 0;
                if (!ok || id < 0 || parent_id < 0 || size < 0)
                        return reader_error(reader, "invalid class metadata");
                if (owner != NULL)
                        return reader_error(reader, "nested class metadata");
                if (vm_find_class_data(vm, (unsigned)id) != NULL)
                        return reader_error(reader, "duplicate class metadata");

                class_data = vm_add_class_data(vm, (unsigned)id, name,
                                               (unsigned)parent_id,
                                               (unsigned)size);
                while (peek_byte(reader, &next) && next != META_TERM)
                        if (!read_meta_block(vm, reader, class_data))
                                return false;
                if (!read_byte(reader, &next) || next != META_TERM)
                        return reader_error(reader, "unterminated class metadata");
                return true;
        }
        case META_FUNC:
                return read_function_metadata(vm, reader, owner, false);
        case META_CONSTRUCTOR:
                return read_function_metadata(vm, reader, owner, true);
        case META_VAR: {
                const char *type;

                id = read_i32(reader, &ok);
                name = ok ? read_string(reader, &ok) : NULL;
                type = ok ? read_string(reader, &ok) : NULL;
                if (!ok)
                        return reader_error(reader, "truncated variable metadata");
                (void)type;
                return true;
        }
        default:
                return reader_error(reader, "unknown metadata block");
        }
}

static bool read_meta(struct VM *vm, struct IRReader *reader) {
        byte next;

        while (peek_byte(reader, &next) && next != META_END)
                if (!read_meta_block(vm, reader, NULL))
                        return false;
        if (!read_byte(reader, &next) || next != META_END)
                return reader_error(reader, "unterminated metadata section");
        return true;
}

static bool read_function(struct VM *vm, struct IRReader *reader,
                          struct VMClassData *owner) {
        bool ok = true;
        int32_t id = read_i32(reader, &ok);
        int32_t encoded_code_size = ok ? read_i32(reader, &ok) : 0;
        byte next;
        unsigned code_begin;
        unsigned code_size;
        struct VMInstruction *instructions;
        unsigned instruction_count;
        struct VMFunctionData *function_data;

        if (!ok || id < 0 || encoded_code_size < 0)
                return reader_error(reader, "invalid function code header");

        function_data = vm_find_function_data(vm, owner, (unsigned)id);
        if (function_data == NULL)
                return reader_error(reader,
                                    "function code has no matching metadata");

        code_size = (unsigned)encoded_code_size;
        if (!has_bytes(reader, code_size))
                return reader_error(reader, "truncated function code");

        code_begin = reader->reader_cnt;
        if (!decode_function_instructions(&reader->bytes[code_begin],
                                          code_size, reader,
                                          &instructions,
                                          &instruction_count))
                return false;
        reader->reader_cnt += code_size;
        if (!read_byte(reader, &next) || next != CODE_TERM) {
                free(instructions);
                return reader_error(reader, "unterminated function code");
        }

        vm_set_function_instructions(function_data, instructions,
                                     instruction_count);
        free(instructions);

        return true;
}

static bool read_class(struct VM *vm, struct IRReader *reader) {
        bool ok = true;
        int32_t id = read_i32(reader, &ok);
        byte next;
        struct VMClassData *class_data;

        if (!ok || id < 0)
                return reader_error(reader, "invalid class id");
        class_data = vm_find_class_data(vm, (unsigned)id);
        if (class_data == NULL)
                return reader_error(reader,
                                    "class code has no matching metadata");
        while (peek_byte(reader, &next) && next != CODE_TERM) {
                if (!read_byte(reader, &next))
                        return reader_error(reader, "truncated class code");
                if (next != CODE_FUNC)
                        return reader_error(reader, "unknown class code block");
                if (!read_function(vm, reader, class_data))
                        return false;
        }
        if (!read_byte(reader, &next) || next != CODE_TERM)
                return reader_error(reader, "unterminated class code");
        return true;
}

static bool read_code(struct VM *vm, struct IRReader *reader) {
        byte kind;

        while (peek_byte(reader, &kind) && kind != CODE_END) {
                if (!read_byte(reader, &kind))
                        return reader_error(reader, "truncated code block");

                if (kind == CODE_CLASS) {
                        if (!read_class(vm, reader))
                                return false;
                } else if (kind == CODE_FUNC) {
                        if (!read_function(vm, reader, NULL))
                                return false;
                } else {
                        return reader_error(reader, "unknown top-level code block");
                }
        }

        if (!read_byte(reader, &kind) || kind != CODE_END)
                return reader_error(reader, "unterminated code section");

        return true;
}

static bool read_rodata(struct VM *vm, struct IRReader *reader) {
        byte kind;

	reset_string_pool(vm, 1024);

        while (peek_byte(reader, &kind) && kind != RODATA_END) {
                bool ok = true;
                int32_t id;
                const char *value;

                if (!read_byte(reader, &kind))
                        return reader_error(reader, "truncated rodata block");
                if (kind != RODATA_STR)
                        return reader_error(reader, "unknown rodata block");

		id = read_i32(reader, &ok);
                value = ok ? read_string(reader, &ok) : NULL;

                if (!ok)
                        return reader_error(reader, "truncated string rodata");
		
		register_string_pool(vm, (char *) value, id);
        }

        if (!read_byte(reader, &kind) || kind != RODATA_END)
                return reader_error(reader, "unterminated rodata section");
        return true;
}

void read_ir(struct VM *vm, struct IRReader *reader) {
        assert(reader != NULL);
        assert(reader->irc != NULL);

        while (has_bytes(reader, 1)) {
                bool ok;
                byte section;

                read_byte(reader, &section);
                switch (section) {
                case META_BEGIN:
                        ok = read_meta(vm, reader);
                        break;
                case CODE_BEGIN:
                        ok = read_code(vm, reader);
                        break;
                case RODATA_BEGIN:
                        ok = read_rodata(vm, reader);
                        break;
                default:
                        reader_error(reader, "unknown IR section");
                        return;
                }
                if (!ok)
                        return;
        }
}
