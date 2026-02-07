#include <error.h>
#include <ir.h>
#include <parser.h>
#include <util.h>

struct IRContext *gen_irc() {
        struct IRContext *irc =
            (struct IRContext *)S_malloc(sizeof(struct IRContext));

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
                irc->bytes = (byte *)S_realloc(irc->bytes,
                                               sizeof(byte) * irc->byte_size);
        }
}

static void emit_str(struct IRContext *irc, const char *str) {
        unsigned i = 0;
        char ch;

        // [0xFF 0xAA] [0xCD 0xEF] ... [0xDF 0xER]

        while ((ch = *(str + i)) != '\0') {
                emit_byte(irc, ((ch) & 0xFF));

                i++;
        }

        // emit null character
        emit_byte(irc, 0x00);
        emit_byte(irc, 0x00);
}

// uint will be stored as little endian.
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

static void gen_func_metadata(struct IRContext *irc, struct ParserContext *pc,
                              struct FuncData *fd) {
        emit_byte(irc, META_FUNC);

        emit_int(irc, fd->id);
        emit_str(irc, fd->func_name);
        emit_str(irc, fd->return_type);
}

static void gen_var_metadata(struct IRContext *irc, struct ParserContext *pc,
                             struct VarData *vd) {
        emit_byte(irc, META_VAR);

        emit_int(irc, vd->id);
        emit_str(irc, vd->var_name);
        emit_str(irc, vd->type);
}

static void gen_class_metadata(struct IRContext *irc, struct ParserContext *pc,
                               struct ClassData *cd) {
        emit_byte(irc, META_CLASS);

        emit_int(irc, cd->id);
        emit_str(irc, cd->class_name);

        int i;
        for (i = 0; i < HTABLE_BUFF; i++) {
                struct DataNode *node = cd->member_funcs->bucket[i];

                while (node != NULL) {
                        gen_func_metadata(irc, pc,
                                          (struct FuncData *)node->ptr);
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

static void gen_metadata(struct IRContext *irc, struct ParserContext *pc) {
        int i;
        for (i = 0; i < pc->class_data_count; i++) {
                struct ClassData *cd = pc->class_data[i];

                gen_class_metadata(irc, pc, cd);
        }
}

void print_bytes(struct IRContext *irc) {
        int i;
        for (i = 0; i < irc->byte_size; i++) {
                printf("%.2x ", irc->bytes[i]);

                if ((i + 1) % 16 == 0) {
                        printf("\n");
                }
        }
}

const byte *get_bytes(struct IRContext *irc) { return irc->bytes; }

static unsigned get_size_of_type(struct ParserContext *pc,
                                 const char *type_str) {

        struct Type *type = find_type(pc, type_str);

        if (type != NULL) {
                return type->nbyte;
        }

        panic("Failed to find type", pc->tc);

        return 0;
}

static unsigned get_total_stack_size_of_func(struct ParserContext *pc,
                                             struct FuncDeclAST *func_decl) {
        int i;
        unsigned stack_offset = 0;

        for (i = 0; i < func_decl->declared_var_count; i++) {
                struct VarData *var_data = func_decl->declared_vars[i];
                unsigned data_size = get_size_of_type(pc, var_data->type);

                var_data->offset = stack_offset;

                printf("%s(%d) declared in %s, type : %s\n", var_data->var_name,
                       var_data->id, func_decl->func_name_tok->str,
                       var_data->type);
                printf("stack offset : %d, size of variable : %d\n\n",
                       stack_offset, data_size);

                stack_offset += data_size;
        }

        return 0;
}

static void gen_node_ir(struct IRContext *irc, struct ParserContext *pc,
                        struct Node *node) {
        switch (node->type) {

        case AST_NumberLiteral: {
                struct NumberLiteralAST *num_lit_ast =
                    (struct NumberLiteralAST *)node->ast;
                //		emit_byte(irc, OP_NUMBER_LITERAL);

                if (num_lit_ast->is_integer) {

                } else { // floating point.
                }

                break;
        }

        case AST_FunctionCall: {

                struct FuncCallAST *func_call_ast =
                    (struct FuncCallAST *)node->ast;

                struct FuncData *func_data = func_call_ast->func_data;

                if (!func_data->varargs) {
                        assert(func_data->arg_count ==
                               func_call_ast->param_size);
                }

                emit_byte(irc, OP_FUNC_CALL);

                emit_int(irc, func_data->id);
                emit_int(irc, func_data->arg_count);

                break;
        }

        case AST_FunctionDeclaration: {
                struct FuncDeclAST *func_decl_ast =
                    (struct FuncDeclAST *)node->ast;

                struct FuncData *func_data = func_decl_ast->func_data;

                unsigned total_stack_size =
                    get_total_stack_size_of_func(pc, func_decl_ast);

                emit_byte(irc, CODE_FUNC);
                emit_int(irc, func_data->id);

                emit_byte(irc, OP_SP_PUSH);
                emit_int(irc, total_stack_size);

                int i;
                for (i = 0; i < func_decl_ast->body_size; i++) {
                        struct Node *body_node = func_decl_ast->body[i];

                        gen_node_ir(irc, pc, body_node);
                }

                emit_byte(irc, OP_SP_POP);
                emit_int(irc, total_stack_size);

                emit_byte(irc, CODE_TERM);

                break;
        }

        default: {
                printf("Unknown ast type : %d\n", node->type);
                break;
        }
        }
}

void gen_ir(struct IRContext *irc, struct ParserContext *pc) {
        gen_metadata(irc, pc);

        emit_byte(irc, CODE_BEGIN);

        int i;
        for (i = 0; i < pc->node_size; i++) {
                struct Node *node = pc->nodes[i];

                gen_node_ir(irc, pc, node);
        }

        emit_byte(irc, CODE_END);
}
