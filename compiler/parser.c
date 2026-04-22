/*
  @Author : @nayoungwook
  @Description :
  Main parser of the Chestnut compiler.
  It processes tokens from the tokenizer and translates the source code into an AST (Abstract Syntax Tree).

  This parser operates in two passes:
  1. Structure analysis: analyzes the program structure and registers ClassData and FuncData in the ParserContext.
  2. AST generation: parses the source code and generates the AST.
*/

#include "token.h"
#include "type.h"
#include "util.h"

#include <error.h>
#include <parser.h>
#include <semantics.h>

static struct Node *parse_term(struct ParserContext *pc);
static struct Node *parse_simple_expression(struct ParserContext *pc);
static struct Node *parse_unary_expression(struct ParserContext *pc);
static struct Node *parse_compare_expression(struct ParserContext *pc);
static struct Node *parse_expression(struct ParserContext *pc);

static void add_primitive_numeric(struct ParserContext *pc, const char *name, unsigned nbyte, unsigned rank, bool is_signed, bool is_integer){
	struct Type *numeric_type = gen_numeric_type(name, nbyte, gen_numeric_data(rank, is_signed, is_integer));
	ht_insert(pc->primitive_type_smtb, name, numeric_type);

	pc->numeric_type_array[pc->numeric_type_count++] = numeric_type;

	if(pc->numeric_type_count >= pc->numeric_type_capacity){
		pc->numeric_type_capacity *= 2;
		pc->numeric_type_array = (struct Type **) S_realloc(pc->numeric_type_array, sizeof(struct Type *) * pc->numeric_type_capacity);
	}
}

static void init_primitive(struct ParserContext *pc) {
        // char < int < uint < float < double
	add_primitive_numeric(pc, "int", 4, 5, true, true);
	add_primitive_numeric(pc, "float", 4, 6, true, false);
	add_primitive_numeric(pc, "double", 4, 7, true, false);

        ht_insert(pc->primitive_type_smtb, "char",
                  gen_primitive_type("char", 2));
	
        ht_insert(pc->primitive_type_smtb, "bool",
                  gen_primitive_type("bool", 1));
        ht_insert(pc->primitive_type_smtb, "void",
                  gen_primitive_type("void", 0));

	ht_insert(pc->primitive_type_smtb, "string",
                  gen_primitive_type("string", 8));
}

static void register_syscall(struct ParserContext *pc, const char *func_name, const char *ret_type, unsigned id, bool is_varargs){
	struct FuncData *func_data = gen_func_data(func_name, find_type(pc, ret_type), id, is_varargs);
	func_data->scope_data = ScopeSyscall;
	
	ht_insert(pc->syscall_smtb, func_name, func_data);
}

static void init_syscall(struct ParserContext *pc) {
        register_syscall(pc, "print", "void", 0, true);
}

struct ParserContext *gen_pc() {
        struct ParserContext *pc =
		(struct ParserContext *)S_malloc(sizeof(struct ParserContext));

        pc->tc = NULL;

        pc->first_pass_queue = gen_queue();
        pc->second_pass_queue = gen_queue();

        pc->current_scope = NULL;
        pc->current_class = NULL;
        pc->current_func = NULL;

        pc->glob_func_smtb = gen_htable();
        pc->glob_var_smtb = gen_htable();

        pc->class_type_smtb = gen_htable();
        pc->primitive_type_smtb = gen_htable();

	pc->numeric_type_array = (struct Type **) S_malloc(sizeof(struct Type *));
	pc->numeric_type_count = 0;
	pc->numeric_type_capacity = 1;
	
        pc->class_data_count = 0;
        pc->func_data_count = 0;

        pc->nodes = S_malloc(sizeof(struct Node *) * pc->node_capacity);
        pc->node_count = 0;
        pc->node_capacity = 1;

        pc->syscall_smtb = gen_htable();

        init_primitive(pc);
        init_syscall(pc);

        return pc;
}

void compile_file(struct ParserContext *pc, struct TokenizerContext *tc) {
        struct Node *node = NULL;

        pc->tc = tc;

        while ((node = parse(pc, false)) != NULL) {
                if (pc->node_count + 1 >= pc->node_capacity) {
                        pc->node_capacity *= 2;
                        pc->nodes = (struct Node **)S_realloc(
							      pc->nodes,
							      sizeof(struct Node *) * pc->node_capacity);
                }

                pc->nodes[pc->node_count++] = node;
        }

        pc->tc = NULL;
}

static struct Node *pack(enum ASTType type, void *ptr) {
        struct Node *result = (struct Node *)S_malloc(sizeof(struct Node));

        result->ast = ptr;
        result->type = type;

        return result;
}

static struct ParamData parse_func_call_params(struct ParserContext *pc) {
        struct TokenizerContext *tc = pc->tc;
	struct ParamData param_data = {0, };
	
