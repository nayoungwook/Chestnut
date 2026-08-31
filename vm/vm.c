#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef __unix__
#include <sys/mman.h>
#endif

#include <code_data.h>
#include <ir.h>
#include <util.h>
#include <vm.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define DEBUG

#ifdef DEBUG

#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#define DEBUG_FPRINTF(...) fprintf(__VA_ARGS__)

#else

#define DEBUG_PRINTF(...) ((void)0)
#define DEBUG_FPRINTF(...) ((void)0)

#endif

#define DUMP_STACK

#ifdef DUMP_STACK
static void dump_stack(struct VM *vm) {
    void *ptr = vm->stack;
    while ((void *)ptr != NULL) {
        printf("%p\n", ptr);
        ptr += 8;
    }
}
#endif

static const char *copy_string(const char *value) {
    size_t size = strlen(value) + 1;
    char *copy = (char *)S_malloc(size);
    memcpy(copy, value, size);
    return copy;
}

struct VM *gen_vm() {
    struct VM *vm = (struct VM *)S_malloc(sizeof(struct VM));

    vm->class_data_count = 0;
    vm->class_data_capacity = 1;
    vm->class_data =
        (struct VMClassData **)S_malloc(sizeof(struct VMClassData *));

    vm->function_data_count = 0;
    vm->function_data_capacity = 1;
    vm->function_data =
        (struct VMFunctionData **)S_malloc(sizeof(struct VMFunctionData *));

    const unsigned HEAP_SIZE = 1024 * 1024 * 16;
    const unsigned STACK_SIZE = 1024 * 1024;

#ifdef __unix__
    vm->heap = mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    vm->stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif

#ifdef _WIN32
    HANDLE h_map = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL,
                                      PAGE_READWRITE, 0, HEAP_SIZE, NULL);
    HANDLE s_map = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL,
                                      PAGE_READWRITE, 0, STACK_SIZE, NULL);

    vm->heap =
        (uint8_t *)MapViewOfFile(h_map, FILE_MAP_ALL_ACCESS, 0, 0, HEAP_SIZE);
    vm->stack =
        (uint8_t *)MapViewOfFile(s_map, FILE_MAP_ALL_ACCESS, 0, 0, STACK_SIZE);
#endif

    vm->stack_pointer = vm->stack;
    vm->stack_pointer_type = S_malloc(STACK_SIZE);

    vm->vm_stack = (struct VMStack *)S_malloc(sizeof(struct VMStack));
    memset(vm->vm_stack, 0, sizeof(struct VMStack));

    vm->vm_string_pool =
        (struct VMStringPool *)S_malloc(sizeof(struct VMStringPool));
    vm->vm_string_pool->str_pool = NULL;

    return vm;
}

void reset_string_pool(struct VM *vm, unsigned size) {
    vm->vm_string_pool->size = size;
    vm->vm_string_pool->str_pool = (char **)S_malloc(sizeof(char *) * size);
}

void register_string_pool(struct VM *vm, char *str, int index) {
    assert(index < vm->vm_string_pool->size);

    vm->vm_string_pool->str_pool[index] = str;
}

const char *get_string_pool(struct VM *vm, int index) {
    assert(index < vm->vm_string_pool->size);

    return vm->vm_string_pool->str_pool[index];
}

struct VMClassData *vm_add_class_data(struct VM *vm, unsigned id,
                                      const char *name, unsigned parent_id,
                                      unsigned size) {
    struct VMClassData *class_data;

    if (vm->class_data_count == vm->class_data_capacity) {
        vm->class_data_capacity *= 2;
        vm->class_data = (struct VMClassData **)S_realloc(
            vm->class_data,
            sizeof(struct VMClassData *) * vm->class_data_capacity);
    }

    class_data = (struct VMClassData *)S_malloc(sizeof(struct VMClassData));
    class_data->id = id;
    class_data->name = copy_string(name);
    class_data->parent_id = parent_id;
    class_data->size = size;
    class_data->function_data_count = 0;
    class_data->function_data_capacity = 1;
    class_data->function_data =
        (FunctionData **)S_malloc(sizeof(FunctionData *));

    vm->class_data[vm->class_data_count++] = class_data;
    return class_data;
}

