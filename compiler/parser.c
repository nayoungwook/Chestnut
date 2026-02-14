#include "token.h"
#include "type.h"
#include "util.h"

#include <error.h>
#include <parser.h>

static struct Node *parse_term(struct ParserContext *pc);
static struct Node *parse_simple_expression(struct ParserContext *pc);
static struct Node *parse_unary_expression(struct ParserContext *pc);
static struct Node *parse_compare_expression(struct ParserContext *pc);
static struct Node *parse_expression(struct ParserContext *pc);

static struct FuncData *gen_func_data(const char *func_name,
                                      const char *ret_type, unsigned id,
                                      bool varargs);

static void init_primitive(struct ParserContext *pc) {
        // char < int < uint < float < double
        ht_insert(pc->primitive_type_smtb, "int",
                  gen_primitive_type("int", 4, 2, true));

        ht_insert(pc->primitive_type_smtb, "char",
                  gen_primitive_type("char", 2, 1, false));

        ht_insert(pc->primitive_type_smtb, "float",
                  gen_primitive_type("float", 4, 4, true));
        ht_insert(pc->primitive_type_smtb, "double",
                  gen_primitive_type("double", 8, 5, true));

        ht_insert(pc->primitive_type_smtb, "bool",
                  gen_primitive_type("bool", 1, -1, false));
        ht_insert(pc->primitive_type_smtb, "void",
                  gen_primitive_type("void", 0, -1, false));
}

static void init_syscall(struct ParserContext *pc) {
        ht_insert(pc->syscall_smtb, "print",
                  gen_func_data("print", "void", 0, true));
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

        pc->class_data_count = 0;
        pc->func_data_count = 0;

        pc->nodes = NULL;
        pc->node_size = 0;
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
                if (pc->node_size + 1 >= pc->node_capacity) {
                        pc->node_capacity *= 2;
                        pc->nodes = (struct Node **)S_realloc(
                            pc->nodes,
                            sizeof(struct Node *) * pc->node_capacity);
                }

                pc->nodes[pc->node_size++] = node;
        }

        pc->tc = NULL;
}

static struct Node *pack(enum ASTType type, void *ptr) {
        struct Node *result = (struct Node *)S_malloc(sizeof(struct Node));

        result->ast = ptr;
        result->type = type;

        return result;
}

static void parse_func_call_params(struct ParserContext *pc,
                                   struct FuncCallAST *func_call) {
        struct TokenizerContext *tc = pc->tc;

        unsigned capacity = 1, size = 0;
        struct Node **params =
            (struct Node **)S_malloc(sizeof(struct Node *) * capacity);

        consume(tc, TokLParen);

        while (peek(tc)->type != TokRParen) {
                struct Node *expr = parse_expression(pc);

                if (size + 1 >= capacity) {
                        capacity *= 2;
                        params = (struct Node **)S_realloc(
                            params, sizeof(struct Node *) * capacity);
                }

                params[size++] = expr;

                if (peek(tc)->type == TokComma) {
                        consume(tc, TokComma);
                }
        }

        func_call->params = params;
        func_call->param_size = size;

        consume(tc, TokRParen);
}

static struct VarData *find_var_data(struct ParserContext *pc,
                                     const char *var_name) {
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
                result = (struct VarData *)ht_find(
                    pc->current_class->member_vars, var_name);
        }

        if (result == NULL) {
                result = ht_find(pc->glob_var_smtb, var_name);
        }

        return result;
}

static struct FuncData *find_func_data(struct ParserContext *pc,
                                       const char *func_name) {
        struct ClassData *current_class = pc->current_class;
        struct FuncData *result = NULL;

        // find in current class
        // TODO : find parent class.
        if (current_class != NULL &&
            (result = ht_find(current_class->member_funcs, func_name)) !=
                NULL) {
                return result;
        }

        // find in global
        if ((result = ht_find(pc->glob_func_smtb, func_name)) != NULL) {
                return result;
        }

        // syscall
        if ((result = ht_find(pc->syscall_smtb, func_name)) != NULL) {
                result->is_syscall = true;
                return result;
        }

        return NULL;
}

static struct ClassData *find_class_data(struct ParserContext *pc,
                                         const char *class_name) {
        struct Type *type = find_type(pc, class_name);

        if (type == NULL) {
                return NULL;
        }

        if (type->data == NULL) {
                return NULL;
        }

        struct ClassData *class_data = (struct ClassData *)type->data;

        return class_data;
}