        unsigned capacity = 1, count = 0;
        struct Node **params =
		(struct Node **)S_malloc(sizeof(struct Node *) * capacity);

        consume(tc, TokLParen);

	// parse arguments
        while (peek(tc)->type != TokRParen) {
                struct Node *expr = parse_expression(pc);

                if (count + 1 >= capacity) {
                        capacity *= 2;
                        params = (struct Node **)S_realloc(
							   params, sizeof(struct Node *) * capacity);
                }

                params[count++] = expr;

                if (peek(tc)->type == TokComma) {
                        consume(tc, TokComma);
                }
        }

	// make param data
	param_data.params = params;
	param_data.param_count = count;

        consume(tc, TokRParen);

	return param_data;
}

static struct Node *gen_func_call_node(struct Token *first,
                                       struct ParserContext *pc, bool is_expr) {
        struct FuncCallAST *func_call =
		(struct FuncCallAST *)S_malloc(sizeof(struct FuncCallAST));

        struct ParamData param_data = parse_func_call_params(pc);

	func_call->func_data = find_func_data(pc, first->str);
	func_call->params = param_data.params;
	func_call->param_count = param_data.param_count;
        func_call->func_name_tok = first;

        return pack(AST_FunctionCall, func_call);
}

static struct Node *gen_ident_node(struct Token *first, struct ParserContext *pc, bool is_expr);

static void parse_attribute(struct ParserContext *pc,
                            struct Node *ident_node, bool is_expr) {
        struct TokenizerContext *tc = pc->tc;

        struct Node *attr = gen_ident_node(pull(tc), pc, is_expr);
        ident_node->attr = attr;
}

// return binary expr ast node if it is assign expression.
static struct Node *check_assign(struct ParserContext *pc,
                                 struct Node *result) {
        struct TokenizerContext *tc = pc->tc;
        struct Token *nt = peek(tc);

        if (nt->type != TokAssign) {
                return result;
        }

        consume(tc, TokAssign);

        struct Node *expr = parse_expression(pc);

        struct BinExprAST *bin_expr_ast =
		(struct BinExprAST *)S_malloc(sizeof(struct BinExprAST));

        bin_expr_ast->left = result;
        bin_expr_ast->op_type = OpASSIGN;
        bin_expr_ast->right = expr;

        return pack(AST_BinExpr, bin_expr_ast);
}

// return incre | decre ast node if it is increase or decrease operation.
static struct Node *check_incre_decre(struct ParserContext *pc,
				      struct Node *result){
	struct TokenizerContext *tc = pc->tc;
	struct Token *nt = peek(tc);
	
	if(!(nt->type == TokIncrease || nt->type == TokDecrease)){
		return result;
	}

	enum ASTType last_ast_type;
	struct Node *searcher = result;

	while(searcher->attr != NULL){
		searcher = searcher->attr;
	}

	last_ast_type = searcher->type;
	
	if(last_ast_type != AST_Identifier){
		if(nt->type == TokIncrease){
			panic("Increasement (++) must be used with identifier!", tc);
		}
		
		if(nt->type == TokDecrease){
			panic("Decreasement (--) must be used with identifier!", tc);
		}
	}
	
	if(nt->type == TokIncrease){
		consume(tc, TokIncrease);
		struct IdentIncreAST *incre_ast = (struct IdentIncreAST *) S_malloc(sizeof(struct IdentIncreAST));
		incre_ast->ident_node = result;

		return pack(AST_IdentIncrease, incre_ast);
	}

	if(nt->type == TokDecrease){
		consume(tc, TokDecrease);
		struct IdentDecreAST *decre_ast = (struct IdentDecreAST *) S_malloc(sizeof(struct IdentDecreAST));
		decre_ast->ident_node = result;

		return pack(AST_IdentDecrease, decre_ast);
	}

	return NULL;
}

static struct Node *gen_ident_node(struct Token *first, struct ParserContext *pc, bool is_expr) {
        struct TokenizerContext *tc = pc->tc;
        struct Token *nt = peek(tc);

        struct Node *result = NULL;

        if (nt->type == TokLParen) {
                result = gen_func_call_node(first, pc, is_expr);
        } else {
                struct IdentifierAST *ident_ast =
			(struct IdentifierAST *)S_malloc(
							 sizeof(struct IdentifierAST));
                ident_ast->ident = first;

                result = pack(AST_Identifier, ident_ast);
        }

        assert(result != NULL);

	bool is_last_node = true;

        if (peek(tc)->type == TokDot) {
                consume(tc, TokDot);

		is_last_node = false;
                parse_attribute(pc, result, is_expr);
        }
                
	result = check_incre_decre(pc, result);
	result = check_assign(pc, result);

