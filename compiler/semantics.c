#include "token.h"
#include <type.h>
#include <semantics.h>
#include <parser.h>
#include <error.h>
#include <stdbool.h>

struct FuncData *gen_func_data(const char *func_name, struct Type *ret_type, unsigned id, bool varargs) {
        struct FuncData *data =
		(struct FuncData *)S_malloc(sizeof(struct FuncData));

	data->is_constructor = false;
        data->return_type = ret_type;
        data->func_name = func_name;
        data->id = id;
        data->varargs = varargs;
	data->arg_types = NULL;
	data->arg_count = 0;
	data->scope_data = ScopeNone;

        return data;
}

//----------------------------
//     DATA REGISTERATION
//----------------------------

struct FuncData *register_constructor_data(const char *func_name,
						  struct ParserContext *pc) {
        // id is -1 in this code but we will assign id after.
        struct FuncData *data = gen_func_data(func_name, find_type(pc, "void"), -1, false);

	data->is_constructor = true;
	data->scope_data = ScopeClass;
	
        // id assign.
        if (pc->current_class) {
                // register in class member.
                struct ClassData *current_class = pc->current_class;

                data->id = current_class->member_funcs->size + 1;

                ht_insert(current_class->member_funcs, func_name, data);

                return data;
        }

	panic("Constructor must be declared in class!", pc->tc);
	return NULL;
}

struct FuncData *register_func_data(const char *func_name,
				    struct Type *ret_type, struct ParserContext *pc) {
        // id is -1 in this code but we will assign id after.
        struct FuncData *data = gen_func_data(func_name, ret_type, -1, false);

        // id assign.
        if (pc->current_class) {
                // register in class member.
                struct ClassData *current_class = pc->current_class;

		data->scope_data = ScopeClass;
                data->id = current_class->member_funcs->size + 1;
		
                ht_insert(current_class->member_funcs, func_name, data);
        } else {
                // register in global.
		data->scope_data = ScopeGlobal;
                data->id = pc->glob_func_smtb->size + 1;

                ht_insert(pc->glob_func_smtb, func_name, data);
                pc->func_data[pc->func_data_count++] = data;

        }
	return data;
}

struct VarData *register_local_var_data(const char *name,
                                               struct Type *type,
                                               struct ParserContext *pc) {
        struct VarData *data =
		(struct VarData *)S_malloc(sizeof(struct VarData));
        data->type = type;
        data->var_name = name;
	data->scope_data = ScopeLocal;

        struct HTable *target_smtb = NULL;

        // register in pc->declared_local_vars to calcaulte total size of stack
        if (pc->declared_local_var_capacity <=
            pc->declared_local_var_count + 1) {
                pc->declared_local_var_capacity *= 2;
                pc->declared_local_vars = (struct VarData **)S_realloc(
								       pc->declared_local_vars,
								       sizeof(struct VarData **) *
								       pc->declared_local_var_capacity);
        }

        pc->declared_local_vars[pc->declared_local_var_count++] = data;

        target_smtb = pc->current_scope->local_var_smtb;

        data->id = target_smtb->size + 1;
        ht_insert(target_smtb, name, data);
	
        return data;
}

struct VarData *register_non_local_var_data(const char *name, struct Type *type,
                                         struct ParserContext *pc) {
        bool in_class = pc->current_class != NULL;
        bool in_func = pc->current_func != NULL;

        // register in member variables.
        bool member = !in_func && in_class;

        // register in global.
        bool glob = !in_func && !in_class;

        // register in local.
        bool local = in_func;

        struct VarData *data =
		(struct VarData *)S_malloc(sizeof(struct VarData));
        data->type = type;
        data->var_name = name;

        struct HTable *target_smtb = NULL;

        if (member) {
		data->scope_data = ScopeClass;
                target_smtb = pc->current_class->member_vars;
        }

        if (glob) {
		data->scope_data = ScopeGlobal;
                target_smtb = pc->glob_var_smtb;
        }

        if (local) {
                return NULL;
        }

        data->id = target_smtb->size + 1;
        ht_insert(target_smtb, name, data);

        return data;
}

