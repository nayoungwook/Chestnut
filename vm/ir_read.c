#include <ir_read.h>
#include <vm.h>

#include <stdint.h>
#include <string.h>

#define DEBUG

#ifdef DEBUG

#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#define DEBUG_PUTCHAR(value) putchar(value)
#define DEBUG_FPRINTF(...) fprintf(__VA_ARGS__)

#else

static void debug_printf(const char *format, ...) {
}

static void debug_putchar(int value) {
}

static void debug_fprintf(FILE *stream, const char *format, ...) {
}

#define DEBUG_PRINTF(...) debug_printf(__VA_ARGS__)

#define DEBUG_PUTCHAR(value) debug_putchar(value)
#define DEBUG_FPRINTF(...) debug_fprintf(__VA_ARGS__)

#endif

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

static float read_f32(struct IRReader *reader, bool *ok) {
        uint32_t raw;
        float value;

        if (!read_u32(reader, &raw)) {
                *ok = false;
                return 0.0f;
        }
        memcpy(&value, &raw, sizeof(value));
        return value;
}

static double read_f64(struct IRReader *reader, bool *ok) {
        uint64_t raw = 0;
        double value;
        int i;

        if (!has_bytes(reader, 8)) {
                *ok = false;
                return 0.0;
        }
        for (i = 0; i < 8; i++)
                raw |= (uint64_t)reader->bytes[reader->reader_cnt++] <<
                       (i * 8);
        memcpy(&value, &raw, sizeof(value));
        return value;
}

static const char *read_string(struct IRReader *reader, bool *ok) {
        const char *value = (const char *)&reader->bytes[reader->reader_cnt];
        byte byte_value;

        while (read_byte(reader, &byte_value)) {
                if (byte_value == '\0')
                        return value;
        }
        *ok = false;
        return NULL;
}

static bool reader_error(struct IRReader *reader, const char *message) {
        DEBUG_FPRINTF(stderr, "IR read error at byte %u: %s\n",
                      reader->reader_cnt, message);
        return false;
}

static bool unknown_opcode_error(struct IRReader *reader, byte opcode) {
        DEBUG_FPRINTF(stderr, "IR read error at byte %u: unknown opcode 0x%02x\n",
                      reader->reader_cnt - 1, opcode);
        return false;
}

static const char *get_expr_op_name(byte opcode) {
        switch (opcode) {
        case OP_ADD: return "add";
        case OP_SUB: return "sub";
        case OP_MUL: return "mul";
        case OP_DIV: return "div";
        case OP_EQUAL: return "equal";
        case OP_NOTEQUAL: return "notequal";
        case OP_GREATER: return "greater";
        case OP_LESS: return "less";
        case OP_EQUALGREATER: return "eqgreater";
        case OP_EQUALLESS: return "eqless";
        case OP_ASSIGN: return "assign";
        case OP_OR: return "or";
        case OP_AND: return "and";
        default: return NULL;
        }
}

static bool read_i32_values(struct IRReader *reader, const char *name,
                            int32_t *values, unsigned count) {
        bool ok = true;
        unsigned i;

        for (i = 0; i < count; i++) {
                values[i] = read_i32(reader, &ok);
                if (!ok)
                        return false;
        }

        DEBUG_PRINTF("%s", name);
        for (i = 0; i < count; i++)
                DEBUG_PRINTF(" %d", values[i]);

        DEBUG_PUTCHAR('\n');

        return true;
}

static void handle_syscall(struct VM *vm, int id, int argc){
	switch(id){
	case 0: // print

		int i;
		for(i=0; i<argc; i++){
			//struct VMOperand op = vm_stack_pop(vm->vm_stack);
		}
		
		break;

	default:
		assert(false && "Syscall not implemented.");
	}
}