	if(!is_expr && is_last_node)
		consume(tc, TokSemiColon);

        return result;
}

// create node of function parameters. (Variable Declaration Bundle AST)
static struct Node *gen_func_param_node(struct ParserContext *pc) {
        struct TokenizerContext *tc = pc->tc;
        struct VarDeclBundleAST *result = (struct VarDeclBundleAST *)S_malloc(
									      sizeof(struct VarDeclBundleAST));

        result->var_count = 0;
        result->var_decls = (struct Node **)S_malloc(sizeof(struct Node *));

        unsigned param_size = 0, capacity = 1;

        // ( a : int, b : int )
        consume(tc, TokLParen);

        while (peek(tc)->type != TokRParen) {
                struct Token *name_tok = pull(tc);

                consume(tc, TokColon); // :

                struct Token *type_tok = pull(tc);
                struct VarDeclAST *var_decl =
			(struct VarDeclAST *)S_malloc(sizeof(struct VarDeclAST));
                var_decl->decl = NULL;
                var_decl->var_name_tok = name_tok;
                var_decl->var_type = find_type(pc, type_tok->str);

                if (param_size + 1 >= capacity) {
                        capacity *= 2;
                        result->var_decls = S_realloc(
						      result->var_decls,
						      sizeof(struct Node *) * capacity);
                }

                result->var_decls[param_size++] =
			pack(AST_VariableDeclaration, var_decl);

                struct Token *nt = peek(tc);
                if (nt->type == TokRParen)
                        break;
                if (nt->type == TokComma)
                        pull(tc);
        }

        result->var_count = param_size;

        consume(tc, TokRParen);

        return pack(AST_VariableDeclarationBundle, result);
}

static struct Node **gen_body(struct ParserContext *pc, unsigned *body_size) {
        struct TokenizerContext *tc = pc->tc;

        consume(tc, TokLBracket);

        unsigned size = 0, capacity = 1;
        struct Node **result =
		(struct Node **)S_malloc(sizeof(struct Node *) * capacity);

        while (peek(tc)->type != TokRBracket) {
                void *element = parse(pc, false);
                assert(element != NULL);

                if (size + 1 >= capacity) {
                        capacity *= 2;
                        result = (struct Node **)S_realloc(
							   result, sizeof(struct Node *) * capacity);
                }

                result[size++] = element;
        }

        consume(tc, TokRBracket);

        *body_size = size;

        return result;
}

static struct Node *gen_constructor_node(struct Token *first,
					 struct ParserContext *pc){
	struct ConstructorAST *constructor = (struct ConstructorAST *) S_malloc(sizeof(struct ConstructorAST));

        unsigned body_count = 0;

        // reset local var declaration.
        pc->declared_local_var_count = 0;
        pc->declared_local_var_capacity = 1;
        pc->declared_local_vars =
		(struct VarData **)S_malloc(sizeof(struct VarData *));
	
	struct Token *func_name = consume(pc->tc, TokIdent);
	
        struct Node *params = gen_func_param_node(pc);
        assert(params->type == AST_VariableDeclarationBundle);

	struct VarDeclBundleAST *params_ast = (struct VarDeclBundleAST *) params->ast;
	
	constructor->params = params;

        struct Node *result = pack(AST_Constructor, constructor);

        constructor->declared_var_count = pc->declared_local_var_count;
        constructor->declared_vars = pc->declared_local_vars;
	
	if(pc->current_class == NULL){
		panic("Constructor declared out of class.", pc->tc);
	}

	constructor->class_data = pc->current_class;

	struct FuncData *func_data = register_constructor_data(func_name->str, pc);
	
	constructor->func_data = func_data;

        func_data->arg_types = S_malloc(params_ast->var_count * sizeof(char *));
        func_data->arg_count = params_ast->var_count;

	int i;
        for (i = 0; i < params_ast->var_count; i++) {
                struct VarDeclAST *param_ast =
			(struct VarDeclAST *) (params_ast->var_decls[i]->ast);

                func_data->arg_types[i] = param_ast->var_type;
        }

	pc->current_func = constructor->func_data;
        constructor->body = gen_body(pc, &body_count);
	pc->current_func = NULL;
	
        constructor->body_count = body_count;
	
	pc->current_class->constructor = constructor->func_data;
	
        return result;
}