struct ClassData *register_class_data(const char *class_name,
                                             struct ParserContext *pc) {
        struct ClassData *data =
		(struct ClassData *)S_malloc(sizeof(struct ClassData));
        data->id = pc->class_data_count + 1;

        data->member_funcs = gen_htable();
        data->member_vars = gen_htable();
	
	data->constructor = NULL;

        pc->class_data[pc->class_data_count++] = data;

        return data;
}

//------------------
//     DATA FIND
//------------------

static struct VarData *find_member_var_data(struct ParserContext *pc, struct ClassData *attr_of, const char *key){
	struct VarData *var_data = NULL;
	while(attr_of != NULL){
		
		if(attr_of->class_type->type_kind != TK_Class){
			panic("This type is not class. WTF", pc->tc);
		}
		
		var_data = ht_find(attr_of->member_vars, key);

		if(var_data != NULL){
			break;
		}

		if(attr_of->parent_type == NULL){
			break;
		}

		attr_of = attr_of->parent_type->data.class_data;
	}

	return var_data;
}

static struct FuncData *find_member_func_data(struct ParserContext *pc, struct ClassData *attr_of, const char *key){
	struct FuncData *func_data = NULL;
	
	while(attr_of != NULL){
		
		if(attr_of->class_type->type_kind != TK_Class){
			panic("This type is not class. WTF", pc->tc);
		}
		
		func_data = ht_find(attr_of->member_funcs, key);

		if(func_data != NULL){
			break;
		}

		if(attr_of->parent_type == NULL){
			break;
		}
		
		attr_of = attr_of->parent_type->data.class_data;
	}

	return func_data;
}

struct FuncData *find_func_data(struct ParserContext *pc,
                                       const char *func_name) {
        struct ClassData *current_class = pc->current_class;
        struct FuncData *result = NULL;

        // find in current class
        // TODO : find parent class.
        if (current_class != NULL &&
            (result = find_member_func_data(pc, current_class, func_name)) !=
	    NULL) {
                return result;
        }

        // find in global
        if ((result = ht_find(pc->glob_func_smtb, func_name)) != NULL) {
                return result;
        }

        // syscall
        if ((result = ht_find(pc->syscall_smtb, func_name)) != NULL) {
                return result;
        }

        return NULL;
}

struct ClassData *find_class_data(struct ParserContext *pc,
                                         struct Type *type) {
        if (type == NULL) {
                return NULL;
        }

        if (type->type_kind != TK_Class) {
                return NULL;
        }

        struct ClassData *class_data = (struct ClassData *)type->data.class_data;
	
        return class_data;
}

static struct VarData *find_var_data(struct ParserContext *pc, const char *var_name) {
        struct VarData *result = NULL;
	
        if (pc->current_scope != NULL) { // first find in local
                struct Scope *scope_searcher = pc->current_scope;

                while (scope_searcher != NULL) {
                        result = (struct VarData *)ht_find(
							   scope_searcher->local_var_smtb, var_name);

                        if (result != NULL)
                                break;

                        scope_searcher = scope_searcher->prev_scope;
                }
        }

        if (result == NULL && pc->current_class != NULL) { // and find in class
                result = find_member_var_data(pc, pc->current_class, var_name);
        }

        if (result == NULL) {
                result = ht_find(pc->glob_var_smtb, var_name);
        }

        return result;
}

//------------------------
//         SCOPES
//-----------------------

static struct Scope *gen_scope(struct Scope *prev_scope) {
        struct Scope *result = (struct Scope *)S_malloc(sizeof(struct Scope));

        result->local_var_smtb = gen_htable();
        result->prev_scope = prev_scope;

        return result;
}

static void open_scope(struct ParserContext *pc) {
        struct Scope *scope = gen_scope(pc->current_scope);

        pc->current_scope = scope;
}
 
