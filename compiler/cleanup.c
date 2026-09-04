#include <parser.h>

#include <stdlib.h>

static void free_node(struct Node *node);

static void free_nodes(struct Node **nodes, unsigned count) {
    unsigned i;

    for (i = 0; i < count; i++)
        free_node(nodes[i]);
    free(nodes);
}

static void free_token(struct Token *token) {
    if (token == NULL || token->managed_by_tokenizer)
        return;
    if (token->owns_str)
        free((void *)token->str);
    free(token);
}

static void free_func_data(struct FuncData *data) {
    if (data == NULL)
        return;
    free(data->arg_types);
    free(data);
}

static void free_node(struct Node *node) {
    if (node == NULL)
        return;

    switch (node->type) {
    case AST_NumberLiteral:
    case AST_StringLiteral:
    case AST_BoolLiteral:
    case AST_Null:
        free(node->ast);
        break;
    case AST_Identifier: {
        struct IdentifierAST *ast = node->ast;
        if (ast->var_data != NULL && ast->var_data->scope_data == ScopeArray)
            free(ast->var_data);
        free(ast);
        break;
    }
    case AST_VariableDeclaration: {
        struct VarDeclAST *ast = node->ast;
        free_node(ast->decl);
        free_token(ast->var_type_tok);
        free(ast->var_data);
        free(ast);
        break;
    }
    case AST_VariableDeclarationBundle: {
        struct VarDeclBundleAST *ast = node->ast;
        free_nodes(ast->var_decls, (unsigned)ast->var_count);
        free(ast);
        break;
    }
    case AST_BinExpr: {
        struct BinExprAST *ast = node->ast;
        free_node(ast->left);
        free_node(ast->right);
        free(ast);
        break;
    }
    case AST_IfStatement: {
        struct IfStmtAST *ast = node->ast;
        free_node(ast->cond);
        free_nodes(ast->body, ast->body_count);
        free_node(ast->next_stmt);
        free(ast);
        break;
    }
    case AST_UnaryExpr: {
        struct UnaryExprAST *ast = node->ast;
        free_node(ast->expr);
        free(ast);
        break;
    }
    case AST_FunctionDeclaration: {
        struct FuncDeclAST *ast = node->ast;
        free_node(ast->params);
        free_nodes(ast->body, ast->body_count);
        free(ast->declared_vars);
        free_token(ast->ret_type_tok);
        free_func_data(ast->func_data);
        free(ast);
        break;
    }
    case AST_FunctionCall: {
        struct FuncCallAST *ast = node->ast;
        free_nodes(ast->params, (unsigned)ast->param_count);
        if (ast->func_data != NULL && ast->func_data->scope_data == ScopeArray)
            free_func_data(ast->func_data);
        free(ast);
        break;
    }
    case AST_ForStatement: {
        struct ForStmtAST *ast = node->ast;
        free_node(ast->init);
        free_node(ast->cond);
        free_node(ast->step);
        free_nodes(ast->body, ast->body_count);
        free(ast);
        break;
    }
    case AST_IdentIncrease: {
        struct IdentIncreAST *ast = node->ast;
        free_node(ast->ident_node);
        free(ast);
        break;
    }
    case AST_IdentDecrease: {
        struct IdentDecreAST *ast = node->ast;
        free_node(ast->ident_node);
        free(ast);
        break;
    }
    case AST_Return: {
        struct ReturnAST *ast = node->ast;
        free_node(ast->expr);
        free(ast);
        break;
    }
    case AST_Class: {
        struct ClassAST *ast = node->ast;
        free_nodes(ast->body, ast->body_count);
        free(ast);
        break;
    }
    case AST_Constructor: {
        struct ConstructorAST *ast = node->ast;
        free_node(ast->params);
        free_nodes(ast->body, ast->body_count);
        free(ast->declared_vars);
        free_func_data(ast->func_data);
        free(ast);
        break;
    }
    case AST_New: {
        struct NewAST *ast = node->ast;
        free_nodes(ast->params, (unsigned)ast->param_count);
        free(ast);
        break;
    }
    case AST_ArrayDeclaration: {
        struct ArrayDeclAST *ast = node->ast;
        free_nodes(ast->elements, (unsigned)ast->element_count);
        free_token(ast->ele_type_tok);
        free(ast);
        break;
    }
    case AST_ArrayAccess: {
        struct ArrayAccessAST *ast = node->ast;
        free_node(ast->target_array);
        free_nodes(ast->indexes, (unsigned)ast->access_count);
        free(ast);
        break;
    }
    case AST_Negative: {
        struct NegAST *ast = node->ast;
        free_node(ast->ast);
        free(ast);
        break;
    }
    }

    free_node(node->attr);
    free(node);
}

static void free_types(struct HTable *table, bool class_types) {
    int i;

    for (i = 0; i < HTABLE_BUFF; i++) {
        struct DataNode *node = table->bucket[i];
        while (node != NULL) {
            struct Type *type = node->ptr;
            if (type->type_kind == TK_Numeric)
                free(type->data.numeric_data);
            if (type->type_kind == TK_Array || class_types)
                free((void *)type->type_str);
            free(type);
            node = node->next;
        }
    }
    free_htable(table);
}

void free_pc(struct ParserContext *pc) {
    unsigned i;

    if (pc == NULL)
        return;
    free_nodes(pc->nodes, pc->node_count);
    for (i = 0; i < pc->class_data_count; i++) {
        free_htable(pc->class_data[i]->member_vars);
        free_htable(pc->class_data[i]->member_funcs);
        free(pc->class_data[i]);
    }
    for (i = 0; i < HTABLE_BUFF; i++) {
        struct DataNode *node = pc->syscall_smtb->bucket[i];
        while (node != NULL) {
            free_func_data(node->ptr);
            node = node->next;
        }
    }
    free_htable(pc->syscall_smtb);
    free_htable(pc->glob_var_smtb);
    free_htable(pc->glob_func_smtb);
    free_types(pc->primitive_type_smtb, false);
    free_types(pc->class_type_smtb, true);
    free(pc->numeric_type_array);
    free_queue(pc->first_pass_queue);
    free_queue(pc->second_pass_queue);
    free(pc);
}