struct VMFunctionData *
vm_add_function_data(struct VM *vm, struct VMClassData *owner, unsigned id,
                     const char *name, const char *return_type,
                     const char **argument_types, unsigned argument_count,
                     unsigned stack_size, bool is_constructor) {
    struct VMFunctionData *function_data;
    unsigned i;

    function_data =
        (struct VMFunctionData *)S_malloc(sizeof(struct VMFunctionData));
    function_data->id = id;
    function_data->stack_size = stack_size;
    function_data->name = copy_string(name);
    function_data->return_type = copy_string(return_type);
    function_data->argument_count = argument_count;
    function_data->is_constructor = is_constructor;
    function_data->instructions = NULL;
    function_data->instruction_count = 0;

    if (strcmp(name, "main") == 0) {
        vm->main_func_id = id;
    }

    function_data->argument_types =
        argument_count == 0
            ? NULL
            : (const char **)S_malloc(sizeof(char *) * argument_count);

    for (i = 0; i < argument_count; i++)
        function_data->argument_types[i] = copy_string(argument_types[i]);

    if (owner == NULL) {
        if (vm->function_data_count == vm->function_data_capacity) {
            vm->function_data_capacity *= 2;
            vm->function_data = (struct VMFunctionData **)S_realloc(
                vm->function_data,
                sizeof(struct VMFunctionData *) * vm->function_data_capacity);
        }

        vm->function_data[vm->function_data_count++] = function_data;
    } else {
        if (owner->function_data_count == owner->function_data_capacity) {
            owner->function_data_capacity *= 2;
            owner->function_data = (FunctionData **)S_realloc(
                owner->function_data,
                sizeof(FunctionData *) * owner->function_data_capacity);
        }

        owner->function_data[owner->function_data_count++] = function_data;
    }

    return function_data;
}

struct VMClassData *vm_find_class_data(const struct VM *vm, unsigned id) {
    unsigned i;

    for (i = 0; i < vm->class_data_count; i++)
        if (vm->class_data[i]->id == id)
            return vm->class_data[i];
    return NULL;
}

struct VMFunctionData *vm_find_function_data(const struct VM *vm,
                                             struct VMClassData *owner,
                                             unsigned id) {
    struct VMFunctionData **items =
        owner == NULL ? vm->function_data : owner->function_data;

    unsigned count =
        owner == NULL ? vm->function_data_count : owner->function_data_count;
    unsigned i;

    for (i = 0; i < count; i++)
        if (items[i]->id == id)
            return items[i];
    return NULL;
}

void vm_set_function_instructions(struct VMFunctionData *function_data,
                                  const struct VMInstruction *instructions,
                                  unsigned instruction_count) {
    function_data->instruction_count = instruction_count;
    function_data->instructions =
        instruction_count == 0
            ? NULL
            : (struct VMInstruction *)S_malloc(sizeof(struct VMInstruction) *
                                               instruction_count);

    if (instruction_count != 0)
        memcpy(function_data->instructions, instructions,
               sizeof(struct VMInstruction) * instruction_count);
}

#if defined(DEBUG) || defined(DEBUG_STAMP_COMMAND)
static const char *get_instruction_name(unsigned char opcode) {
    switch (opcode) {
    case OP_PUSH_NULL:
        return "push_null";
    case OP_EXPR_OP:
        return "expr_op";
    case OP_SP_PUSH:
        return "sp_push";
    case OP_SP_POP:
        return "sp_pop";
    case OP_SP_LOAD:
        return "sp_load";
    case OP_SP_SAVE:
        return "sp_save";
    case OP_SP_INCRE:
        return "sp_incre";
    case OP_SP_DECRE:
        return "sp_decre";
    case OP_LOAD_CLASS:
        return "load_class";
    case OP_INCRE_CLASS:
        return "incre_class";
    case OP_DECRE_CLASS:
        return "decre_class";
    case OP_SAVE_CLASS:
        return "save_class";
    case OP_LOAD_GLOBAL:
        return "load_global";
    case OP_INCRE_GLOBAL:
        return "incre_global";
    case OP_DECRE_GLOBAL:
        return "decre_global";
    case OP_SAVE_GLOBAL:
        return "save_global";
    case OP_LOAD_ATTR:
        return "load_attr";
    case OP_INCRE_ATTR:
        return "incre_attr";
    case OP_DECRE_ATTR:
        return "decre_attr";
    case OP_SAVE_ATTR:
        return "save_attr";
    case OP_SYSCALL:
        return "syscall";
    case OP_CALL:
        return "call";
    case OP_CALL_ATTR:
        return "call_attr";
    case OP_CALL_CLASS:
        return "call_class";
    case OP_CALL_GLOBAL:
        return "call_global";
    case OP_LOAD_STR:
        return "load_str";
    case OP_GOTO:
        return "goto";
    case OP_JE:
        return "je";
    case OP_JNE:
        return "jne";
    case OP_RET:
        return "ret";
    case OP_NEG:
        return "neg";
    case OP_LDC_I4:
        return "ldc_i4";
    case OP_LDC_F4:
        return "ldc_f4";
    case OP_LDC_F8:
        return "ldc_f8";
    case OP_NEW_OBJECT:
        return "new_object";
    case OP_NEW_ARRAY:
        return "new_array";
    case OP_ARRAY_LOAD:
        return "array_load";
    case OP_ARRAY_SAVE:
        return "array_save";
    case OP_ARRAY_LENGTH:
        return "array_length";
    case OP_ARRAY_PUSH:
        return "array_push";
    case OP_ARRAY_REMOVE:
        return "array_remove";
    default:
        return NULL;
    }
}
#endif