static void close_scope(struct ParserContext *pc) {
        assert(pc->current_scope != NULL);

        struct Scope *prev_scope = pc->current_scope->prev_scope;

        free_htable(pc->current_scope->local_var_smtb);
        free(pc->current_scope);

        pc->current_scope = prev_scope;
}

static void check_semantics_body(struct ParserContext *pc, struct Node **body, unsigned body_count){
	int i;
	
	for(i=0; i<body_count; i++){
		check_semantics(pc, body[i]);
	}
}

static void resolve_attr(struct ParserContext *pc, struct Type *type_of_node, struct Node *node);

// check semantics of attribute node.
// attr_of will be targeted class we have to find variable or function from this class.
// and node will be attribute.
static void check_attr_semantics(struct ParserContext *pc, struct ClassData *attr_of, struct Node *node){
	struct Type *type_of_node = NULL;
	
	switch(node->type){
	case AST_Identifier:{
		struct IdentifierAST *ident_ast = (struct IdentifierAST *) node->ast;
		const char *key = ident_ast->ident->str;

		struct VarData *var_data = find_member_var_data(pc, attr_of, key);
		
		if(var_data == NULL){
			printf("%s\n", key);
			panic("Failed to find identifier in class.", pc->tc);
		}
		
		type_of_node = var_data->type;
		ident_ast->var_data = var_data;
		ident_ast->is_attr = true;

		break;
	}

	case AST_FunctionCall: {
		struct FuncCallAST *func_call_ast = (struct FuncCallAST *) node->ast;
		const char *key = func_call_ast->func_name_tok->str;
		struct FuncData *func_data = find_member_func_data(pc, attr_of, key);
		
		if(func_data == NULL){
			printf("%s %s\n", attr_of->class_type->type_str, key);
			panic("Failed to find func in class.", pc->tc);
		}

		type_of_node = func_data->return_type;
		func_call_ast->func_data = func_data;
		func_call_ast->is_attr = true;
		
		break;
	}
		
	default:
		break;
	}

	resolve_attr(pc, type_of_node, node);
}

static void resolve_attr(struct ParserContext *pc, struct Type *type_of_node, struct Node *node) {
	if(node->attr != NULL){
		if(type_of_node == NULL){
			panic("We can\'t find attribute from non-type.", pc->tc);
		}
		
		if(type_of_node->type_kind != TK_Class){
			panic("We can\'t find attribute from non-class type.", pc->tc);
		}
		
		check_attr_semantics(pc, type_of_node->data.class_data, node->attr);
	}
}

static void check_class_semantics(struct ParserContext *pc, struct ClassAST *class_ast){
	struct ClassData *class_data = class_ast->class_data;
	
	pc->current_class = class_data;
	check_semantics_body(pc, class_ast->body, class_ast->body_count);
	pc->current_class = NULL;
}

static void check_func_decl_semantics(struct ParserContext *pc, struct FuncDeclAST *func_decl_ast){
	check_semantics(pc, func_decl_ast->params);
	check_semantics_body(pc, func_decl_ast->body, func_decl_ast->body_count);
}

static void check_if_stmt_semantics(struct ParserContext *pc, struct IfStmtAST *if_stmt_ast){
	if(if_stmt_ast->cond != NULL){
		check_semantics(pc, if_stmt_ast->cond);
	}
	check_semantics_body(pc, if_stmt_ast->body, if_stmt_ast->body_count);
	check_semantics(pc, if_stmt_ast->next_stmt);
}

static void check_for_stmt_semantics(struct ParserContext *pc, struct ForStmtAST *for_stmt_ast){
	check_semantics(pc, for_stmt_ast->init);
	check_semantics(pc, for_stmt_ast->cond);
	check_semantics(pc, for_stmt_ast->step);
	check_semantics_body(pc, for_stmt_ast->body, for_stmt_ast->body_count);
}

