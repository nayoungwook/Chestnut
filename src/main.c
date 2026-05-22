#define DEBUG

#include <ir.h>
#include <parser.h>
#include <token.h>
#include <type.h>
#include <util.h>
#include <semantics.h>

#include <ir_read.h>

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

static void resolve_sementic_analysis(struct ParserContext *pc){

	int i;
	for(i=0; i<pc->node_count; i++){
		struct Node *node = pc->nodes[i];

		register_data(pc, node);
	}

	for(i=0; i<pc->node_count; i++){
		struct Node *node = pc->nodes[i];

		check_semantics(pc, node);
	}
}

int main(int arc, char *args[]) {

        // front end
        struct ParserContext *pc = gen_pc();

        q_push(pc->first_pass_queue, gen_tc(read_file("test.cn")));
        q_push(pc->first_pass_queue, gen_tc(read_file("test2.cn")));
	
        resolve_first_pass_queue(pc);
	resolve_second_pass_queue(pc);
	resolve_sementic_analysis(pc);
	
	debug_view_data(pc);
	
        // back end
        struct IRContext *irc = gen_irc();

        init_irc(irc, NULL);

	gen_ir(irc, pc);

	write_file("test.cb", irc->byte_size, (const char *)get_bytes(irc));

	// print_bytes(irc);

	read_ir(gen_ir_reader(irc));

        return 0;
}
