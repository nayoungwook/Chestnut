#include <error.h>
#include <parser.h>
#include <type.h>

#include <stdio.h>

bool is_castable(struct Type *from, struct Type *to) {
        bool result = false;

        if (from == to)
                return true;

        if (from->type_kind != TK_Null && from->type_kind != to->type_kind) {
                return false;
        }

        switch (from->type_kind) {
        case TK_Class: {
                struct ClassData *class_data = from->data.class_data;
                struct ClassData *to_class_data = to->data.class_data;

                // check parents and compare with 'to' type.
                while (class_data != NULL) {
                        if (class_data == to_class_data) {
                                result = true;
                                break;
                        }

                        struct Type *parent_type = class_data->parent_type;

                        if (parent_type == NULL) {
                                break;
                        }

                        assert(parent_type->type_kind == TK_Class);
                        class_data = parent_type->data.class_data;
                }
                break;
        }

        case TK_Numeric: {
                result = true;
                break;
        }

        case TK_Primitive: {
                result = from == to;
                break;
        }

        case TK_Null: {
                result = to->type_kind == TK_Class;
                break;
        }
        }

        return result;
}

struct Type *infer_type(struct ParserContext *pc, struct Node *node) {
        struct Type *result = NULL;
        const char *ident = "";

        assert(node->ast != NULL);

        switch (node->type) {

        case AST_Null: {
                result = find_type(pc, "null");
                break;
        }

        case AST_NumberLiteral: {
                struct NumberLiteralAST *num_lit_ast =
                    (struct NumberLiteralAST *)node->ast;
                result = num_lit_ast->type;

                break;
        }

        case AST_StringLiteral: {
                result = find_type(pc, "string");
                break;
        }

        case AST_FunctionCall: {
                struct FuncCallAST *func_call_ast =
                    (struct FuncCallAST *)node->ast;
                result = func_call_ast->func_data->return_type;

                break;
        }

        case AST_Identifier: {
                struct IdentifierAST *ident_ast =
                    (struct IdentifierAST *)node->ast;
                struct VarData *var_data = ident_ast->var_data;

                result = var_data->type;

                break;
        }

        case AST_IdentIncrease: {
                struct IdentIncreAST *ident_incre =
                    (struct IdentIncreAST *)node->ast;
                result = infer_type(pc, ident_incre->ident_node);

                break;
        }

        case AST_IdentDecrease: {
                struct IdentDecreAST *ident_decre =
                    (struct IdentDecreAST *)node->ast;
                result = infer_type(pc, ident_decre->ident_node);

                break;
        }

        case AST_BinExpr: {
                struct BinExprAST *bin_expr_ast =
                    (struct BinExprAST *)node->ast;
                struct Type *left_type = infer_type(pc, bin_expr_ast->left);
                struct Type *right_type = infer_type(pc, bin_expr_ast->right);

                if (!is_castable(left_type, right_type)) {
                        panic("In binary expression, failed to math left and "
                              "right expression",
                              pc->tc);
                }

                result = left_type;
                break;
        }

        case AST_UnaryExpr: {
                struct UnaryExprAST *unary_expr_ast =
                    (struct UnaryExprAST *)node->ast;

                result = infer_type(pc, unary_expr_ast->expr);

                break;
        }

        default: {
                char err_buf[512];
                sprintf(err_buf, "Failed to inference type %d", node->type);
                panic(err_buf, pc->tc);

                break;
        }
        }

        if (result == NULL) {
                char err_buf[512];

#ifdef __linux__
                sprintf(err_buf, "Failed to infer type of identifier : %s",
                        ident);
#endif

#ifdef _WIN32
                snprintf(err_buf, sizeof(err_buf),
                         "Failed to infer type of identifier : %s", ident);
#endif

                printf("%s\n", err_buf);
        }

        return result;
}

struct Type *gen_primitive_type(const char *type_str, unsigned nbyte) {
        struct Type *result = (struct Type *)S_malloc(sizeof(struct Type));

        result->type_str = type_str;
        result->nbyte = nbyte;
        result->type_kind = TK_Primitive;

        return result;
}

struct NumericData *gen_numeric_data(unsigned rank, bool is_signed,
                                     bool is_integer) {
        struct NumericData *numeric_data =
            (struct NumericData *)S_malloc(sizeof(struct NumericData));

        numeric_data->rank = rank;
        numeric_data->is_integer = is_integer;
        numeric_data->is_signed = is_signed;

        return numeric_data;
}

struct Type *gen_numeric_type(const char *type_str, unsigned nbyte,
                              struct NumericData *numeric_data) {
        struct Type *result = (struct Type *)S_malloc(sizeof(struct Type));

        result->type_kind = TK_Numeric;
        result->data.numeric_data = numeric_data;
        result->type_str = type_str;
        result->nbyte = nbyte;

        return result;
}

struct Type *gen_null_type() {
        struct Type *type = (struct Type *)S_malloc(sizeof(struct Type));

        type->type_kind = TK_Null;
        type->type_str = "null";
        type->nbyte = 0;

        return type;
}

struct Type *gen_class_type(const char *type_str) {
        struct Type *type = (struct Type *)S_malloc(sizeof(struct Type));

        type->type_kind = TK_Class;
        type->type_str = type_str;
        type->nbyte = 8;

        return type;
}

struct Type *find_type(struct ParserContext *pc, const char *type_str) {
        struct Type *result = ht_find(pc->primitive_type_smtb, type_str);

        if (result == NULL) {
                result = ht_find(pc->class_type_smtb, type_str);
        }

        if (result == NULL) {
                printf("%s\n", type_str);
                panic("Failed to find type!", pc->tc);
        }

        return result;
}

bool check_type_existance(struct ParserContext *pc, const char *type) {
        return find_type(pc, type) != NULL;
}

unsigned get_size_of_type(struct ParserContext *pc, struct Type *type) {

        if (type != NULL) {
                return type->nbyte;
        }

        panic("Failed to find type", pc->tc);

        return 0;
}