static void check_var_decl_bundle_semantics(struct ParserContext *pc, struct VarDeclBundleAST *var_decl_bundle){

	int i;
	for(i=0; i<var_decl_bundle->var_count; i++){
		check_semantics(pc, var_decl_bundle->var_decls[i]);
	}
}

static void check_var_decl_semantics(struct ParserContext *pc, struct VarDeclAST *var_decl_ast){
	if(var_decl_ast->decl != NULL){
		check_semantics(pc, var_decl_ast->decl);

		//		push_type_match(pc, var_decl_ast->decl, var_decl_ast->var_type);
	}
}

static void check_ident_semantics(struct ParserContext *pc, struct Node *node, struct IdentifierAST *ident_ast){		
	struct Type *type = ident_ast->var_data->type;
		
	resolve_attr(pc, type, node);
}

static void check_parameter_type(struct ParserContext *pc, struct Type *arg_type, struct Node *param){
	//	push_type_match(pc, param, arg_type);
}

static void check_func_call_semantics(struct ParserContext *pc, struct Node *node, struct FuncCallAST *func_call_ast){
	struct FuncData *func_data = func_call_ast->func_data;
	struct Type *ret_type = func_data->return_type;

	if(!func_data->varargs){
		if(func_data->arg_count != func_call_ast->param_count){
			panic("Wrong parameter count of function!", pc->tc);
		}
	}

	int i;
	for(i=0; i<func_call_ast->param_count; i++){
		check_semantics(pc, func_call_ast->params[i]);
		
		if(!func_data->varargs){
			check_parameter_type(pc, func_data->arg_types[i], func_call_ast->params[i]);
		}
	}
		
	resolve_attr(pc, ret_type, node);
}

static void reset_declared_local_var_data(struct ParserContext *pc){
	// reset local var declaration.
	pc->declared_local_var_count = 0;
	pc->declared_local_var_capacity = 1;
	pc->declared_local_vars =
		(struct VarData **)S_malloc(sizeof(struct VarData *));
}

static void register_data_of_body(struct ParserContext *pc, struct Node **body, unsigned body_count){
	int i;
	open_scope(pc);
	for(i=0; i<body_count; i++){
			
		register_data(pc, body[i]);
	}
	close_scope(pc);
}

static void register_attr_data(struct ParserContext *pc, struct ClassData *attr_of, struct Node *node){
	struct Type *type_of_node = NULL;
	
	switch(node->type){
	case AST_Identifier:{
		struct IdentifierAST *ident_ast = (struct IdentifierAST *) node->ast;
		const char *key = ident_ast->ident->str;

		struct VarData *var_data = find_member_var_data(pc, attr_of, key);
		
		if(var_data == NULL){
			printf("%s\n", key);
			panic("Failed to find identifier in class.", pc->tc);
		}
		
		type_of_node = var_data->type;
		ident_ast->var_data = var_data;
		ident_ast->is_attr = true;

		break;
	}

	case AST_FunctionCall: {
		struct FuncCallAST *func_call_ast = (struct FuncCallAST *) node->ast;
		const char *key = func_call_ast->func_name_tok->str;
		struct FuncData *func_data = find_member_func_data(pc, attr_of, key);
		
		if(func_data == NULL){
			printf("%s %s\n", attr_of->class_type->type_str, key);
			panic("Failed to find func in class.", pc->tc);
		}

		type_of_node = func_data->return_type;
		func_call_ast->func_data = func_data;
		func_call_ast->is_attr = true;
		
		break;
	}
		
	default:
		break;
	}

	resolve_attr(pc, type_of_node, node);
}