static struct Node *gen_func_call_node(struct Token *first,
                                       struct ParserContext *pc, bool is_expr) {
        struct FuncCallAST *func_call =
            (struct FuncCallAST *)S_malloc(sizeof(struct FuncCallAST));

        parse_func_call_params(pc, func_call);

        func_call->func_name_tok = first;

        return pack(AST_FunctionCall, func_call);
}

static struct Node *gen_ident_node(struct Token *first,
                                   struct ParserContext *pc,
                                   struct ClassData *attr_of, bool is_expr);

static bool check_attr(struct ClassData *class_data, struct Node *node) {
        if (node->type == AST_Identifier) {
                struct IdentifierAST *ident_ast =
                    (struct IdentifierAST *)node->ast;

                return ht_find(class_data->member_vars,
                               ident_ast->ident->str) != NULL;
        }

        if (node->type == AST_FunctionCall) {
                struct FuncCallAST *func_call_ast =
                    (struct FuncCallAST *)node->ast;

                return ht_find(class_data->member_funcs,
                               func_call_ast->func_name_tok->str) != NULL;
        }

        return false;
}

static void parse_attribute(struct ParserContext *pc,
                            struct ClassData *target_class,
                            struct Node *ident_node, bool is_expr) {
        struct TokenizerContext *tc = pc->tc;

        assert(target_class != NULL);

        struct Node *attr = gen_ident_node(pull(tc), pc, target_class, is_expr);
        ident_node->attr = attr;

        if (!check_attr(target_class, attr)) {
                const char *ident_str = "";
                if (attr->type == AST_Identifier) {
                        ident_str =
                            ((struct IdentifierAST *)attr->ast)->ident->str;
                }

                if (attr->type == AST_FunctionCall) {
                        ident_str = ((struct FuncCallAST *)attr->ast)
                                        ->func_name_tok->str;
                }

                char error_buff[512];
                sprintf(error_buff,
                        "Failed to find attribute \"%s\" from \"%s\"",
                        ident_str, target_class->class_name);
                panic(error_buff, tc);
        }
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
        consume(tc, TokSemiColon);

        struct BinExprAST *bin_expr_ast =
            (struct BinExprAST *)S_malloc(sizeof(struct BinExprAST));

        bin_expr_ast->left = result;
        bin_expr_ast->opType = OpASSIGN;
        bin_expr_ast->right = expr;

        //	q_push(tcc->tc_assign_queue, gen_assign_tcqn(pc,
        // bin_expr_ast->left,
        // bin_expr_ast->right));

        return pack(AST_BinExpr, bin_expr_ast);
}

static struct ClassData *get_ident_class_data(struct ParserContext *pc,
                                              struct ClassData *attr_of,
                                              struct Node *ident_node) {
        struct ClassData *target_class = NULL;
        struct IdentifierAST *ident_ast =
            (struct IdentifierAST *)ident_node->ast;

        struct VarData *var_data = NULL;
        struct TokenizerContext *tc = pc->tc;

        if (attr_of == NULL) {
                // if attr_of is null, we can directly access to variable so
                // find_var_data
                var_data = find_var_data(pc, ident_ast->ident->str);
        } else {
                // if attr_of is not null, it means we have to check attribute
                // of attr_of so find htable of member_vars
                var_data = (struct VarData *)ht_find(attr_of->member_vars,
                                                     ident_ast->ident->str);
        }

        if (var_data == NULL) {
                char error_buff[512];
                if (attr_of == NULL) {
                        sprintf(error_buff, "Failed to find variable %s",
                                ident_ast->ident->str);
                }

                if (attr_of != NULL) {
                        sprintf(error_buff,
                                "Failed to find member variable %s of "
                                "\"%s\"",
                                ident_ast->ident->str, attr_of->class_name);
                }

                panic(error_buff, tc);
        }

        target_class = find_class_data(pc, var_data->type);

        return target_class;
}

static struct ClassData *get_func_call_class_data(struct ParserContext *pc,
                                                  struct ClassData *attr_of,
                                                  struct Node *ident_node) {
        struct FuncCallAST *func_call_ast =
            (struct FuncCallAST *)ident_node->ast;
        struct FuncData *func_data = NULL;

        struct ClassData *target_class = NULL;
        struct TokenizerContext *tc = pc->tc;

        if (attr_of == NULL) {
                func_data =
                    find_func_data(pc, func_call_ast->func_name_tok->str);
        } else {
                func_data = (struct FuncData *)ht_find(
                    attr_of->member_funcs, func_call_ast->func_name_tok->str);
        }

        if (func_data == NULL) {
                char error_buff[512];
                if (attr_of == NULL) {
                        sprintf(error_buff, "Failed to find function %s",
                                func_call_ast->func_name_tok->str);
                }

                if (attr_of != NULL) {
                        sprintf(error_buff,
                                "Failed to find member function %s of "
                                "\"%s\"",
                                func_call_ast->func_name_tok->str,
                                attr_of->class_name);
                }

                panic(error_buff, tc);
        }

        func_call_ast->func_data = func_data;

        target_class = find_class_data(pc, func_data->return_type);

        return target_class;
}

