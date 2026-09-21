#include <compile_sources.h>
#include <token.h>
#include <source_manager.h>

#include <ir.h>
#include <semantics.h>
#include <string.h>

struct SourceFile {
    const char *path;
    struct TokenizerContext *tc;
    unsigned node_begin;
    unsigned node_count;
};

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

static char *bytecode_path(const char *source_path) {
    const char *name = source_path;
    const char *cursor;
    const char *extension;
    size_t stem_length;
    char *path;

    for (cursor = source_path; *cursor != '\0'; cursor++)
        if (*cursor == '/' || *cursor == '\\')
            name = cursor + 1;
    extension = strrchr(name, '.');
    stem_length = extension == NULL ? strlen(source_path) : (size_t)(extension - source_path);
    path = S_malloc(stem_length + sizeof(".cb"));
    memcpy(path, source_path, stem_length);
    memcpy(path + stem_length, ".cb", sizeof(".cb"));
    return path;
}

static void analyze_sources(struct ParserContext *pc, struct SourceFile *sources, unsigned count) {
    unsigned phase, i, j;

    for (phase = 0; phase < 3; phase++) {
        for (i = 0; i < count; i++) {
            struct SourceFile *source = &sources[i];
            pc->tc = source->tc;
            for (j = source->node_begin; j < source->node_begin + source->node_count; j++) {
                struct Node *node = pc->nodes[j];
                if (phase == 2)
                    check_semantics(pc, node);
                else if ((node->type == AST_Class) == (phase == 0))
                    register_data(pc, node);
            }
        }
    }
}

static void prepare_output_context(struct ParserContext *output, const struct ParserContext *pc,
                                   const struct SourceFile *source) {
    unsigned i, j;
    unsigned local_class_count;

    *output = *pc;
    output->tc = source->tc;
    output->nodes = pc->nodes + source->node_begin;
    output->node_count = source->node_count;
    output->class_data_count = 0;
    output->func_data_count = 0;
    for (i = 0; i < output->node_count; i++) {
        struct Node *node = output->nodes[i];
        if (node->type == AST_Class)
            output->class_data[output->class_data_count++] = ((struct ClassAST *)node->ast)->class_data;
        else if (node->type == AST_FunctionDeclaration)
            output->func_data[output->func_data_count++] = ((struct FuncDeclAST *)node->ast)->func_data;
    }
    local_class_count = output->class_data_count;
    for (i = 0; i < pc->class_data_count; i++) {
        for (j = 0; j < local_class_count; j++)
            if (output->class_data[j] == pc->class_data[i])
                break;
        if (j == local_class_count)
            output->class_data[output->class_data_count++] = pc->class_data[i];
    }
}

static bool write_source(struct ParserContext *pc, const struct SourceFile *source) {
    struct ParserContext output;
    struct IRContext *irc = gen_irc();
    unsigned original_ids[MAX_CLASS_COUNT];
    unsigned i;
    char *path = bytecode_path(source->path);
    bool written;

    prepare_output_context(&output, pc, source);
    /* The output view borrows shared data; restore IDs immediately after emission. */
    for (i = 0; i < output.class_data_count; i++) {
        original_ids[i] = output.class_data[i]->id;
        output.class_data[i]->id = i + 1;
    }
    init_irc(irc, NULL);
    gen_ir(irc, &output);
    for (i = 0; i < output.class_data_count; i++)
        output.class_data[i]->id = original_ids[i];
    written = write_file(path, irc->byte_cnt, (const char *)get_bytes(irc));
    free(path);
    free_irc(irc);
    return written;
}

static void check_preprocessor(struct HTable *source_table, struct Sources *sources, const char *path){    
    struct TokenizerContext *tc = gen_tc(read_file(path));

    struct Token *tok = NULL;
    
    while((tok = peek(tc)) != NULL && tok->type != TokEOF){
        tok = pull(tc);
        
        if(tok->type == TokSharp){
            struct Token *pp_tok = pull(tc);

            switch(pp_tok->type){
            case TokImport:{
                struct Token *path_tok = consume(tc, TokStringLiteral);

                const char *import_path = path_tok->str;
                
                if(ht_find(source_table, import_path) != NULL){
                    break;
                }

                ht_insert(source_table, import_path, (char *) import_path);

                add_source(sources, import_path);
                
                break;
            }

            default:
            printf("Unknown preprocessor : %s", pp_tok->str);
            break;
            }
        }
    }

    free(tc);
}

void handle_preprocessor(struct HTable *source_table, struct Sources *sources){
    int i;
    unsigned count = sources->count;
    
    for(i=0; i<count; i++){
        check_preprocessor(source_table, sources, sources->paths[i]);
    }
}

bool compile_sources(struct Sources *sources) {
    struct ParserContext *pc = gen_pc();
    unsigned count = sources->count;
    struct SourceFile *source_files = S_malloc(sizeof(*source_files) * count);
    unsigned loaded = 0;
    unsigned i;
    bool success = false;
    
    for (i = 0; i < count; i++) {
        char *text = read_file(sources->paths[i]);

        if (text == NULL)
            goto done;
        
        source_files[i].path = sources->paths[i];
        source_files[i].tc = gen_tc(text);
        
        q_push(pc->first_pass_queue, source_files[i].tc);
        loaded++;
    }
    
    resolve_first_pass_queue(pc);
    
    for (i = 0; i < count; i++) {
        source_files[i].node_begin = pc->node_count;
        compile_file(pc, q_pop(pc->second_pass_queue));
        source_files[i].node_count = pc->node_count - source_files[i].node_begin;
    }
    
    analyze_sources(pc, source_files, count);
    
    for (i = 0; i < count; i++)
        if (!write_source(pc, &source_files[i]))
            goto done;
    
    success = true;

 done:
    free_pc(pc);
    for (i = 0; i < loaded; i++)
        free_tc(source_files[i].tc);
    free(sources);
    
    return success;
}