void register_data(struct ParserContext *pc, struct Node *node){
	if(node == NULL){
		return;
	}
	
	switch(node->type){
	case AST_Class: {
		struct ClassAST *class_ast = (struct ClassAST *) node->ast;	

		struct Type *class_type = find_type(pc, class_ast->name_tok->str);
		struct Type *parent_type = NULL;

		if(class_ast->parent_name_tok != NULL){
			parent_type = find_type(pc, class_ast->parent_name_tok->str);
		}
		
		// Data & Type registeration
		struct ClassData *class_data = register_class_data(class_ast->name_tok->str, pc);
		class_type->data.class_data = class_data;
		
		class_data->parent_type = parent_type;
		class_data->class_type = class_type;
	
		class_ast->class_data = class_data;

		pc->current_class = class_data;

		register_data_of_body(pc, class_ast->body, class_ast->body_count);

		register_data(pc, class_ast->constructor);
		register_data(pc, class_ast->initializer);

		pc->current_class = NULL;
		
		break;
	}

	case AST_FunctionDeclaration: {
		struct FuncDeclAST *func_decl_ast = (struct FuncDeclAST *) node->ast;

		reset_declared_local_var_data(pc);

		struct Type *ret_type = find_type(pc, func_decl_ast->ret_type_tok->str);
		struct FuncData *func_data =
			register_func_data(func_decl_ast->func_name_tok->str, ret_type, pc);
		struct VarDeclBundleAST *params_ast = (struct VarDeclBundleAST *) func_decl_ast->params->ast;

		func_data->arg_types = S_malloc(params_ast->var_count * sizeof(struct Type *));
		func_data->arg_count = params_ast->var_count;
		func_decl_ast->func_data = func_data;

		pc->current_func = func_decl_ast->func_data;
		open_scope(pc);
		
		int i;
		for (i = 0; i < params_ast->var_count; i++) {
			struct VarDeclAST *param_ast =
				(struct VarDeclAST *) (params_ast->var_decls[i]->ast);

			register_data(pc, params_ast->var_decls[i]);
			func_data->arg_types[i] = find_type(pc, param_ast->var_type_tok->str);
			
		}

		register_data_of_body(pc, func_decl_ast->body, func_decl_ast->body_count);

		func_decl_ast->declared_var_count = pc->declared_local_var_count;
		func_decl_ast->declared_vars = pc->declared_local_vars;

		close_scope(pc);
		pc->current_func = NULL;
		break;
	}

	case AST_FunctionCall: {
		struct FuncCallAST *func_call_ast = (struct FuncCallAST *) node->ast;
		struct FuncData *func_data = find_func_data(pc, func_call_ast->func_name_tok->str);

		if(func_data == NULL){
			panic("Failed to find function", pc->tc);
		}

		func_call_ast->func_data = func_data;

		int i;
		for(i=0; i<func_call_ast->param_count; i++){
			register_data(pc, func_call_ast->params[i]);
		}
		
		break;
	}
		
	case AST_VariableDeclarationBundle: {
		struct VarDeclBundleAST *var_decl_bundle_ast = (struct VarDeclBundleAST *) node->ast;

		int i;
		for(i=0; i<var_decl_bundle_ast->var_count; i++){
			register_data(pc, var_decl_bundle_ast->var_decls[i]);
		}
		break;
	}

	case AST_VariableDeclaration: {
		struct VarDeclAST *var_decl_ast = (struct VarDeclAST *) node->ast;

		struct VarData *var_data = NULL;

		struct Type *var_type = find_type(pc, var_decl_ast->var_type_tok->str);
		
		if(pc->current_func != NULL){
			var_data = register_local_var_data(var_decl_ast->var_name_tok->str, var_type, pc);
		}else{
			var_data = register_non_local_var_data(var_decl_ast->var_name_tok->str, var_type, pc);
		}

		var_decl_ast->var_data = var_data;

		register_data(pc, var_decl_ast->decl);
		
		break;
	}

	case AST_Identifier: {
		struct IdentifierAST *ident_ast = (struct IdentifierAST *) node->ast;

		ident_ast->var_data = find_var_data(pc, ident_ast->ident->str);
		if(ident_ast->var_data == NULL){
			panic("Failed to find identifier", pc->tc);
		}
		if(node->attr != NULL){
			struct Type *type = ident_ast->var_data->type;
			if(type->type_kind != TK_Class){
				panic("We can\'t find attribute from non-class type.", pc->tc);
			}
			register_attr_data(pc, type->data.class_data, node->attr);
		}
		
		break;
	}

	case AST_IdentIncrease:{
		struct IdentIncreAST *ident_incre_ast = (struct IdentIncreAST *) node->ast;

		register_data(pc, ident_incre_ast->ident_node);
		
		break;
	}

	case AST_IdentDecrease:{
		struct IdentDecreAST *ident_decre_ast = (struct IdentDecreAST *) node->ast;

		register_data(pc, ident_decre_ast->ident_node);
		
		break;
	}
		
	case AST_Constructor: {
		struct ConstructorAST *constructor_ast = (struct ConstructorAST *) node->ast;
		
		reset_declared_local_var_data(pc);

		struct FuncData *func_data = register_constructor_data(constructor_ast->func_name->str, pc);
		struct VarDeclBundleAST *params_ast = (struct VarDeclBundleAST *) constructor_ast->params->ast;
		
		func_data->arg_types = S_malloc(params_ast->var_count * sizeof(struct Type *));
		func_data->arg_count = params_ast->var_count;

		int i;
		for (i = 0; i < params_ast->var_count; i++) {
			struct VarDeclAST *param_ast =
				(struct VarDeclAST *) (params_ast->var_decls[i]->ast);
			
			func_data->arg_types[i] = find_type(pc, param_ast->var_type_tok->str);
		}

		if(pc->current_class == NULL){
			panic("Constructor declared out of class.", pc->tc);
		}
		
		constructor_ast->func_data = func_data;
		
		pc->current_func = constructor_ast->func_data;

		open_scope(pc);
		for (i = 0; i < params_ast->var_count; i++) {
			register_data(pc, params_ast->var_decls[i]);
		}

		register_data_of_body(pc, constructor_ast->body, constructor_ast->body_count);

		close_scope(pc);
		pc->current_func = NULL;

		constructor_ast->declared_var_count = pc->declared_local_var_count;
		constructor_ast->declared_vars = pc->declared_local_vars;
		
		break;
	}

	case AST_IfStatement: {
		struct IfStmtAST *if_stmt_ast = (struct IfStmtAST *) node->ast;

		register_data(pc, if_stmt_ast->cond);
		register_data_of_body(pc, if_stmt_ast->body, if_stmt_ast->body_count);
		register_data(pc, if_stmt_ast->next_stmt);
		
		break;
	}

	case AST_ForStatement: {
		struct ForStmtAST *for_stmt_ast = (struct ForStmtAST *) node->ast;

		open_scope(pc);

		register_data(pc, for_stmt_ast->init);
		register_data(pc, for_stmt_ast->cond);
		register_data(pc, for_stmt_ast->step);
		register_data_of_body(pc, for_stmt_ast->body, for_stmt_ast->body_count);
		
		close_scope(pc);
		
		break;
	}

	case AST_BinExpr: {
		struct BinExprAST *bin_expr_ast = (struct BinExprAST *) node->ast;

		register_data(pc, bin_expr_ast->left);
		register_data(pc, bin_expr_ast->right);

		break;
	}

	case AST_UnaryExpr: {
		struct UnaryExprAST *unary_expr_ast = (struct UnaryExprAST *) node->ast;

		register_data(pc, unary_expr_ast->expr);
		
		break;
	}

	case AST_New: {
		struct NewAST *new_ast = (struct NewAST *) node->ast;
		struct Type *type = find_type(pc, new_ast->name_tok->str);
		new_ast->class_data = find_class_data(pc, type);
		int i;
		for(i=0; i<new_ast->param_count; i++){
			register_data(pc, new_ast->params[i]);
		}
		break;
	}
		
	default: {
		break;
	}
	}
}