static struct Node *gen_func_decl_node(struct Token *first,
                                       struct ParserContext *pc) {
        struct FuncDeclAST *func_decl =
		(struct FuncDeclAST *)S_malloc(sizeof(struct FuncDeclAST));

        struct TokenizerContext *tc = pc->tc;
        unsigned body_count = 0;
	int i;
	
        // reset local var declaration.
        pc->declared_local_var_count = 0;
        pc->declared_local_var_capacity = 1;
        pc->declared_local_vars =
		(struct VarData **)S_malloc(sizeof(struct VarData *));

        struct Token *func_name_tok = pull(tc);

        struct Node *params = gen_func_param_node(pc);
	assert(params->type == AST_VariableDeclarationBundle);
	struct VarDeclBundleAST *params_ast = (struct VarDeclBundleAST *) params->ast;

        assert(params->type == AST_VariableDeclarationBundle);

        consume(tc, TokColon);

        struct Token *ret_type_tok = pull(tc);
	struct Type *ret_type  = find_type(pc, ret_type_tok->str);
	
        func_decl->func_name_tok = func_name_tok;
        func_decl->ret_type = ret_type;
        func_decl->params = params;
	func_decl->func_data = find_func_data(pc, func_name_tok->str);

        struct FuncData *func_data =
            register_func_data(func_name_tok->str, ret_type, pc);

        func_data->arg_types = S_malloc(params_ast->var_count * sizeof(char *));
        func_data->arg_count = params_ast->var_count;

        for (i = 0; i < params_ast->var_count; i++) {
                struct VarDeclAST *param_ast =
			(struct VarDeclAST *) (params_ast->var_decls[i]->ast);

                func_data->arg_types[i] = param_ast->var_type;
        }

        // register local var data to calcaulte stack size after.
        func_decl->declared_var_count = pc->declared_local_var_count;
        func_decl->declared_vars = pc->declared_local_vars;
	
        struct Node *result = pack(AST_FunctionDeclaration, func_decl);

	pc->current_func = func_decl->func_data;
        func_decl->body = gen_body(pc, &body_count);
	pc->current_func = NULL;

        func_decl->body_count = body_count;

        return result;
}

static struct Node *gen_var_decl_node(struct Token *first,
                                      struct ParserContext *pc, bool is_expr) {
        struct TokenizerContext *tc = pc->tc;
        struct VarDeclBundleAST *result = (struct VarDeclBundleAST *)S_malloc(
            sizeof(struct VarDeclBundleAST));

        result->var_decls = (struct Node **)S_malloc(sizeof(struct Node *));

        // var a: int = 0, b : float;

        bool comp = false;
        unsigned var_count = 0, capacity = 1;

        while (!comp) {
                struct Token *var_name_tok = pull(tc);

                consume(tc, TokColon);

                struct Token *var_type_tok = pull(tc);

                struct Token *cont_tok = peek(tc);

                void *decl = NULL;

                if (cont_tok->type == TokAssign) {
                        consume(tc, TokAssign);
                        decl = parse_expression(pc);
                        cont_tok = peek(tc);
                }

                switch (cont_tok->type) {
                case TokComma:
                        consume(tc, TokComma);
                        break;
                case TokSemiColon:
                        comp = true;
                        if (!is_expr) // if it is statement, consume semicolon
                                consume(tc, TokSemiColon);

                        break;
                default:
                        panic("Unexpected token in variable declaration.", tc);
                }

                struct VarDeclAST *var_decl =
                    (struct VarDeclAST *)S_malloc(sizeof(struct VarDeclAST));
		struct Type *var_type = find_type(pc, var_type_tok->str);
		
                var_decl->var_name_tok = var_name_tok;
                var_decl->var_type = var_type;
                var_decl->decl = decl;
                var_decl->ac_mod = ACMOD_DEFAULT;
		var_decl->var_data = NULL;

                register_non_local_var_data(var_name_tok->str, find_type(pc, var_type_tok->str), pc);

                if (var_count + 1 >= capacity) {
                        capacity *= 2;
                        result->var_decls = (struct Node **)S_realloc(
                            result->var_decls,
                            sizeof(struct Node *) * capacity);
                }

                struct Node *node = pack(AST_VariableDeclaration, var_decl);
                result->var_decls[var_count++] = node;
		result->var_count = var_count;
        }
	
        return pack(AST_VariableDeclarationBundle, result);
}

static struct Node *gen_if_stmt_node(struct Token *first,
                                     struct ParserContext *pc);
static enum IfStmtType get_if_stmt_type(struct Token *first,
                                        struct ParserContext *pc);
static struct Node *gen_next_if_stmt_node(enum IfStmtType stmt_type,
                                          struct ParserContext *pc);

static enum IfStmtType get_if_stmt_type(struct Token *first,
                                        struct ParserContext *pc) {
        struct TokenizerContext *tc = pc->tc;
        struct Token *nt = peek(tc);
        enum IfStmtType stmt_type = StmtNone;

        if (first->type == TokIf) {
                stmt_type = StmtIf;
        }

        if (first->type == TokElse) {
                stmt_type = StmtElse;

                if (nt->type == TokIf) {
                        stmt_type = StmtElseIf;
                }
        }

        return stmt_type;
}