#ifdef DEBUG
static const char *get_expr_op_name(unsigned char opcode) {
    switch (opcode) {
    case OP_ADD:
        return "add";
    case OP_SUB:
        return "sub";
    case OP_MUL:
        return "mul";
    case OP_DIV:
        return "div";
    case OP_EQUAL:
        return "equal";
    case OP_NOTEQUAL:
        return "notequal";
    case OP_GREATER:
        return "greater";
    case OP_LESS:
        return "less";
    case OP_EQUALGREATER:
        return "eqgreater";
    case OP_EQUALLESS:
        return "eqless";
    case OP_ASSIGN:
        return "assign";
    case OP_OR:
        return "or";
    case OP_AND:
        return "and";
    default:
        return NULL;
    }
}
#endif

#ifdef DEBUG
static void debug_print_instruction(const struct VMInstruction *instruction) {
    const char *name = get_instruction_name(instruction->opcode);
    unsigned i;

    if (name == NULL) {
        DEBUG_PRINTF("unknown 0x%02x\n", instruction->opcode);
        return;
    }

    if (instruction->opcode == OP_EXPR_OP) {
        const char *expr_name = get_expr_op_name(instruction->operands.u8);

        DEBUG_PRINTF("%s\n", expr_name != NULL ? expr_name : "unknown");
        return;
    }

    DEBUG_PRINTF("%s", name);
    if (instruction->opcode == OP_LDC_F4) {
        DEBUG_PRINTF(" %.9g", instruction->operands.f32);
    } else if (instruction->opcode == OP_LDC_F8) {
        DEBUG_PRINTF(" %.17g", instruction->operands.f64);
    } else {
        for (i = 0; i < instruction->operand_count; i++)
            DEBUG_PRINTF(" %d", instruction->operands.i32[i]);
    }
    DEBUG_PRINTF("\n");
}

static void
debug_print_function_bytecode(const struct VMClassData *owner,
                              const struct VMFunctionData *function_data) {
    unsigned i;

    if (owner == NULL) {
        DEBUG_PRINTF("[function %s (id: %u)]\n", function_data->name,
                     function_data->id);
    } else {
        DEBUG_PRINTF("[class %s (id: %u), function %s (id: %u)]\n", owner->name,
                     owner->id, function_data->name, function_data->id);
    }

    for (i = 0; i < function_data->instruction_count; i++) {
        DEBUG_PRINTF("%04u: ", i);
        debug_print_instruction(&function_data->instructions[i]);
    }
}
#endif

void vm_debug_print_bytecode(const struct VM *vm) {
#ifdef DEBUG
    unsigned i;
    unsigned j;

    DEBUG_PRINTF("=== VM bytecode ===\n");
    for (i = 0; i < vm->function_data_count; i++)
        debug_print_function_bytecode(NULL, vm->function_data[i]);

    for (i = 0; i < vm->class_data_count; i++) {
        const struct VMClassData *class_data = vm->class_data[i];

        for (j = 0; j < class_data->function_data_count; j++)
            debug_print_function_bytecode(class_data,
                                          class_data->function_data[j]);
    }
    DEBUG_PRINTF("=== end VM bytecode ===\n");
#endif
}