void check_semantics(struct ParserContext *pc, struct Node *node){
	if(node == NULL){
		return;
	}

	switch(node->type){
	case  AST_Class:{
		struct ClassAST *class_ast = (struct ClassAST *) node->ast;
		check_class_semantics(pc, class_ast);
		break;
	}

	case AST_FunctionDeclaration:{
		struct FuncDeclAST *func_decl_ast = (struct FuncDeclAST *) node->ast;

		check_func_decl_semantics(pc, func_decl_ast);
		
		break;
	}

	case AST_IfStatement: {
		struct IfStmtAST *if_stmt_ast = (struct IfStmtAST *) node->ast;
		check_if_stmt_semantics(pc, if_stmt_ast);
		break;		
	}
		
	case AST_ForStatement: {
		struct ForStmtAST *for_stmt_ast = (struct ForStmtAST *) node->ast;
		check_for_stmt_semantics(pc, for_stmt_ast);
		break;		
	}

	case AST_VariableDeclarationBundle: {
		struct VarDeclBundleAST *var_decl_bundle = (struct VarDeclBundleAST *) node->ast;
		check_var_decl_bundle_semantics(pc, var_decl_bundle);
		break;
	}

	case AST_VariableDeclaration: {
		struct VarDeclAST *var_decl_ast = (struct VarDeclAST *) node->ast;
		check_var_decl_semantics(pc, var_decl_ast);
		break;
	}

	case AST_Identifier:{
		struct IdentifierAST *ident_ast = (struct IdentifierAST *) node->ast;
		
		check_ident_semantics(pc, node, ident_ast);
		
		break;
	}

	case AST_IdentIncrease:{
		struct IdentIncreAST *ident_incre_ast = (struct IdentIncreAST *) node->ast;

		check_semantics(pc, ident_incre_ast->ident_node);

		//struct Type *type = infer_type(pc, node);

		//		if(!(type->type_kind == TK_Numeric && type->data.numeric_data->is_integer)){
		//panic("Increasement operator (++) must be used with integer", pc->tc);
		//}
		
		break;
	}

	case AST_IdentDecrease:{
		struct IdentDecreAST *ident_decre_ast = (struct IdentDecreAST *) node->ast;

		check_semantics(pc, ident_decre_ast->ident_node);

		//struct Type *type = infer_type(pc, node);
		
		//if(!(type->type_kind == TK_Numeric && type->data.numeric_data->is_integer)){
		//panic("Decreasement operator (--) must be used with integer", pc->tc);
		//}
		
		break;
	}

	case AST_FunctionCall: {
		struct FuncCallAST *func_call_ast = (struct FuncCallAST *) node->ast;

		check_func_call_semantics(pc, node, func_call_ast);
		
		break;
	}

	case AST_BinExpr: {
		struct BinExprAST *bin_expr_ast = (struct BinExprAST *) node->ast;

		check_semantics(pc, bin_expr_ast->left);
		check_semantics(pc, bin_expr_ast->right);

		//		push_bin_type_match(pc, bin_expr_ast->left, bin_expr_ast->right);
		
		break;
	}

	case AST_UnaryExpr: {
		struct UnaryExprAST *unary_expr_ast = (struct UnaryExprAST *) node->ast;

		check_semantics(pc, unary_expr_ast->expr);
		
		break;
	}

	case AST_Return: {
		struct ReturnAST *ret_ast = (struct ReturnAST *) node->ast;

		check_semantics(pc, ret_ast->expr);
		
		break;
	}

	case AST_Constructor: {
		struct ConstructorAST *constructor_ast = (struct ConstructorAST *) node->ast;		

		constructor_ast->class_data = pc->current_class;
		pc->current_class->constructor = constructor_ast->func_data;
		
		check_semantics_body(pc, constructor_ast->body, constructor_ast->body_count);
		
		break;
	}

	case AST_New :{
		struct NewAST *new_ast = (struct NewAST *) node->ast;
		struct Type *type = find_type(pc, new_ast->name_tok->str);
		new_ast->class_data = find_class_data(pc, type);

		int i;
		for(i=0; i<new_ast->param_count; i++){
			check_semantics(pc, new_ast->params[i]);
		}
		
		break;
	}
		
	default:
		break;
	}
}