static struct Node *gen_next_if_stmt_node(enum IfStmtType stmt_type,
                                          struct ParserContext *pc) {
        struct TokenizerContext *tc = pc->tc;
        struct Token *nt = peek(tc);
        struct Node *next_stmt = NULL;

        if (nt->type == TokElse) {
                consume(tc, TokElse);
                next_stmt = gen_if_stmt_node(nt, pc);
        }

        if (stmt_type == StmtElse && next_stmt != NULL) {
                panic("Wrong statement. statement after else statement.", tc);
        }

        return next_stmt;
}

static struct Node *gen_if_stmt_node(struct Token *first,
                                     struct ParserContext *pc) {
        struct IfStmtAST *result =
            (struct IfStmtAST *)S_malloc(sizeof(struct IfStmtAST));

        struct TokenizerContext *tc = pc->tc;
        enum IfStmtType stmt_type = get_if_stmt_type(first, pc);

        if (stmt_type == StmtElseIf) {
                consume(tc, TokIf);
        }
        if (stmt_type == StmtNone) {
                panic("Wrong Statement type", tc);
        }

        void *cond = NULL;
        if (stmt_type == StmtElseIf || stmt_type == StmtIf) {
                consume(tc, TokLParen);
                cond = parse_expression(pc);
                consume(tc, TokRParen);
        }

        unsigned body_count = 0;
        struct Node **body = gen_body(pc, &body_count);

        result->cond = cond;
        result->body = body;
        result->body_count = body_count;
        result->stmt_type = stmt_type;

        // gen next if stmt
        result->next_stmt = gen_next_if_stmt_node(stmt_type, pc);

        return pack(AST_IfStatement, result);
}

static struct Node *gen_for_stmt_node(struct Token *first,
                                      struct ParserContext *pc) {
        struct ForStmtAST *result =
            (struct ForStmtAST *)S_malloc(sizeof(struct ForStmtAST));

        struct TokenizerContext *tc = pc->tc;

        consume(tc, TokLParen);

        struct Node *init = parse_expression(pc);
        consume(tc, TokSemiColon);

        struct Node *cond = parse_expression(pc);
        consume(tc, TokSemiColon);

        struct Node *step = parse_expression(pc);

        consume(tc, TokRParen);

        unsigned body_count = 0;
        struct Node **body = gen_body(pc, &body_count);

        result->init = init;
        result->cond = cond;
        result->step = step;
        result->body = body;
        result->body_count = body_count;

        return pack(AST_ForStatement, result);
}

static struct Node *gen_ret_node(struct Token *first,
                                 struct ParserContext *pc) {
        struct Node *expr = parse_expression(pc);
        struct ReturnAST *ret_ast =
            (struct ReturnAST *)S_malloc(sizeof(struct ReturnAST));

        ret_ast->expr = expr;

	consume(pc->tc, TokSemiColon);
	
        return pack(AST_Return, ret_ast);
}

static struct Node *gen_class_decl_node(struct Token *first,
                                        struct ParserContext *pc) {
        struct TokenizerContext *tc = pc->tc;
        struct ClassAST *class_ast =
            (struct ClassAST *)S_malloc(sizeof(struct ClassAST));

        struct Token *name_tok = pull(tc);
        class_ast->name_tok = name_tok;
        class_ast->parent_name_tok = NULL;

	struct Type *class_type = find_type(pc, name_tok->str);
	struct Type *parent_type = NULL;
	
        if (peek(tc)->type == TokExtends) {
                consume(tc, TokExtends);

                struct Token *parent_name_tok = pull(tc);
                class_ast->parent_name_tok = parent_name_tok;

		parent_type = find_type(pc, parent_name_tok->str);
        }

	// Data & Type registeration

	struct ClassData *class_data = find_class_data(pc, find_type(pc, name_tok->str));
	
	if(class_data == NULL){
		panic("Failed to find class data", pc->tc);
	}

	class_data->class_type = class_type;
	class_data->parent_type = parent_type;
	class_ast->class_data = class_data;
	
        struct Node *result = pack(AST_Class, class_ast);

        unsigned body_size;

	pc->current_class = class_data;
        class_ast->body = gen_body(pc, &body_size);
	pc->current_class = NULL;
        class_ast->body_count = body_size;

        return result;
}

static struct Node *gen_new_node(struct Token *first,
				 struct ParserContext *pc){

	struct TokenizerContext *tc = pc->tc;
	struct Token *class_name_tok = consume(tc, TokIdent);
	struct ParamData param_data = parse_func_call_params(pc);

	struct NewAST *new_ast = (struct NewAST *) S_malloc(sizeof(struct NewAST));

	new_ast->name_tok = class_name_tok;
	new_ast->params = param_data.params;
	new_ast->param_count = param_data.param_count;
	
	return pack(AST_New, new_ast);
}