static void handle_syscall(struct VM *vm, int id, int argc) {
    switch (id) {
    case 0: {
        int i;

        for (i = 0; i < argc; i++) {
            struct VMOperand op = vm_stack_pop(vm->vm_stack);

            switch (op.op_type) {
            case OPRND_String: {
                const char *str = get_string_pool(vm, (int)op.val);

                fputs(str, stdout);

                break;
            }
            case OPRND_INT32: {
                const int val = (int)op.val;

                printf("%d", val);
                break;
            }
            case OPRND_FLOAT32: {
                float val;
                memcpy(&val, &op.val, sizeof(float));
                printf("%g", val);
                break;
            }
            case OPRND_FLOAT64: {
                double val;
                memcpy(&val, &op.val, sizeof(double));
                printf("%g", val);
                break;
            }

            default: {
                printf("print format not supported. : %d\n", op.op_type);
                break;
            }
            }
        }
        break;
    }
    default: {
        assert(false && "Syscall not implemented.");
    }
    }
}

union VMNumericValue {
    int32_t i32;
    float f32;
    double f64;
};

typedef bool (*VMOperatorFunc)(const union VMNumericValue *lhs_value,
                               const union VMNumericValue *rhs_value,
                               union VMNumericValue *result_value);

enum VMOperatorIndex {
    VM_OPERATOR_ADD,
    VM_OPERATOR_SUB,
    VM_OPERATOR_MUL,
    VM_OPERATOR_DIV,
    VM_OPERATOR_EQUAL,
    VM_OPERATOR_NOTEQUAL,
    VM_OPERATOR_GREATER,
    VM_OPERATOR_LESS,
    VM_OPERATOR_EQUALGREATER,
    VM_OPERATOR_EQUALLESS,
    VM_OPERATOR_OR,
    VM_OPERATOR_AND,
    VM_OPERATOR_COUNT,
};

#define DEFINE_VM_OPERATOR_SET(prefix, type, member)                           \
    static bool prefix##_add(const union VMNumericValue *lhs_value,            \
                             const union VMNumericValue *rhs_value,            \
                             union VMNumericValue *result_value) {             \
        result_value->member = lhs_value->member + rhs_value->member;          \
        return true;                                                           \
    }                                                                          \
    static bool prefix##_sub(const union VMNumericValue *lhs_value,            \
                             const union VMNumericValue *rhs_value,            \
                             union VMNumericValue *result_value) {             \
        result_value->member = lhs_value->member - rhs_value->member;          \
        return true;                                                           \
    }                                                                          \
    static bool prefix##_mul(const union VMNumericValue *lhs_value,            \
                             const union VMNumericValue *rhs_value,            \
                             union VMNumericValue *result_value) {             \
        result_value->member = lhs_value->member * rhs_value->member;          \
        return true;                                                           \
    }                                                                          \
    static bool prefix##_div(const union VMNumericValue *lhs_value,            \
                             const union VMNumericValue *rhs_value,            \
                             union VMNumericValue *result_value) {             \
        if (rhs_value->member == (type)0)                                      \
            return false;                                                      \
        result_value->member = lhs_value->member / rhs_value->member;          \
        return true;                                                           \
    }                                                                          \
    static bool prefix##_equal(const union VMNumericValue *lhs_value,          \
                               const union VMNumericValue *rhs_value,          \
                               union VMNumericValue *result_value) {           \
        result_value->i32 = lhs_value->member == rhs_value->member;            \
        return true;                                                           \
    }                                                                          \
    static bool prefix##_notequal(const union VMNumericValue *lhs_value,       \
                                  const union VMNumericValue *rhs_value,       \
                                  union VMNumericValue *result_value) {        \
        result_value->i32 = lhs_value->member != rhs_value->member;            \
        return true;                                                           \
    }                                                                          \
    static bool prefix##_greater(const union VMNumericValue *lhs_value,        \
                                 const union VMNumericValue *rhs_value,        \
                                 union VMNumericValue *result_value) {         \
        result_value->i32 = lhs_value->member > rhs_value->member;             \
        return true;                                                           \
    }                                                                          \
    static bool prefix##_less(const union VMNumericValue *lhs_value,           \
                              const union VMNumericValue *rhs_value,           \
                              union VMNumericValue *result_value) {            \
        result_value->i32 = lhs_value->member < rhs_value->member;             \
        return true;                                                           \
    }                                                                          \
    static bool prefix##_equalgreater(const union VMNumericValue *lhs_value,   \
                                      const union VMNumericValue *rhs_value,   \
                                      union VMNumericValue *result_value) {    \
        result_value->i32 = lhs_value->member >= rhs_value->member;            \
        return true;                                                           \
    }                                                                          \
    static bool prefix##_equalless(const union VMNumericValue *lhs_value,      \
                                   const union VMNumericValue *rhs_value,      \
                                   union VMNumericValue *result_value) {       \
        result_value->i32 = lhs_value->member <= rhs_value->member;            \
        return true;                                                           \
    }                                                                          \
    static bool prefix##_or(const union VMNumericValue *lhs_value,             \
                            const union VMNumericValue *rhs_value,             \
                            union VMNumericValue *result_value) {              \
        result_value->i32 =                                                    \
            lhs_value->member != (type)0 || rhs_value->member != (type)0;      \
        return true;                                                           \
    }                                                                          \
    static bool prefix##_and(const union VMNumericValue *lhs_value,            \
                             const union VMNumericValue *rhs_value,            \
                             union VMNumericValue *result_value) {             \
        result_value->i32 =                                                    \
            lhs_value->member != (type)0 && rhs_value->member != (type)0;      \
        return true;                                                           \
    }