bool read_instruction(struct VM *vm, struct IRReader *reader, byte opcode,
                      bool exec) {
        int32_t arguments[5] = {0};
        bool ok = true;

	assert(vm->vm_stack != NULL);
	
        switch (opcode) {
        case OP_PUSH_NULL: {
                DEBUG_PRINTF("push_null\n");

		struct VMOperand null_op = {OPRND_NULL, 0};
		vm_stack_push(vm->vm_stack, null_op);

                break;
        }

        case OP_EXPR_OP: {
                const char *name;
                byte expr_opcode;

                if (!read_byte(reader, &expr_opcode))
                        return reader_error(reader, "truncated expression opcode");
		
                name = get_expr_op_name(expr_opcode);

                if (name == NULL)
                        return reader_error(reader, "unknown expression opcode");
                DEBUG_PRINTF("%s\n", name);

                break;
        }

        case OP_SP_PUSH: {
                ok = read_i32_values(reader, "sp_push", arguments, 1);

		unsigned amount = arguments[0];
		vm->stack_pointer += amount;
		
                break;
        }
        case OP_SP_POP: {
                ok = read_i32_values(reader, "sp_pop", arguments, 1);

		unsigned amount = arguments[0];
		vm->stack_pointer -= amount;
		
                break;
        }
        case OP_SP_LOAD: {
                ok = read_i32_values(reader, "sp_load", arguments, 2);
		
		int offset = arguments[0];
		size_t size = (size_t) arguments[1];
		int64_t val = 0;
		enum VMOPType op_type = (enum VMOPType) vm->stack_pointer_type[offset];
		
		struct VMOperand op = {op_type, val};

		memcpy(&val, vm->stack_pointer + offset, size);
		vm_stack_push(vm->vm_stack, op);
			      
                break;
        }
        case OP_SP_SAVE: {
                ok = read_i32_values(reader, "sp_save", arguments, 2);

		struct VMOperand val = vm_stack_pop(vm->vm_stack);

		int offset = arguments[0];
		size_t size = (size_t) arguments[1];
		
		memcpy(vm->stack_pointer + offset, &val.val, size);
		vm->stack_pointer_type[offset] = val.op_type;
		
                break;
        }
        case OP_SP_INCRE: {
                ok = read_i32_values(reader, "sp_incre", arguments, 2);
                break;
        }
        case OP_SP_DECRE: {
                ok = read_i32_values(reader, "sp_decre", arguments, 2);
                break;
        }

        case OP_LOAD_CLASS: {
                ok = read_i32_values(reader, "load_class", arguments, 2);
                break;
        }
        case OP_INCRE_CLASS: {
                ok = read_i32_values(reader, "incre_class", arguments, 2);
                break;
        }
        case OP_DECRE_CLASS: {
                ok = read_i32_values(reader, "decre_class", arguments, 2);
                break;
        }
        case OP_SAVE_CLASS: {
                ok = read_i32_values(reader, "save_class", arguments, 2);
                break;
        }

        case OP_LOAD_GLOBAL: {
                ok = read_i32_values(reader, "load_global", arguments, 2);
                break;
        }
        case OP_INCRE_GLOBAL: {
                ok = read_i32_values(reader, "incre_global", arguments, 2);
                break;
        }
        case OP_DECRE_GLOBAL: {
                ok = read_i32_values(reader, "decre_global", arguments, 2);
                break;
        }
        case OP_SAVE_GLOBAL: {
                ok = read_i32_values(reader, "save_global", arguments, 2);
                break;
        }

        case OP_LOAD_ATTR: {
                ok = read_i32_values(reader, "load_attr", arguments, 2);
                break;
        }
        case OP_INCRE_ATTR: {
                ok = read_i32_values(reader, "incre_attr", arguments, 2);
                break;
        }
        case OP_DECRE_ATTR: {
                ok = read_i32_values(reader, "decre_attr", arguments, 2);
                break;
        }
        case OP_SAVE_ATTR: {
                ok = read_i32_values(reader, "save_attr", arguments, 2);
                break;
        }

        case OP_SYSCALL: {
                ok = read_i32_values(reader, "syscall", arguments, 2);

		handle_syscall(vm, arguments[0], arguments[1]);
		
                break;
        }
        case OP_CALL: {
                ok = read_i32_values(reader, "call", arguments, 2);
                break;
        }
        case OP_CALL_ATTR: {
                ok = read_i32_values(reader, "call_attr", arguments, 2);
                break;
        }
        case OP_CALL_CLASS: {
                ok = read_i32_values(reader, "call_class", arguments, 2);
                break;
        }
        case OP_CALL_GLOBAL: {
                ok = read_i32_values(reader, "call_global", arguments, 2);
                break;
        }
        case OP_LOAD_STR: {
                ok = read_i32_values(reader, "load_str", arguments, 1);
		struct VMOperand str_op = {OPRND_String, arguments[0]};
		vm_stack_push(vm->vm_stack, str_op);
		
                break;
        }

        case OP_RET: {
                DEBUG_PRINTF("ret\n");
                break;
        }
        case OP_RET_VAL: {
                DEBUG_PRINTF("ret_val\n");
                break;
        }
        case OP_GOTO: {
                ok = read_i32_values(reader, "goto", arguments, 1);
                break;
        }
        case OP_LABEL: {
                ok = read_i32_values(reader, "label", arguments, 1);
                break;
        }
        case OP_JE: {
                ok = read_i32_values(reader, "je", arguments, 1);
                break;
        }
        case OP_JNE: {
                ok = read_i32_values(reader, "jne", arguments, 1);
                break;
        }
        case OP_NEG: {
                DEBUG_PRINTF("neg\n");
                break;
        }

        case OP_LDC_I4: {
                ok = read_i32_values(reader, "ldc_i4", arguments, 1);
		
		struct VMOperand i32_op = {OPRND_INT32, (int32_t) arguments[0]};
		vm_stack_push(vm->stack, i32_op);

                break;
        }
        case OP_LDC_F4: {
                float value = read_f32(reader, &ok);

                if (ok)
                        DEBUG_PRINTF("ldc_f4 %.9g\n", value);
				
		struct VMOperand f32_op = {OPRND_FLOAT32, (int32_t) arguments[0]};
		vm_stack_push(vm->stack, f32_op);

                break;
        }
        case OP_LDC_F8: {
                double value = read_f64(reader, &ok);

                if (ok)
                        DEBUG_PRINTF("ldc_f8 %.17g\n", value);

		struct VMOperand f64_op = {OPRND_FLOAT64, (int32_t) arguments[0]};
		vm_stack_push(vm->stack, f64_op);

                break;
        }

        case OP_NEW_OBJECT: {
                ok = read_i32_values(reader, "new_object", arguments, 5);
                break;
        }
        case OP_NEW_ARRAY: {
                ok = read_i32_values(reader, "new_array", arguments, 2);
                break;
        }
        case OP_ARRAY_LOAD: {
                ok = read_i32_values(reader, "array_load", arguments, 1);
                break;
        }
        case OP_ARRAY_SAVE: {
                ok = read_i32_values(reader, "array_save", arguments, 1);
                break;
        }
        case OP_ARRAY_LENGTH: {
                DEBUG_PRINTF("array_length\n");
                break;
        }
        case OP_ARRAY_PUSH: {
                ok = read_i32_values(reader, "array_push", arguments, 2);
                break;
        }
        case OP_ARRAY_REMOVE: {
                ok = read_i32_values(reader, "array_remove", arguments, 2);
                break;
        }
        default: {
                return unknown_opcode_error(reader, opcode);
        }
        }

        if (!ok)
                return reader_error(reader, "truncated instruction operand");
        return true;
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
                    reader->irc->byte_cnt - reader->reader_cnt)
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

        DEBUG_PRINTF(is_constructor ? "[constructor] %d %s" :
                     "[function] %d %s %s", id, name, return_type);
        DEBUG_PRINTF(" (");
        for (i = 0; i < (unsigned)argument_count; i++)
                DEBUG_PRINTF("%s%s", i == 0 ? "" : ", ", argument_types[i]);
        DEBUG_PRINTF(")\n");
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
                DEBUG_PRINTF("[class] %d %s parent=%d size=%d {\n", id,
                             name, parent_id, size);
                while (peek_byte(reader, &next) && next != META_TERM)
                        if (!read_meta_block(vm, reader, class_data))
                                return false;
                if (!read_byte(reader, &next) || next != META_TERM)
                        return reader_error(reader, "unterminated class metadata");
                DEBUG_PRINTF("}\n\n");
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
                DEBUG_PRINTF("[variable] %d %s %s\n", id, name, type);
                return true;
        }
        default:
                return reader_error(reader, "unknown metadata block");
        }
}

