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
	irc->label_id = 0;
        irc->str_rodata = gen_htable();

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
                emit_byte(irc, ch);

                i++;
        }

        // emit null character
        emit_byte(irc, 0x00);
}

// uint will be stored as little endian.
static void emit_int(struct IRContext *irc, int si) {
        int i;
        for (i = 0; i < sizeof(int); i++) {
                emit_byte(irc, (si & 0xFF));
                si >>= 8;
        }
}

static void emit_float(struct IRContext *irc, float f) {
        unsigned fb; // bit data of float.
        memcpy(&fb, &f, sizeof(float));
        int i;
        for (i = 0; i < sizeof(float); i++) {
                emit_byte(irc, (fb & 0xFF));
                fb >>= 8;
        }
}

static void gen_func_metadata(struct IRContext *irc, struct ParserContext *pc,
                              struct FuncData *fd) {
	if(fd->is_constructor){
		emit_byte(irc, META_CONSTRUCTOR);
	}else{
		emit_byte(irc, META_FUNC);
	}

        emit_int(irc, fd->id);
        emit_str(irc, fd->func_name);

	if(!fd->is_constructor){
		emit_str(irc, fd->return_type);
	}
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

        emit_byte(irc, META_BEGIN);

        int i;
        for (i = 0; i < pc->class_data_count; i++) {
                struct ClassData *cd = pc->class_data[i];

                gen_class_metadata(irc, pc, cd);
        }

        for (i = 0; i < pc->func_data_count; i++) {
                struct FuncData *fd = pc->func_data[i];

                gen_func_metadata(irc, pc, fd);
        }

        emit_byte(irc, META_END);
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

static unsigned get_total_stack_size_of_func(struct ParserContext *pc,
                                             struct FuncDeclAST *func_decl) {
        int i;
        unsigned stack_offset = 0;

        for (i = 0; i < func_decl->declared_var_count; i++) {
                struct VarData *var_data = func_decl->declared_vars[i];
                unsigned data_size = get_size_of_type(pc, var_data->type);
		
                var_data->offset = stack_offset;

                stack_offset += data_size;
        }

        return stack_offset;
}

static struct RODATA_Str *gen_str_rodata(unsigned id, const char *str) {
        struct RODATA_Str *result =
		(struct RODATA_Str *)S_malloc(sizeof(struct RODATA_Str));

        result->id = id;
        result->str = str;

        return result;
}

static struct RODATA_Str *add_str_rodata(struct IRContext *irc,
                                         const char *str) {

	struct RODATA_Str *rodata_str = (struct RODATA_Str *) ht_find(irc->str_rodata, str);

	if(rodata_str == NULL){
		rodata_str = gen_str_rodata(irc->str_rodata->size, str);
		ht_insert(irc->str_rodata, str, rodata_str);
	}
	
        return rodata_str;
}

static byte get_op_byte(struct ParserContext *pc, enum OperatorType op_type){
	byte op_byte = 0x00;

	switch(op_type){
	case OpADD:{
		op_byte = OP_ADD;
		break;
	}
			
	case OpSUB:{
		op_byte = OP_SUB;
		break;
	}
				
	case OpMUL: {
		op_byte = OP_MUL;
		break;
	}

	case OpDIV:{
		op_byte = OP_DIV;
		break;
	}
			
	case OpEQUAL: {
		op_byte = OP_EQUAL;
		break;
	}

	case OpNOTEQUAL: {
		op_byte = OP_NOTEQUAL;
		break;
	}

	case OpGREATER: {
		op_byte = OP_GREATER;
		break;
	}
			
	case OpLESS: {
		op_byte = OP_LESS;
		break;
	}
			
	case OpEQUALGREATER: {
		op_byte = OP_EQUALGREATER;
		break;
	}

	case OpEQUALLESS: {
		op_byte = OP_EQUALLESS;
		break;
	}
	case OpASSIGN: {
		op_byte = OP_ASSIGN;
		break;
	}
			
	case OpOR: {
		op_byte = OP_OR;
		break;
	}
			 
	case OpAND: {
		op_byte = OP_AND;
		break;
	}
			
	default: {
		panic("Unknown Op Type", pc->tc);
		break;
	}
	}

	assert(op_byte != 0x00);

	return op_byte;
}

static void gen_node_ir(struct IRContext *irc, struct ParserContext *pc,
                        struct Node *node);

static void gen_ident_load_ir(struct IRContext *irc, struct ParserContext *pc, enum ScopeData scope_data, unsigned offset, unsigned size) {
	switch(scope_data){
	case ScopeLocal:
		emit_byte(irc, OP_SP_LOAD);
		break;

	case ScopeGlobal:
		emit_byte(irc, OP_LOAD_GLOBAL);
		break;

	case ScopeClass:
		emit_byte(irc, OP_LOAD_CLASS);
		break;

	default:
		panic("Wrong scope of variable!", pc->tc);
		break;
	}
	
	emit_int(irc, offset);
	emit_int(irc, size);
}

static void gen_ident_incre_ir(struct IRContext *irc, struct ParserContext *pc, enum ScopeData scope_data, unsigned offset, unsigned size) {
	switch(scope_data){
	case ScopeLocal:
		emit_byte(irc, OP_SP_INCRE);
		break;

	case ScopeGlobal:
		emit_byte(irc, OP_INCRE_GLOBAL);
		break;

	case ScopeClass:
		emit_byte(irc, OP_INCRE_CLASS);
		break;

	default:
		panic("Wrong scope of variable!", pc->tc);
		break;
	}
	
	emit_int(irc, offset);
	emit_int(irc, size);
}

static void gen_ident_decre_ir(struct IRContext *irc, struct ParserContext *pc, enum ScopeData scope_data, unsigned offset, unsigned size) {
	switch(scope_data){
	case ScopeLocal:
		emit_byte(irc, OP_SP_DECRE);
		break;

	case ScopeGlobal:
		emit_byte(irc, OP_DECRE_GLOBAL);
		break;

	case ScopeClass:
		emit_byte(irc, OP_DECRE_CLASS);
		break;

	default:
		panic("Wrong scope of variable!", pc->tc);
		break;
	}

	emit_int(irc, offset);
	emit_int(irc, size);
}

static void gen_attr_load_ir(struct IRContext *irc, struct ParserContext *pc, unsigned offset, unsigned size){
	emit_byte(irc, OP_LOAD_ATTR);
	emit_int(irc, offset);
	emit_int(irc, size);
}

static void gen_attr_incre_ir(struct IRContext *irc, struct ParserContext *pc, unsigned offset, unsigned size){
	emit_byte(irc, OP_INCRE_ATTR);
	emit_int(irc, offset);
	emit_int(irc, size);
}

static void gen_attr_decre_ir(struct IRContext *irc, struct ParserContext *pc, unsigned offset, unsigned size){
	emit_byte(irc, OP_DECRE_ATTR);
	emit_int(irc, offset);
	emit_int(irc, size);
}

static void gen_ident_ir(struct IRContext *irc, struct ParserContext *pc, struct Node *node){

	struct IdentifierAST *ident_ast = NULL;

	switch(node->type){
	case AST_Identifier:{
		ident_ast = ((struct IdentifierAST *) node->ast);
		break;
	}

	case AST_IdentIncrease:{
		struct IdentIncreAST *ident_incre_ast = ((struct IdentIncreAST *) node->ast);
		ident_ast = (struct IdentifierAST *) ident_incre_ast->ident_node->ast;
		break;
	}

	case AST_IdentDecrease:{
		struct IdentDecreAST *ident_decre_ast = ((struct IdentDecreAST *) node->ast);
		ident_ast = (struct IdentifierAST *) ident_decre_ast->ident_node->ast;
		break;
	}

	default:{
	}
	}

	assert(ident_ast != NULL);

	struct VarData *var_data = ident_ast->var_data;
	assert(var_data != NULL);
	
	unsigned size = get_size_of_type(pc, var_data->type);
	unsigned offset = var_data->offset;
	
	switch(node->type){
	case AST_Identifier:{
		
		if(ident_ast->is_attr){
			gen_attr_load_ir(irc, pc, offset, size);
		} else {
			gen_ident_load_ir(irc, pc, var_data->scope_data, offset, size);
		}
		break;
	}

	case AST_IdentIncrease:{
		
		
		if(ident_ast->is_attr){
			gen_attr_incre_ir(irc, pc, offset, size);
		} else {
			gen_ident_incre_ir(irc, pc, var_data->scope_data, offset, size);
		}
		break;
	}

	case AST_IdentDecrease:{
		if(ident_ast->is_attr){
			gen_attr_decre_ir(irc, pc, offset, size);
		} else {
			gen_ident_decre_ir(irc, pc, var_data->scope_data, offset, size);
		}
		break;
	}

	default:{
		panic("in gen_ident_ir function, this node is not identifier.", pc->tc);
	}
	}
	
	if(node->attr != NULL){
		gen_node_ir(irc, pc, node->attr);
	}
}

static void gen_node_ir(struct IRContext *irc, struct ParserContext *pc,
                        struct Node *node) {
        switch (node->type) {

	case AST_Null: {
		emit_byte(irc, OP_PUSH_NULL);
		
		break;
	}

	case AST_Identifier:
	case AST_IdentIncrease:
	case AST_IdentDecrease:{
		gen_ident_ir(irc, pc, node);
		
		break;
	}
		
        case AST_StringLiteral: {
                struct StringLiteralAST *str_lit_ast =
			(struct StringLiteralAST *)node->ast;

                struct RODATA_Str *rodata =
			add_str_rodata(irc, str_lit_ast->str_tok->str);
		
                emit_byte(irc, OP_LOAD_STR);
                emit_int(irc, rodata->id);

                break;
        }

	case AST_VariableDeclarationBundle: {
		struct VarDeclBundleAST *var_decl_bundle_ast = (struct VarDeclBundleAST *)node->ast;
		
		int i;
		for(i=0; i<var_decl_bundle_ast->var_count; i++){
			gen_node_ir(irc, pc, var_decl_bundle_ast->var_decls[i]);
		}
		
		break;
	}

	case AST_VariableDeclaration: {
		struct VarDeclAST *var_decl_ast = (struct VarDeclAST *) node->ast;

		gen_node_ir(irc, pc, var_decl_ast->decl);
		
		// case for local var data
		if(var_decl_ast->local_var_data != NULL){
			struct VarData *var_data = var_decl_ast->local_var_data;
			
			emit_byte(irc, OP_SP_SAVE); // stack point load
			emit_int(irc, var_data->offset);

			break;
		}

		// global or class attribute
		
		break;
	}
		
	case AST_BinExpr: {
		struct BinExprAST *bin_expr_ast = (struct BinExprAST *) node->ast;

		if(bin_expr_ast->op_type == OpASSIGN){
			
		}else{
			gen_node_ir(irc, pc, bin_expr_ast->left);
			gen_node_ir(irc, pc, bin_expr_ast->right);
		
			emit_byte(irc, OP_EXPR_OP);
			byte op_byte = get_op_byte(pc, bin_expr_ast->op_type);
			emit_byte(irc, op_byte);
		}
		break;
	}
		
        case AST_NumberLiteral: {
                struct NumberLiteralAST *num_lit_ast =
			(struct NumberLiteralAST *)node->ast;
		bool valid_number = false;
		
                if (num_lit_ast->is_integer) {
			if(num_lit_ast->byte == 4){
				emit_byte(irc, OP_LDC_I4);
				valid_number = true;
				emit_int(irc, atoi(num_lit_ast->num_tok->str));
			}
                } else { // floating point.
			if(num_lit_ast->byte == 4){
				emit_byte(irc, OP_LDC_F4);
				valid_number = true;
				emit_float(irc, atof(num_lit_ast->num_tok->str));
			}
			if(num_lit_ast->byte == 8){
				emit_byte(irc, OP_LDC_F8);
				valid_number = true;
				// emit_double
			}
		}

		if(!valid_number)
			panic("Failed to load number literal, invalid type and size", pc->tc);
		
                break;
        }

	case AST_ForStatement: {
		struct ForStmtAST *for_stmt_ast = (struct ForStmtAST *) node->ast;

		gen_node_ir(irc, pc, for_stmt_ast->init);

		irc->label_id++;
		int end_label_id = irc->label_id;
		irc->label_id++;
		int begin_label_id = irc->label_id;

		emit_byte(irc, OP_GOTO);
		emit_int(irc, end_label_id);

		emit_byte(irc, OP_LABEL);
		emit_int(irc, begin_label_id);

		int i;
		for(i=0; i<for_stmt_ast->body_count; i++){
			gen_node_ir(irc, pc, for_stmt_ast->body[i]);
		}

		gen_node_ir(irc, pc, for_stmt_ast->step);

		emit_byte(irc, OP_LABEL);
		emit_int(irc, end_label_id);

		gen_node_ir(irc, pc, for_stmt_ast->cond);

		emit_byte(irc, OP_JE);
		emit_int(irc, begin_label_id);

		break;
	}
		
        case AST_FunctionCall: {
                struct FuncCallAST *func_call_ast =
			(struct FuncCallAST *)node->ast;

                struct FuncData *func_data = func_call_ast->func_data;

                if (!func_data->varargs) {
                        assert(func_data->arg_count ==
                               func_call_ast->param_count);
                }

                int i;
                for (i = 0; i < func_call_ast->param_count; i++) {
                        gen_node_ir(irc, pc, func_call_ast->params[i]);
                }

		enum ScopeData scope_data = func_data->scope_data;
		
		if(func_call_ast->is_attr){
			emit_byte(irc, OP_CALL_ATTR);
		}else{
		switch(scope_data){
		case ScopeGlobal:
			emit_byte(irc, OP_CALL_GLOBAL);
			break;

		case ScopeClass:
			emit_byte(irc, OP_CALL_CLASS);
			break;
			
		case ScopeSyscall:
			emit_byte(irc, OP_SYSCALL);
			break;

		default:
			panic("Wrong scope of variable!", pc->tc);
			break;
		}
		}
		
                emit_int(irc, func_data->id);
                emit_int(irc, func_call_ast->param_count);

                break;
        }

	case AST_Class: {
		struct ClassAST *class_ast = (struct ClassAST *)node->ast;
		struct ClassData *class_data = class_ast->class_data;
		
		emit_byte(irc, CODE_CLASS);

		emit_int(irc, class_data->id);

		int i;
		for(i=0; i<class_ast->body_count; i++){
			gen_node_ir(irc, pc, class_ast->body[i]);
		}
		
		emit_byte(irc, CODE_TERM);
		
		break;
	}

	case AST_Return: {
		struct ReturnAST *ret_ast = (struct ReturnAST *) node->ast;

		gen_node_ir(irc, pc, ret_ast->expr);
		
		emit_byte(irc, OP_RET_VAL);
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

                if (total_stack_size != 0) {
                        emit_byte(irc, OP_SP_PUSH);
                        emit_int(irc, total_stack_size);
                }

                int i;
                for (i = 0; i < func_decl_ast->body_count; i++) {
                        struct Node *body_node = func_decl_ast->body[i];

                        gen_node_ir(irc, pc, body_node);
                }

                if (total_stack_size != 0) {
                        emit_byte(irc, OP_SP_POP);
                        emit_int(irc, total_stack_size);
                }

                emit_byte(irc, OP_RET);

                emit_byte(irc, CODE_TERM);

                break;
        }

        default: {
                printf("Unknown ast type : %d\n", node->type);
                break;
        }
        }
}

static void gen_rodata(struct IRContext *irc, struct ParserContext *pc) {
        emit_byte(irc, RODATA_BEGIN);

	int i;
	for(i=0; i<HTABLE_BUFF; i++){
		struct DataNode *data_node = irc->str_rodata->bucket[i];

		while(data_node != NULL){
			struct RODATA_Str *rodata_str = (struct RODATA_Str *) data_node->ptr;
			
			emit_byte(irc, RODATA_STR);
			emit_int(irc, rodata_str->id);
			emit_str(irc, rodata_str->str);

			data_node = data_node->next;
		}
	}

        emit_byte(irc, RODATA_END);
}

void gen_ir(struct IRContext *irc, struct ParserContext *pc) {
        gen_metadata(irc, pc);

        emit_byte(irc, CODE_BEGIN);

        int i;
        for (i = 0; i < pc->node_count; i++) {
                struct Node *node = pc->nodes[i];

                gen_node_ir(irc, pc, node);
        }

        emit_byte(irc, CODE_END);

        gen_rodata(irc, pc);
}