DEFINE_VM_OPERATOR_SET(i32, int32_t, i32)
DEFINE_VM_OPERATOR_SET(f32, float, f32)
DEFINE_VM_OPERATOR_SET(f64, double, f64)

#undef DEFINE_VM_OPERATOR_SET

static const VMOperatorFunc vm_operator_table[3][VM_OPERATOR_COUNT] = {
    {
        i32_add,
        i32_sub,
        i32_mul,
        i32_div,
        i32_equal,
        i32_notequal,
        i32_greater,
        i32_less,
        i32_equalgreater,
        i32_equalless,
        i32_or,
        i32_and,
    },
    {
        f32_add,
        f32_sub,
        f32_mul,
        f32_div,
        f32_equal,
        f32_notequal,
        f32_greater,
        f32_less,
        f32_equalgreater,
        f32_equalless,
        f32_or,
        f32_and,
    },
    {
        f64_add,
        f64_sub,
        f64_mul,
        f64_div,
        f64_equal,
        f64_notequal,
        f64_greater,
        f64_less,
        f64_equalgreater,
        f64_equalless,
        f64_or,
        f64_and,
    },
};

static int get_vm_operator_index(byte expr_opcode) {
    switch (expr_opcode) {
    case OP_ADD:
        return VM_OPERATOR_ADD;
    case OP_SUB:
        return VM_OPERATOR_SUB;
    case OP_MUL:
        return VM_OPERATOR_MUL;
    case OP_DIV:
        return VM_OPERATOR_DIV;
    case OP_EQUAL:
        return VM_OPERATOR_EQUAL;
    case OP_NOTEQUAL:
        return VM_OPERATOR_NOTEQUAL;
    case OP_GREATER:
        return VM_OPERATOR_GREATER;
    case OP_LESS:
        return VM_OPERATOR_LESS;
    case OP_EQUALGREATER:
        return VM_OPERATOR_EQUALGREATER;
    case OP_EQUALLESS:
        return VM_OPERATOR_EQUALLESS;
    case OP_OR:
        return VM_OPERATOR_OR;
    case OP_AND:
        return VM_OPERATOR_AND;
    default:
        return -1;
    }
}

static bool is_boolean_operator(int operator_index) {
    return operator_index >= VM_OPERATOR_EQUAL;
}

static int get_operand_level(enum VMOPType op_type) {
    switch (op_type) {
    case OPRND_INT32:
    case OPRND_BOOL: {
        return 0;
    }
    case OPRND_FLOAT32: {
        return 1;
    }
    case OPRND_FLOAT64: {
        return 2;
    }
    default: {
        return -1;
    }
    }
}