static bool read_meta(struct VM *vm, struct IRReader *reader) {
        byte next;

        DEBUG_PRINTF("----- meta begin -----\n");
        while (peek_byte(reader, &next) && next != META_END)
                if (!read_meta_block(vm, reader, NULL))
                        return false;
        if (!read_byte(reader, &next) || next != META_END)
                return reader_error(reader, "unterminated metadata section");
        DEBUG_PRINTF("----- meta end -----\n\n");
        return true;
}

static bool read_function(struct VM *vm, struct IRReader *reader,
                          struct VMClassData *owner) {
        bool ok = true;
        int32_t id = read_i32(reader, &ok);
        byte next;
        unsigned code_begin;
        unsigned code_size;
        struct VMFunctionData *function_data;

        if (!ok || id < 0)
                return reader_error(reader, "invalid function id");

        function_data = vm_find_function_data(vm, owner, (unsigned)id);
        if (function_data == NULL)
                return reader_error(reader,
                                    "function code has no matching metadata");

        DEBUG_PRINTF("func %d:\n", id);
        code_begin = reader->reader_cnt;
        while (peek_byte(reader, &next) && next != CODE_TERM) {
                if (!read_byte(reader, &next) ||
                    !read_instruction(vm, reader, next, false))
                        return false;
        }

        code_size = reader->reader_cnt - code_begin;
        if (!read_byte(reader, &next) || next != CODE_TERM)
                return reader_error(reader, "unterminated function code");

        vm_set_function_code(function_data, &reader->bytes[code_begin],
                             code_size);

        DEBUG_PUTCHAR('\n');
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
        DEBUG_PRINTF("class %d:\n", id);
        while (peek_byte(reader, &next) && next != CODE_TERM) {
                if (!read_byte(reader, &next))
                        return reader_error(reader, "truncated class code");
                if (next == CODE_FUNC) {
                        if (!read_function(vm, reader, class_data))
                                return false;
                } else if (!read_instruction(vm, reader, next, false)) {
                        return false;
                }
        }
        if (!read_byte(reader, &next) || next != CODE_TERM)
                return reader_error(reader, "unterminated class code");
        DEBUG_PUTCHAR('\n');
        return true;
}

static bool read_code(struct VM *vm, struct IRReader *reader) {
        byte kind;

        DEBUG_PRINTF("---- code begin ----\n");
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

        DEBUG_PRINTF("---- code end ----\n\n");
        return true;
}

static bool read_rodata(struct IRReader *reader) {
        byte kind;

        DEBUG_PRINTF("---- rodata begin ----\n");
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
                DEBUG_PRINTF("str : %d %s\n", id, value);
        }
        if (!read_byte(reader, &kind) || kind != RODATA_END)
                return reader_error(reader, "unterminated rodata section");
        DEBUG_PRINTF("---- rodata end ----\n\n");
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
                        ok = read_rodata(reader);
                        break;
                default:
                        reader_error(reader, "unknown IR section");
                        return;
                }
                if (!ok)
                        return;
        }
}
