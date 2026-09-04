#define DEBUG

#include <ir.h>
#include <parser.h>
#include <semantics.h>
#include <token.h>
#include <type.h>
#include <util.h>

#include <code_data.h>
#include <ir_read.h>
#include <vm.h>

#include <assert.h>
#include <locale.h>

static void resolve_first_pass_queue(struct ParserContext *pc) {
    while (pc->first_pass_queue->size != 0) {
        struct TokenizerContext *tc = q_pop(pc->first_pass_queue);

        pc->tc = tc;
        while (peek(tc)->type != TokEOF) {
            parse_structure(pc);
        }

        init_tc(tc);
        q_push(pc->second_pass_queue, tc);
    }
}

static void resolve_second_pass_queue(struct ParserContext *pc) {
    while (pc->second_pass_queue->size != 0) {
        struct TokenizerContext *tc = q_pop(pc->second_pass_queue);

        compile_file(pc, tc);
    }
}

static void resolve_sementic_analysis(struct ParserContext *pc) {

    int i;
    for (i = 0; i < pc->node_count; i++) {
        struct Node *node = pc->nodes[i];

        if (node->type == AST_Class) {
            register_data(pc, node);
        }
    }

    for (i = 0; i < pc->node_count; i++) {
        struct Node *node = pc->nodes[i];

        if (node->type != AST_Class) {
            register_data(pc, node);
        }
    }

    for (i = 0; i < pc->node_count; i++) {
        struct Node *node = pc->nodes[i];

        check_semantics(pc, node);
    }
}

static void free_frontend(struct ParserContext *pc, struct TokenizerContext **contexts,
                          int context_count) {
    int i;

    free_pc(pc);
    for (i = 0; i < context_count; i++)
        free_tc(contexts[i]);
    free(contexts);
}

static bool add_source(struct ParserContext *pc, struct TokenizerContext **contexts,
                       int *context_count, const char *path) {
    char *source = read_file(path);

    if (source == NULL)
        return false;
    contexts[*context_count] = gen_tc(source);
    q_push(pc->first_pass_queue, contexts[*context_count]);
    (*context_count)++;
    return true;
}

int main(int arc, char *args[]) {
    int i;
    int source_count = arc > 1 ? arc - 1 : 2;
    int loaded_count = 0;

    // front end
    struct ParserContext *pc = gen_pc();
    struct TokenizerContext **contexts =
        S_malloc(sizeof(struct TokenizerContext *) * (size_t)source_count);

    if (arc > 1) {
        for (i = 1; i < arc; i++) {
            if (!add_source(pc, contexts, &loaded_count, args[i])) {
                free_frontend(pc, contexts, loaded_count);
                return 1;
            }
        }
    } else {
        if (!add_source(pc, contexts, &loaded_count, "test.cn") ||
            !add_source(pc, contexts, &loaded_count, "test2.cn")) {
            free_frontend(pc, contexts, loaded_count);
            return 1;
        }
        // q_push(pc->first_pass_queue, gen_tc(read_file("fibo.cn")));
        // q_push(pc->first_pass_queue, gen_tc(read_file("test_basic.cn")));
    }

    resolve_first_pass_queue(pc);
    resolve_second_pass_queue(pc);
    resolve_sementic_analysis(pc);

    debug_view_data(pc);

    // back end
    struct IRContext *irc = gen_irc();

    init_irc(irc, NULL);

    gen_ir(irc, pc);

    if (!write_file("test.cb", irc->byte_cnt, (const char *)get_bytes(irc))) {
        free_irc(irc);
        free_frontend(pc, contexts, loaded_count);
        return 1;
    }

    // print_bytes(irc);

    struct VM *vm = gen_vm();
    struct IRReader *ir_reader = gen_ir_reader(irc);

    if (!read_ir(vm, ir_reader)) {
        free_ir_reader(ir_reader);
        free_vm(vm);
        free_irc(irc);
        free_frontend(pc, contexts, loaded_count);
        return 1;
    }

    struct VMFunctionData *func_data = vm_find_function_data(vm, NULL, vm->main_func_id);

    vm_exec_function(vm, func_data, -1);

    free_ir_reader(ir_reader);
    free_vm(vm);
    free_irc(irc);
    free_frontend(pc, contexts, loaded_count);
    return 0;
}