static struct ClassData *get_class_data_of_ident_node(struct ParserContext *pc,
                                                      struct ClassData *attr_of,
                                                      struct Node *ident_node) {
        struct ClassData *target_class = NULL;

        if (ident_node->type == AST_Identifier) {
                target_class = get_ident_class_data(pc, attr_of, ident_node);
        }

        if (ident_node->type == AST_FunctionCall) {
                target_class =
                    get_func_call_class_data(pc, attr_of, ident_node);
        }

        return target_class;
}

static struct Node *gen_ident_node(struct Token *first,
                                   struct ParserContext *pc,
                                   struct ClassData *attr_of, bool is_expr) {
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

        bool is_end_of_statement = attr_of == NULL && !is_expr;

        attr_of = get_class_data_of_ident_node(pc, attr_of, result);

        if (peek(tc)->type == TokDot) {
                consume(tc, TokDot);

                parse_attribute(pc, attr_of, result, is_expr);
        }

        result = check_assign(pc, result);

        if (is_end_of_statement) {
                consume(tc, TokSemiColon);
        }

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
                var_decl->var_type_tok = type_tok;

                if (!check_type_existance(pc, type_tok->str)) {
                        char error_buff[512];
                        sprintf(error_buff, "Failed to find type %s",
                                type_tok->str);
                        panic(error_buff, tc);
                }

                if (param_size + 1 >= capacity) {
                        capacity *= 2;
                        result->var_decls = S_realloc(
                            result->var_decls,
                            sizeof(struct VarDeclBundleAST *) * capacity);
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

static struct Node **gen_body(struct ParserContext *pc, unsigned *body_size) {
        struct TokenizerContext *tc = pc->tc;

        consume(tc, TokLBracket);
        open_scope(pc);

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
        close_scope(pc);

        *body_size = size;

        return result;
}

static struct FuncData *gen_func_data(const char *func_name,
                                      const char *ret_type, unsigned id,
                                      bool varargs) {
        struct FuncData *data =
            (struct FuncData *)S_malloc(sizeof(struct FuncData));
        data->return_type = ret_type;
        data->func_name = func_name;
        data->id = id;
        data->varargs = varargs;
        data->is_syscall = false;

        return data;
}

static struct FuncData *register_func_data(const char *func_name,
                                           const char *ret_type,
                                           struct ParserContext *pc) {

        // id is -1 in this code but we will assign id after.
        struct FuncData *data = gen_func_data(func_name, ret_type, -1, false);

        // id assign.
        if (pc->current_class) {
                // register in class member.
                struct ClassData *current_class = pc->current_class;

                data->id = current_class->member_funcs->size + 1;

                ht_insert(current_class->member_funcs, func_name, data);

                return data;
        } else {
                // register in global.
                data->id = pc->glob_func_smtb->size + 1;

                ht_insert(pc->glob_func_smtb, func_name, data);
                pc->func_data[pc->func_data_count++] = data;

                return data;
        }
}

static struct Node *gen_func_decl_node(struct Token *first,
                                       struct ParserContext *pc) {
        struct FuncDeclAST *func_decl =
            (struct FuncDeclAST *)S_malloc(sizeof(struct FuncDeclAST));

        struct TokenizerContext *tc = pc->tc;
        unsigned body_size = 0;

        // reset local var declaration.
        pc->declared_local_var_count = 0;
        pc->declared_local_var_capacity = 1;
        pc->declared_local_vars =
            (struct VarData **)S_malloc(sizeof(struct VarData *));

        struct Token *func_name_tok = pull(tc);

        struct Node *params = gen_func_param_node(pc);

        assert(params->type == AST_VariableDeclarationBundle);

        consume(tc, TokColon);

        struct Token *ret_type_tok = pull(tc);

        if (!check_type_existance(pc, ret_type_tok->str)) {
                char error_buff[512];
                sprintf(error_buff, "Failed to find type %s",
                        ret_type_tok->str);
                panic(error_buff, tc);
        }

        func_decl->func_name_tok = func_name_tok;
        func_decl->ret_type_tok = ret_type_tok;
        func_decl->params = params;

        struct Node *result = pack(AST_FunctionDeclaration, func_decl);

        struct FuncData *func_data = find_func_data(pc, func_name_tok->str);

        func_decl->func_data = func_data;

        pc->current_func = func_data;
        func_decl->body = gen_body(pc, &body_size);
        pc->current_func = NULL;

        func_decl->body_size = body_size;

        // register local var data to calcaulte stack size after.
        func_decl->declared_var_count = pc->declared_local_var_count;
        func_decl->declared_vars = pc->declared_local_vars;

        return result;
}

static struct VarData *register_local_var_data(const char *name,
                                               const char *type,
                                               struct ParserContext *pc) {
        struct VarData *data =
            (struct VarData *)S_malloc(sizeof(struct VarData));
        data->type = type;
        data->var_name = name;

        struct HTable *target_smtb = NULL;

        // register in pc->declared_local_vars to calcaulte total size of stack
        if (pc->declared_local_var_capacity >=
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

static struct VarData *register_var_data(const char *name, const char *type,
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
                target_smtb = pc->current_class->member_vars;
        }

        if (glob) {
                target_smtb = pc->glob_var_smtb;
        }

        if (local) {
                // errror
        }

        assert(target_smtb != NULL);

        data->id = target_smtb->size + 1;
        ht_insert(target_smtb, name, data);

        return data;
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

                if (!check_type_existance(pc, var_type_tok->str)) {
                        char error_buff[512];
                        sprintf(error_buff, "Failed to find type %s",
                                var_type_tok->str);
                        panic(error_buff, tc);
                }

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
                var_decl->var_name_tok = var_name_tok;
                var_decl->var_type_tok = var_type_tok;
                var_decl->decl = decl;
                var_decl->ac_mod = ACMOD_DEFAULT;

                if (var_count + 1 >= capacity) {
                        capacity *= 2;
                        result->var_decls = (struct Node **)S_realloc(
                            result->var_decls,
                            sizeof(struct Node *) * capacity);
                }

                struct Node *node = pack(AST_VariableDeclaration, var_decl);
                result->var_decls[var_count++] = node;

                bool in_func = pc->current_func != NULL;

                bool local = in_func;

                if (local) {
                        register_local_var_data(var_name_tok->str,
                                                var_type_tok->str, pc);
                }
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

        unsigned body_size = 0;
        struct Node **body = gen_body(pc, &body_size);

        result->cond = cond;
        result->body = body;
        result->body_size = body_size;
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

        unsigned body_size = 0;
        struct Node **body = gen_body(pc, &body_size);

        result->init = init;
        result->cond = cond;
        result->step = step;
        result->body = body;
        result->body_size = body_size;

        return pack(AST_ForStatement, result);
}

static struct Node *gen_ret_node(struct Token *first,
                                 struct ParserContext *pc) {
        struct Node *expr = parse_expression(pc);
        struct ReturnAST *ret_ast =
            (struct ReturnAST *)S_malloc(sizeof(struct ReturnAST));

        ret_ast->expr = expr;

        return pack(AST_Return, ret_ast);
}

static struct ClassData *register_class_data(const char *class_name,
                                             const char *parent_name,
                                             struct ParserContext *pc) {

        struct ClassData *data =
            (struct ClassData *)S_malloc(sizeof(struct ClassData));
        data->id = pc->class_type_smtb->size + 1;

        data->class_name = class_name;

        data->parent_name = parent_name;

        data->member_funcs = gen_htable();
        data->member_vars = gen_htable();

        ht_insert(pc->class_type_smtb, class_name,
                  gen_class_type(class_name, data));

        pc->class_data[pc->class_data_count++] = data;

        return data;
}

static struct Node *gen_class_decl_node(struct Token *first,
                                        struct ParserContext *pc) {
        struct TokenizerContext *tc = pc->tc;
        struct ClassAST *class_ast =
            (struct ClassAST *)S_malloc(sizeof(struct ClassAST));

        struct Token *name_tok = pull(tc);
        class_ast->name_tok = name_tok;
        class_ast->parent_name_tok = NULL;

        if (peek(tc)->type == TokExtends) {
                consume(tc, TokExtends);

                struct Token *parent_name_tok = pull(tc);
                class_ast->parent_name_tok = parent_name_tok;
        }

        // first register class data.
        struct Node *result = pack(AST_Class, class_ast);

        // and parse body
        unsigned body_size;

        pc->current_class = find_class_data(pc, name_tok->str);
        class_ast->body = gen_body(pc, &body_size);
        class_ast->body_size = body_size;
        pc->current_class = NULL;

        return result;
}

static void parse_class_structure(struct ParserContext *pc) {
        struct TokenizerContext *tc = pc->tc;

        struct Token *class_name_tok = pull(tc);

        const char *class_name = class_name_tok->str;
        const char *parent_name = "";

        if (peek(tc)->type == TokExtends) {
                consume(tc, TokExtends);
                struct Token *parent_name_tok = pull(tc);
                parent_name = parent_name_tok->str;
        }

        struct ClassData *cd = register_class_data(class_name, parent_name, pc);
        pc->current_class = cd;

        consume(tc, TokLBracket);

        struct Token *tok = NULL;

        while ((tok = peek(tc))->type != TokRBracket) {
                parse_structure(pc);
        }

        consume(tc, TokRBracket);

        pc->current_class = NULL;
}

static void parse_func_structure(struct ParserContext *pc) {
        struct TokenizerContext *tc = pc->tc;

        struct Token *func_name_tok = pull(tc);
        int i;
        struct Token *tok;

        struct Node *params = gen_func_param_node(pc);
        struct VarDeclBundleAST *params_ast =
            (struct VarDeclBundleAST *)params->ast;

        consume(tc, TokColon);

        struct Token *ret_type_tok = pull(tc);

        struct FuncData *func_data =
            register_func_data(func_name_tok->str, ret_type_tok->str, pc);

        func_data->arg_types = S_malloc(params_ast->var_count * sizeof(char *));
        func_data->arg_count = params_ast->var_count;

        for (i = 0; i < params_ast->var_count; i++) {
                struct VarDeclAST *param =
                    (struct VarDeclAST *)params_ast->var_decls[i];

                func_data->arg_types[i] = param->var_type_tok->str;
        }

        // We will not parse content of function declaration.
        consume(tc, TokLBracket);

        while ((tok = pull(tc))->type != TokRBracket) {
        }
}

static void parse_var_structure(struct ParserContext *pc) {
        struct TokenizerContext *tc = pc->tc;
        bool comp = false;

        while (!comp) {
                struct Token *var_name_tok = pull(tc);

                consume(tc, TokColon);

                struct Token *var_type_tok = pull(tc);

                //		var a: int = 0;

                struct Token *tok = NULL;

                while ((tok = pull(tc)) != NULL) {
                        if (tok->type == TokSemiColon) {
                                comp = true;
                                break;
                        }

                        if (tok->type == TokComma) {
                                break;
                        }
                }

                // Since we do not parse function declaration, all variables
                // must be in class or global scope.
                register_var_data(var_name_tok->str, var_type_tok->str, pc);
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

        default: {
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
                               fd->func_name, fd->id, fd->return_type);

                        printf("Arguments : ");
                        for (j = 0; j < fd->arg_count; j++) {
                                printf("%s,", fd->arg_types[j]);
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
                               vd->var_name, vd->id, vd->type);
                        dn = dn->next;
                }
        }
}

void debug_view_data(struct ParserContext *pc) {
        debug_view_func_smtb(pc->glob_func_smtb);

        int i;
        for (i = 0; i < pc->class_data_count; i++) {
                struct ClassData *cd = pc->class_data[i];

                printf("Class Data : %s(%d), parent : %s\n", cd->class_name,
                       cd->id, cd->parent_name);

                printf("------ members -----\n");

                debug_view_func_smtb(cd->member_funcs);
                debug_view_var_smtb(cd->member_vars);

                printf("-------------------\n\n");
        }
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

                return pack(AST_NumberLiteral, num);
        }

        case TokStringLiteral: {
                struct StringLiteralAST *str =
                    (struct StringLiteralAST *)S_malloc(
                        sizeof(struct StringLiteralAST));

                str->str_tok = first;

                return pack(AST_StringLiteral, str);
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

        case TokVar: {
                return gen_var_decl_node(first, pc, is_expr);
        }

        case TokClass: {
                return gen_class_decl_node(first, pc);
        }

        case TokIdent: {
                return gen_ident_node(first, pc, NULL, is_expr);
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
                bin_expr->opType = op_type;

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
                bin_expr->opType = op_type;

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

                bin_expr->opType = op_type;

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
                bin_expr->opType = op_type;

                node = pack(AST_BinExpr, bin_expr);
        }

        return node;
}