static bool unpack_numeric_operand(const struct VMOperand *operand,
                                   int op_level, union VMNumericValue *value) {
    int operand_level = get_operand_level(operand->op_type);

    if (operand_level < 0 || operand_level > op_level)
        return false;

    switch (op_level) {
    case 0: {
        value->i32 = (int32_t)operand->val;
        return true;
    }
    case 1: {
        if (operand_level == 0) {
            value->f32 = (float)(int32_t)operand->val;
        } else {
            memcpy(&value->f32, &operand->val, sizeof(value->f32));
        }

        return true;
    }
    case 2: {
        if (operand_level == 0) {
            value->f64 = (double)(int32_t)operand->val;
        } else if (operand_level == 1) {
            float float_value;

            memcpy(&float_value, &operand->val, sizeof(float_value));
            value->f64 = (double)float_value;
        } else {
            memcpy(&value->f64, &operand->val, sizeof(value->f64));
        }

        return true;
    }
    default: {
        return false;
    }
    }
}

static void pack_numeric_result(struct VMOperand *result, int op_level,
                                const union VMNumericValue *result_value,
                                bool boolean_result) {
    result->val = 0;
    if (boolean_result) {
        result->op_type = OPRND_BOOL;
        result->val = result_value->i32 != 0;
        return;
    }

    switch (op_level) {
    case 0: {
        result->op_type = OPRND_INT32;
        result->val = result_value->i32;
        break;
    }
    case 1: {
        result->op_type = OPRND_FLOAT32;
        memcpy(&result->val, &result_value->f32, sizeof(result_value->f32));
        break;
    }
    case 2: {
        result->op_type = OPRND_FLOAT64;
        memcpy(&result->val, &result_value->f64, sizeof(result_value->f64));
        break;
    }
    }
}

size_t get_operand_size(enum VMOPType op_type) {
    switch (op_type) {
    case OPRND_NULL:
        return 0;

    case OPRND_BOOL:
        return 1;
    case OPRND_FLOAT32:
        return 4;
    case OPRND_FLOAT64:
        return 8;
    case OPRND_INT32:
        return 4;
    case OPRND_String:
        return 8;

    default:
        return 0;
    }
}

