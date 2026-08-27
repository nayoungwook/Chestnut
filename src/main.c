#define DEBUG

#include <ir.h>
#include <parser.h>
#include <semantics.h>
#include <token.h>
#include <type.h>
#include <util.h>

#include <vm.h>
#include <ir_read.h>
#include <code_data.h>

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

                free_tc(tc);
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

int main(int arc, char *args[]) {

        // front end
        struct ParserContext *pc = gen_pc();

        if (arc > 1) {
                int i;
                for (i = 1; i < arc; i++)
                        q_push(pc->first_pass_queue,
                               gen_tc(read_file(args[i])));
        } else {
                // q_push(pc->first_pass_queue, gen_tc(read_file("test.cn")));
                // q_push(pc->first_pass_queue, gen_tc(read_file("test2.cn")));
		// q_push(pc->first_pass_queue, gen_tc(read_file("fibo.cn")));
		q_push(pc->first_pass_queue, gen_tc(read_file("test_basic.cn")));
        }

        resolve_first_pass_queue(pc);
        resolve_second_pass_queue(pc);
        resolve_sementic_analysis(pc);

        // debug_view_data(pc);

        // back end
        struct IRContext *irc = gen_irc();

        init_irc(irc, NULL);

        gen_ir(irc, pc);

        write_file("test.cb", irc->byte_size, (const char *)get_bytes(irc));

        // print_bytes(irc);

	struct VM *vm = gen_vm();
	struct IRReader *ir_reader = gen_ir_reader(irc);

        read_ir(vm, ir_reader);

        struct VMFunctionData *func_data =
            vm_find_function_data(vm, NULL, vm->main_func_id);

	vm_exec_function(vm, func_data);
        
        return 0;
}
