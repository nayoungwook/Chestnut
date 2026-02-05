#include <error.h>
#include <parser.h>
#include <type.h>

struct Type *infer_type(struct ParserContext *pc, struct Node *node) {
        struct Type *result = NULL;
        const char *ident = "";

        assert(node->ast != NULL);

        switch (node->type) {
        case AST_FunctionCall: {

                break;
        }

        case AST_Identifier: {

                break;
        }

        default:
                break;
        }

        if (result == NULL) {
                char err_buf[512];

#ifdef __linux__
                sprintf(err_buf, "Failed to infer type of identifier : %s",
                        ident);
#endif

#ifdef _WIN32
                sprintf_s(err_buf, 512,
                          "Failed to infer type of identifier : %s", ident);
#endif

                printf("%s\n", err_buf);
        }

        return result;
}

struct Type *gen_primitive_type(const char *type_str, unsigned nbyte,
                                unsigned rank, bool is_signed) {
        struct Type *result = (struct Type *)S_malloc(sizeof(struct Type));

        result->data = NULL;
        result->type_str = type_str;
        result->nbyte = nbyte;
        result->rank = rank;
        result->is_signed = is_signed;

        return result;
}

struct Type *gen_class_type(const char *type_str, void *data) {
        struct Type *type = (struct Type *)S_malloc(sizeof(struct Type));

        type->type_str = type_str;
        type->data = data;
        type->nbyte = 8;

        type->rank = 0;
        type->is_signed = false;

        return type;
}

struct Type *find_type(struct ParserContext *pc, const char *type_str) {
        struct Type *result = ht_find(pc->primitive_type_smtb, type_str);

        if (result == NULL) {
                result = ht_find(pc->class_type_smtb, type_str);
        }

        return result;
}

bool check_type_existance(struct ParserContext *pc, const char *type) {
        return find_type(pc, type) != NULL;
}