bool exec_instruction(struct VM *vm, const struct VMInstruction *instruction,
                      unsigned *instruction_index) {
    const int32_t *arguments = instruction->operands.i32;
    unsigned char opcode = instruction->opcode;
    bool ok = true;

    assert(vm->vm_stack != NULL);
    assert(instruction_index != NULL);
    switch (opcode) {
    case OP_PUSH_NULL: {
        struct VMOperand operand = {OPRND_NULL, 0};

        vm_stack_push(vm->vm_stack, operand);
        break;
    }
    case OP_EXPR_OP: {
        byte expr_opcode;
        int operator_index;
        int lhs_level;
        int rhs_level;
        int op_level;
        VMOperatorFunc operator_func;
        struct VMOperand lhs;
        struct VMOperand rhs;
        struct VMOperand result = {OPRND_NULL, 0};
        union VMNumericValue lhs_value = {0};
        union VMNumericValue rhs_value = {0};
        union VMNumericValue result_value = {0};

        expr_opcode = instruction->operands.u8;

#ifdef DEBUG
        {
            const char *name = get_expr_op_name(expr_opcode);

            if (name == NULL) {
                ok = false;
                break;
            }
        }
#endif

        operator_index = get_vm_operator_index(expr_opcode);
        if (operator_index < 0) {
            ok = false;
            break;
        }

        rhs = vm_stack_pop(vm->vm_stack);
        lhs = vm_stack_pop(vm->vm_stack);
        lhs_level = get_operand_level(lhs.op_type);
        rhs_level = get_operand_level(rhs.op_type);
        if (lhs_level < 0 || rhs_level < 0) {
            ok = false;
            break;
        }

        op_level = lhs_level > rhs_level ? lhs_level : rhs_level;
        ok = unpack_numeric_operand(&lhs, op_level, &lhs_value) &&
             unpack_numeric_operand(&rhs, op_level, &rhs_value);

        if (!ok)
            break;

        operator_func = vm_operator_table[op_level][operator_index];
        ok = operator_func(&lhs_value, &rhs_value, &result_value);

        if (!ok)
            break;

        pack_numeric_result(&result, op_level, &result_value,
                            is_boolean_operator(operator_index));
        vm_stack_push(vm->vm_stack, result);
        break;
    }
    case OP_SP_PUSH: {
        if (ok && arguments[0] >= 0)
            vm->stack_pointer = (uint8_t *)vm->stack_pointer + arguments[0];
        else
            ok = false;

        break;
    }
    case OP_SP_POP: {
        if (ok && arguments[0] >= 0)
            vm->stack_pointer = vm->stack_pointer - arguments[0];
        else
            ok = false;
        break;
    }
    case OP_SP_LOAD: {
        assert(arguments[0] >= 0 && arguments[1] >= 0 &&
               (size_t)arguments[1] <= sizeof(int64_t));

        if (!ok)
            break;

        int offset = arguments[0];
        size_t size = (size_t)arguments[1];

        int64_t value = 0;
        enum VMOPType type = (enum VMOPType)vm->stack_pointer_type[offset];
        struct VMOperand operand;

        memcpy(&value, vm->stack_pointer + offset, size);
        operand.op_type = type;
        operand.val = value;
        vm_stack_push(vm->vm_stack, operand);

        break;
    }
    case OP_SP_SAVE: {
        assert(arguments[0] >= 0 && arguments[1] >= 0 &&
               (size_t)arguments[1] <= sizeof(int64_t));

        if (!ok)
            break;

        struct VMOperand operand = vm_stack_pop(vm->vm_stack);
        int offset = arguments[0];
        size_t size = (size_t)arguments[1];

        memcpy(vm->stack_pointer + offset, &operand.val, size);
        vm->stack_pointer_type[offset] = (char)operand.op_type;

        break;
    }
    case OP_SP_INCRE: {
        int offset = arguments[0];
        size_t size = (size_t)arguments[1];
        uint64_t value = 0;

        if (offset < 0 || arguments[1] <= 0 || size > sizeof(value)) {
            ok = false;
            break;
        }

        memcpy(&value, (uint8_t *)vm->stack_pointer + offset, size);
        value++;
        memcpy((uint8_t *)vm->stack_pointer + offset, &value, size);

        break;
    }
    case OP_SP_DECRE: {
        int offset = arguments[0];
        size_t size = (size_t)arguments[1];
        uint64_t value = 0;

        if (offset < 0 || arguments[1] <= 0 || size > sizeof(value)) {
            ok = false;
            break;
        }

        memcpy(&value, (uint8_t *)vm->stack_pointer + offset, size);
        value--;
        memcpy((uint8_t *)vm->stack_pointer + offset, &value, size);

        break;
    }
    case OP_LOAD_CLASS: {
        break;
    }
    case OP_INCRE_CLASS: {
        break;
    }
    case OP_DECRE_CLASS: {
        break;
    }
    case OP_SAVE_CLASS: {
        break;
    }
    case OP_LOAD_GLOBAL: {
        break;
    }
    case OP_INCRE_GLOBAL: {
        break;
    }
    case OP_DECRE_GLOBAL: {
        break;
    }
    case OP_SAVE_GLOBAL: {
        break;
    }
    case OP_LOAD_ATTR: {
        break;
    }
    case OP_INCRE_ATTR: {
        break;
    }
    case OP_DECRE_ATTR: {
        break;
    }
    case OP_SAVE_ATTR: {
        break;
    }
    case OP_SYSCALL: {
        if (ok)
            handle_syscall(vm, arguments[0], arguments[1]);
        break;
    }
    case OP_CALL: {
        if (!ok)
            break;

        break;
    }
    case OP_CALL_ATTR: {
        break;
    }
    case OP_CALL_CLASS: {
        break;
    }
    case OP_CALL_GLOBAL: {
        struct VMFunctionData *function_data;
        struct VMOperand *call_arguments;
        uint8_t *frame_pointer;

        unsigned argument_count;
        unsigned frame_size;
        unsigned offset = 0;
        unsigned i;

        if (!ok || arguments[0] < 0 || arguments[1] < 0) {
            ok = false;
            break;
        }

        function_data = vm_find_function_data(vm, NULL, (unsigned)arguments[0]);
        argument_count = (unsigned)arguments[1];
        if (function_data == NULL ||
            argument_count != function_data->argument_count ||
            argument_count > vm->vm_stack->index) {
            ok = false;
            break;
        }

        frame_size = function_data->stack_size;

        call_arguments = argument_count == 0
                             ? NULL
                             : (struct VMOperand *)S_malloc(
                                   sizeof(struct VMOperand) * argument_count);

        for (i = 0; i < argument_count; i++) {
            call_arguments[i] = vm_stack_pop(vm->vm_stack);
            offset += (unsigned)get_operand_size(call_arguments[i].op_type);
        }

        if (offset > frame_size) {
            free(call_arguments);
            ok = false;
            break;
        }

        frame_pointer = (uint8_t *)vm->stack_pointer + frame_size;
        offset = 0;
        for (i = 0; i < argument_count; i++) {
            size_t size = get_operand_size(call_arguments[i].op_type);

            memcpy(frame_pointer + offset, &call_arguments[i].val, size);
            vm->stack_pointer_type[offset] = (char)call_arguments[i].op_type;
            offset += (unsigned)size;
        }

        free(call_arguments);

        vm_exec_function(vm, function_data);

        break;
    }
    case OP_LOAD_STR: {
        if (ok) {
            struct VMOperand operand = {OPRND_String, arguments[0]};

            vm_stack_push(vm->vm_stack, operand);
        }
        break;
    }
    case OP_RET: {
        break;
    }
    case OP_GOTO: {
        *instruction_index = instruction->operands.u32[0];
        break;
    }
    case OP_JE: {
        struct VMOperand cond = vm_stack_pop(vm->vm_stack);

        assert(cond.op_type == OPRND_BOOL);
        if (cond.val)
            *instruction_index = instruction->operands.u32[0];

        break;
    }
    case OP_JNE: {
        struct VMOperand cond = vm_stack_pop(vm->vm_stack);

        assert(cond.op_type == OPRND_BOOL);

        if (!cond.val)
            *instruction_index = instruction->operands.u32[0];

        break;
    }
    case OP_NEG: {
        break;
    }
    case OP_LDC_I4: {
        if (ok) {
            struct VMOperand operand = {OPRND_INT32, arguments[0]};

            vm_stack_push(vm->vm_stack, operand);
        }
        break;
    }
    case OP_LDC_F4: {
        float value = instruction->operands.f32;
        if (ok) {
            struct VMOperand operand = {OPRND_FLOAT32, 0};

            memcpy(&operand.val, &value, sizeof(value));
            vm_stack_push(vm->vm_stack, operand);
        }
        break;
    }
    case OP_LDC_F8: {
        double value = instruction->operands.f64;
        if (ok) {
            struct VMOperand operand = {OPRND_FLOAT64, 0};

            memcpy(&operand.val, &value, sizeof(value));
            vm_stack_push(vm->vm_stack, operand);
        }
        break;
    }
    case OP_NEW_OBJECT: {
        break;
    }
    case OP_NEW_ARRAY: {
        break;
    }
    case OP_ARRAY_LOAD: {
        break;
    }
    case OP_ARRAY_SAVE: {
        break;
    }
    case OP_ARRAY_LENGTH: {
        break;
    }
    case OP_ARRAY_PUSH: {
        break;
    }
    case OP_ARRAY_REMOVE: {
        break;
    }
    default: {
        DEBUG_FPRINTF(stderr,
                      "VM execution error at instruction %u: "
                      "unknown opcode 0x%02x\n",
                      *instruction_index - 1, opcode);
        return false;
    }
    }

    return ok;
}