//=======================================================================
//      FIRST PASS OF COMPILER FOR CODE STRUCTURES AND BLOCKS.
//=======================================================================

static void pass_body(struct ParserContext *pc){
	struct TokenizerContext *tc = pc->tc;
	struct Token *tok;
	
	consume(tc, TokLBracket);
        int bracket_counter = 1;

        while ((tok = pull(tc))->type != TokEOF) {
                if (tok->type == TokRBracket) {
                        bracket_counter--;
                }
                if (tok->type == TokLBracket) {
                        bracket_counter++;
                }

                if (bracket_counter == 0)
                        break;
        }
}

static void pass_func_param(struct ParserContext *pc){
	struct TokenizerContext *tc = pc->tc;
	struct Token *tok;
	
	consume(tc, TokLParen);
        int paren_counter = 1;

        while ((tok = pull(tc))->type != TokEOF) {
                if (tok->type == TokRParen) {
                        paren_counter--;
                }
                if (tok->type == TokLParen) {
                        paren_counter++;
                }

                if (paren_counter == 0)
                        break;
        }
}

static void parse_class_structure(struct ParserContext *pc) {
        struct TokenizerContext *tc = pc->tc;

        struct Token *class_name_tok = pull(tc);

        const char *class_name = class_name_tok->str;

        if (peek(tc)->type == TokExtends) {
                consume(tc, TokExtends);
		consume(tc, TokIdent);
        }

        struct ClassData *cd = register_class_data(class_name, pc);
        pc->current_class = cd;

	pass_body(pc);

        pc->current_class = NULL;
}

static void parse_func_structure(struct ParserContext *pc) {
        struct TokenizerContext *tc = pc->tc;

        consume(tc, TokIdent);

	pass_func_param(pc);

        consume(tc, TokColon);

        pull(tc); // return type
	
        // We will not parse content of function declaration.
	pass_body(pc);
}

static void parse_constructor_structure(struct ParserContext *pc) {
        struct TokenizerContext *tc = pc->tc;

	consume(tc, TokIdent);

	pass_func_param(pc);
        // We will not parse content of function declaration.
	pass_body(pc);
}

static void parse_var_structure(struct ParserContext *pc) {
        struct TokenizerContext *tc = pc->tc;
        bool comp = false;

        while (!comp) {
		consume(tc, TokIdent);
		
                consume(tc, TokColon);

                pull(tc); // type

                struct Token *tok = NULL;

                while ((tok = pull(tc))->type != TokEOF) {
                        if (tok->type == TokSemiColon) {
                                comp = true;
                                break;
                        }

                        if (tok->type == TokComma) {
                                break;
                        }
                }
        }
}

// First pass of compiler
// This function will parse the structure of source code roughly
// This function will register global class data (with member of class) , global
// function data, global variable data
void parse_structure(struct ParserContext *pc) {
        struct TokenizerContext *tc = pc->tc;

        assert(tc != NULL);

        struct Token *first = pull(tc);

        switch (first->type) {
        case TokClass: {
                parse_class_structure(pc);
                break;
        }

        case TokFunc: {
                parse_func_structure(pc);
                break;
        }

        case TokVar: {
                parse_var_structure(pc);
                break;
        }

	case TokConstructor: {
		parse_constructor_structure(pc);
		break;
	}
		
        default: {
                printf("\n%d\n", first->type);
                panic("Unexpected token, block of source code must begin with "
                      "class or "
                      "function.",
                      tc);
        }
        }
}

static void debug_view_func_smtb(struct HTable *htable) {
        int i, j;
        for (i = 0; i < HTABLE_BUFF; i++) {
                struct DataNode *dn = htable->bucket[i];

                while (dn != NULL) {
                        struct FuncData *fd = (struct FuncData *)dn->ptr;

                        printf("Function Data : %s(%d), ret type : %s ",
                               fd->func_name, fd->id, fd->return_type->type_str);

                        printf("Arguments : ");
                        for (j = 0; j < fd->arg_count; j++) {
                                printf("%s,", fd->arg_types[j]->type_str);
                        }
                        printf("\n");

                        dn = dn->next;
                }
        }
}

static void debug_view_var_smtb(struct HTable *htable) {
        int i;
        for (i = 0; i < HTABLE_BUFF; i++) {
                struct DataNode *dn = htable->bucket[i];

                while (dn != NULL) {
                        struct VarData *vd = (struct VarData *)dn->ptr;

                        printf("Variable Data : %s(%d), type : %s\n",
                               vd->var_name, vd->id, vd->type->type_str);
                        dn = dn->next;
                }
        }
}

