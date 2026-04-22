#include <error.h>
#include <parser.h>
#include <type.h>

struct Type *infer_type(struct ParserContext *pc, struct Node *node) {
        struct Type *result = NULL;
        const char *ident = "";

        assert(node->ast != NULL);

        switch (node->type) {

	case AST_NumberLiteral: {
		struct NumberLiteralAST *num_lit_ast = (struct NumberLiteralAST *) node->ast;
		
		break;
	}
		
        case AST_FunctionCall: {
		struct FuncCallAST *func_call_ast = (struct FuncCallAST *) node->ast;
		result = find_type(pc, func_call_ast->func_name_tok->str);
		
                break;
        }

        case AST_Identifier: {
		struct IdentifierAST *ident_ast = (struct IdentifierAST *) node->ast;
		struct VarData *var_data = ident_ast->var_data;

		result = find_type(pc, var_data->var_name);
		
                break;
        }

        default:{
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
                sprintf_s(err_buf, 512,
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

struct NumericData *gen_numeric_data(unsigned rank, bool is_signed, bool is_integer){
	struct NumericData *numeric_data = (struct NumericData *) S_malloc(sizeof(struct NumericData));

	numeric_data->rank = rank;
	numeric_data->is_integer = is_integer;
	numeric_data->is_signed = is_signed;

	return numeric_data;
}

struct Type *gen_numeric_type(const char *type_str, unsigned nbyte, struct NumericData *numeric_data) {
        struct Type *result = (struct Type *)S_malloc(sizeof(struct Type));

	result->type_kind = TK_Numeric;
	result->data.numeric_data = numeric_data;
        result->type_str = type_str;
        result->nbyte = nbyte;
	
        return result;
}

struct Type *gen_class_type(const char *type_str, void *data) {
        struct Type *type = (struct Type *)S_malloc(sizeof(struct Type));

	type->type_kind = TK_Class;
        type->type_str = type_str;
        type->data.class_data = data;
        type->nbyte = 8;

        return type;
}

struct Type *find_type(struct ParserContext *pc, const char *type_str) {
        struct Type *result = ht_find(pc->primitive_type_smtb, type_str);

        if (result == NULL) {
                result = ht_find(pc->class_type_smtb, type_str);
        }

	if(result == NULL){
		printf("%s\n", type_str);
		panic("Failed to find type!", pc->tc);
	}
	
        return result;
}

bool check_type_existance(struct ParserContext *pc, const char *type) {
        return find_type(pc, type) != NULL;
}

unsigned get_size_of_type(struct ParserContext *pc,
			  struct Type *type) {

        if (type != NULL) {
                return type->nbyte;
        }

        panic("Failed to find type", pc->tc);

        return 0;
}