void vm_exec_function(struct VM *vm, struct VMFunctionData *function_data) {
    unsigned instruction_index = 0;

#ifdef DUMP_STACK
    dump_stack(vm);
#endif

    while (instruction_index < function_data->instruction_count) {
        const struct VMInstruction *instruction =
            &function_data->instructions[instruction_index++];
        bool ok;

#ifdef DEBUG_STAMP_COMMAND
        double started_at = command_time_ms();
#endif

        ok = exec_instruction(vm, instruction, &instruction_index);

#ifdef DEBUG_STAMP_COMMAND
        const char *command_name = get_instruction_name(instruction->opcode);

        fprintf(stderr, "[command %s/0x%02x] %.3f ms\n",
                command_name != NULL ? command_name : "unknown",
                instruction->opcode, command_time_ms() - started_at);
#endif

        if (!ok || instruction->opcode == OP_RET)
            return;
    }
}

void vm_stack_push(struct VMStack *vm_stack, struct VMOperand val) {
    assert(vm_stack->index < 1024 * 256); // 256 KB

    vm_stack->stack[vm_stack->index++] = val;
}

struct VMOperand vm_stack_pop(struct VMStack *vm_stack) {
    assert(vm_stack->index > 0);

    return vm_stack->stack[--vm_stack->index];
}