void debug_view_data(struct ParserContext *pc) {
        debug_view_func_smtb(pc->glob_func_smtb);

        int i;
        for (i = 0; i < pc->class_data_count; i++) {
                struct ClassData *cd = pc->class_data[i];
		
                printf("Class Data : %s(%d), parent :", cd->class_type->type_str,
                       cd->id);

		printf(" %s\n", cd->parent_type == NULL ? "(none)" : cd->parent_type->type_str); 
		
                printf("------ members -----\n");

                debug_view_func_smtb(cd->member_funcs);
                debug_view_var_smtb(cd->member_vars);

                printf("-------------------\n\n");
        }
}

static bool is_number_literal_integer(const char *num_lit_str){
	char *c = (char *) num_lit_str;

	while(*c != '\0'){
		if(*c == '.' || *c == 'f'){
			return false;
		}

		c++;
	}
	
	return true;
}

static short get_number_literal_byte(const char *num_lit_str){
	short result = 0;

	size_t len = strlen(num_lit_str);

	bool is_float = num_lit_str[len - 1] == 'f';
	bool is_integer = is_number_literal_integer(num_lit_str);

	if(is_float || is_integer){ // int and float
		result = 4;
	}

	if(!is_integer && !is_float){ // double
		result = 8;
	}
	
	return result;
}

static struct Type *find_numeric_type(struct ParserContext *pc, unsigned nbyte, bool is_integer, bool is_signed){
	int i;
	for(i=0; i < pc->numeric_type_count; i++){
		struct Type *numeric_type = pc->numeric_type_array[i];
		assert(numeric_type->type_kind == TK_Numeric);
		struct NumericData *numeric_data = numeric_type->data.numeric_data;

		if(numeric_data->is_integer == is_integer && numeric_data->is_signed == is_signed && numeric_type->nbyte == nbyte){
			return numeric_type;
		}
	}

	return NULL;
}

struct Node *parse(struct ParserContext *pc, bool is_expr) {

        struct TokenizerContext *tc = pc->tc;
        assert(tc != NULL);

        struct Token *first = pull(tc);

        switch (first->type) {

        case TokNumberLiteral: {
                struct NumberLiteralAST *num =
                    (struct NumberLiteralAST *)S_malloc(
                        sizeof(struct NumberLiteralAST));
		
                num->num_tok = first;
		
		bool is_integer = is_number_literal_integer(num->num_tok->str);
		bool is_signed = true;
		unsigned nbyte = get_number_literal_byte(num->num_tok->str);

		struct Type *numeric_type = find_numeric_type(pc, nbyte, is_integer, is_signed);

		if(numeric_type == NULL){
			panic("This number literal is not supported in this compiler.", pc->tc);
		}

		num->type = numeric_type;
		
                return pack(AST_NumberLiteral, num);
        }

        case TokStringLiteral: {
                struct StringLiteralAST *str =
                    (struct StringLiteralAST *)S_malloc(
                        sizeof(struct StringLiteralAST));

                str->str_tok = first;

                return pack(AST_StringLiteral, str);
        }

	case TokIncrease: {
		struct IdentIncreAST *incre = (struct IdentIncreAST *) S_malloc(sizeof(struct IdentIncreAST));

		struct Node *node = parse(pc, is_expr);
		
		if(node->type != AST_Identifier){
			panic("Increasement (++) must be used with identifier!", pc->tc);
		}
		
		incre->ident_node = node;
		
		return pack(AST_IdentIncrease, incre);
	}

	case TokDecrease: {
		struct IdentDecreAST *decre = (struct IdentDecreAST *) S_malloc(sizeof(struct IdentDecreAST));

		struct Node *node = parse(pc, is_expr);

		if(node->type != AST_Identifier){
			panic("Decreasement (--) must be used with identifier!", pc->tc);
		}
		
		decre->ident_node = node;
		
		return pack(AST_IdentDecrease, decre);
	}
		
        case TokNull: {
                struct NullAST *null =
                    (struct NullAST *)S_malloc(sizeof(struct NullAST *));
                null->null_tok = first;

                return pack(AST_Null, null);
        }

        case TokIf: {
                return gen_if_stmt_node(first, pc);
        }

        case TokFor: {
                return gen_for_stmt_node(first, pc);
        }

        case TokFunc: {
                return gen_func_decl_node(first, pc);
        }

	case TokConstructor: {
		return gen_constructor_node(first, pc);
	}
		
        case TokVar: {
                return gen_var_decl_node(first, pc, is_expr);
        }

        case TokClass: {
                return gen_class_decl_node(first, pc);
        }

	case TokNew: {
		return gen_new_node(first, pc);
	}

        case TokIdent: {
                return gen_ident_node(first, pc, is_expr);
        }
		
        case TokReturn: {
                return gen_ret_node(first, pc);
        }

        case TokLParen: {
                struct Node *expr = parse_expression(pc);

                consume(tc, TokRParen);

                return expr;
        }

        case TokEOF: {
                return NULL;
        }

        default:
                panic("Unexpected Token type\n", tc);
        }

        return NULL;
}

static struct Node *parse_term(struct ParserContext *pc) {
        struct Node *node = parse(pc, true);
        struct TokenizerContext *tc = pc->tc;
        struct Token *tok = NULL;

        while ((tok = peek(tc)) != NULL &&
               (tok->type == TokMul || tok->type == TokDiv)) {
                enum TokenType op = pull(tc)->type;
                void *right = parse(pc, true);

                enum OperatorType op_type = (op == TokMul) ? OpMUL : OpDIV;

                struct BinExprAST *bin_expr =
                    (struct BinExprAST *)S_malloc(sizeof(struct BinExprAST));

                bin_expr->left = node;
                bin_expr->right = right;
                bin_expr->op_type = op_type;

                node = pack(AST_BinExpr, bin_expr);
        }
        return node;
}

static struct Node *parse_simple_expression(struct ParserContext *pc) {
        struct Node *node = parse_term(pc);
        struct TokenizerContext *tc = pc->tc;
        struct Token *tok = NULL;

        while ((tok = peek(tc)) != NULL &&
               (tok->type == TokAdd || tok->type == TokSub)) {
                enum TokenType op = pull(tc)->type;
                void *right = parse_term(pc);

                enum OperatorType op_type = (op == TokAdd) ? OpADD : OpSUB;

                struct BinExprAST *bin_expr =
                    (struct BinExprAST *)S_malloc(sizeof(struct BinExprAST));

                bin_expr->left = node;
                bin_expr->right = right;
                bin_expr->op_type = op_type;

                node = pack(AST_BinExpr, bin_expr);
        }
        return node;
}

static struct Node *parse_unary_expression(struct ParserContext *pc) {
        struct TokenizerContext *tc = pc->tc;
        struct Token *tok = NULL;

        if ((tok = peek(tc)) != NULL && tok->type == TokNot) {
                pull(tc); // Consume '!'
                struct UnaryExprAST *unary_expr =
                    (struct UnaryExprAST *)S_malloc(
                        sizeof(struct UnaryExprAST));

                unary_expr->expr = parse_unary_expression(pc);
                return pack(AST_UnaryExpr, unary_expr);
        }

        return parse_simple_expression(pc);
}

static struct Node *parse_compare_expression(struct ParserContext *pc) {
        void *node = parse_unary_expression(pc);
        struct TokenizerContext *tc = pc->tc;
        struct Token *tok = NULL;

        while (((tok = peek(tc)) != NULL) &&
               (tok->type == TokEqual || tok->type == TokNotEqual ||
                tok->type == TokGreater || tok->type == TokLesser ||
                tok->type == TokEqualGreater || tok->type == TokEqualLesser)) {

                struct Token *operator_token = pull(tc);
                enum TokenType op = operator_token->type;
                struct Node *right = parse_unary_expression(pc);

                enum OperatorType op_type = OpNone;
                switch (op) {
                case TokEqual:
                        op_type = OpEQUAL;
                        break;
                case TokNotEqual:
                        op_type = OpNOTEQUAL;
                        break;
                case TokGreater:
                        op_type = OpGREATER;
                        break;
                case TokLesser:
                        op_type = OpLESS;
                        break;
                case TokEqualGreater:
                        op_type = OpEQUALGREATER;
                        break;
                case TokEqualLesser:
                        op_type = OpEQUALLESS;
                        break;
                default:
                        panic("Unknown operator type.", tc);
                }

                struct BinExprAST *bin_expr =
                    (struct BinExprAST *)S_malloc(sizeof(struct BinExprAST));

                bin_expr->left = node;
                bin_expr->right = right;

                bin_expr->op_type = op_type;

                node = pack(AST_BinExpr, bin_expr);
        }
        return node;
}

static struct Node *parse_expression(struct ParserContext *pc) {
        struct Node *node = parse_compare_expression(pc);
        struct TokenizerContext *tc = pc->tc;
        struct Token *tok = NULL;

        while (((tok = peek(tc)) != NULL) &&
               (peek(tc)->type == TokOr || peek(tc)->type == TokAnd)) {
                enum TokenType op = pull(tc)->type;
                struct Node *right = parse_compare_expression(pc);

                enum OperatorType op_type = OpNone;
                switch (op) {
                case TokOr:
                        op_type = OpOR;
                        break;
                case TokAnd:
                        op_type = OpAND;
                        break;
                default:
                        panic("Unknown operator type.", tc);
                }

                struct BinExprAST *bin_expr =
                    (struct BinExprAST *)S_malloc(sizeof(struct BinExprAST));

                bin_expr->left = node;
                bin_expr->right = right;
                bin_expr->op_type = op_type;

                node = pack(AST_BinExpr, bin_expr);
        }

        return node;
}
