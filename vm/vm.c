#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef __unix__
#include <sys/mman.h>
#endif

#include <vm.h>
#include <code_data.h>
#include <ir_read.h>
#include <util.h>

#include <stdio.h>
#include <string.h>

//#define DEBUG

#ifdef DEBUG

#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#define DEBUG_FPRINTF(...) fprintf(__VA_ARGS__)

#else

#define DEBUG_PRINTF(...) ((void)0)
#define DEBUG_FPRINTF(...) ((void)0)

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
	vm->heap = mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS , -1, 0);
	vm->stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS , -1, 0);
#endif

#ifdef _WIN32
	HANDLE h_map = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, HEAP_SIZE, NULL);
	HANDLE s_map = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, STACK_SIZE, NULL);

	vm->heap = (uint8_t*) MapViewOfFile(h_map, FILE_MAP_ALL_ACCESS, 0, 0, HEAP_SIZE);
	vm->stack = (uint8_t*) MapViewOfFile(s_map, FILE_MAP_ALL_ACCESS, 0, 0, STACK_SIZE);
#endif

	vm->stack_pointer = vm->stack;
	vm->stack_pointer_type = S_malloc(STACK_SIZE);
	
	vm->vm_stack = (struct VMStack *) S_malloc(sizeof(struct VMStack));
	memset(vm->vm_stack, 0, sizeof(struct VMStack));
	
	vm->vm_string_pool = (struct VMStringPool *) S_malloc(sizeof(struct VMStringPool));
	vm->vm_string_pool->str_pool = NULL;
	memset(vm->jump_table, 0xff, sizeof(vm->jump_table));

	return vm;
}

void reset_string_pool(struct VM *vm, unsigned size) {
	vm->vm_string_pool->size = size;
	vm->vm_string_pool->str_pool = (char **) S_malloc(sizeof(char *) * size);
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

	class_data =
		(struct VMClassData *)S_malloc(sizeof(struct VMClassData));
	class_data->id = id;
	class_data->name = copy_string(name);
	class_data->parent_id = parent_id;
	class_data->size = size;
	class_data->function_data_count = 0;
	class_data->function_data_capacity = 1;
	class_data->function_data = (FunctionData **)S_malloc(
		sizeof(FunctionData *));

	vm->class_data[vm->class_data_count++] = class_data;
	return class_data;
}

struct VMFunctionData *vm_add_function_data(struct VM *vm,
				       struct VMClassData *owner,
				       unsigned id, const char *name,
				       const char *return_type,
				       const char **argument_types,
				       unsigned argument_count,
				       bool is_constructor) {
	struct VMFunctionData *function_data;
	unsigned i;

	function_data =
		(struct VMFunctionData *)S_malloc(sizeof(struct VMFunctionData));
	function_data->id = id;
	function_data->name = copy_string(name);
	function_data->return_type = copy_string(return_type);
	function_data->argument_count = argument_count;
	function_data->is_constructor = is_constructor;
	function_data->code = NULL;
	function_data->code_size = 0;

	if (strcmp(name, "main") == 0) {
		vm->main_func_id = id;
	}

	function_data->argument_types = argument_count == 0 ? NULL :
		(const char **)S_malloc(sizeof(char *) * argument_count);

	for (i = 0; i < argument_count; i++)
		function_data->argument_types[i] = copy_string(argument_types[i]);

	if (owner == NULL) {
		if (vm->function_data_count == vm->function_data_capacity) {
			vm->function_data_capacity *= 2;
			vm->function_data = (struct VMFunctionData **)S_realloc(
				vm->function_data, sizeof(struct VMFunctionData *) *
				vm->function_data_capacity);
		}

		vm->function_data[vm->function_data_count++] = function_data;
	} else {
		if (owner->function_data_count ==
		    owner->function_data_capacity) {
			owner->function_data_capacity *= 2;
			owner->function_data = (FunctionData **)S_realloc(
				owner->function_data, sizeof(FunctionData *) *
				owner->function_data_capacity);
		}

		owner->function_data[owner->function_data_count++] =
			function_data;
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
	struct VMFunctionData **items = owner == NULL ? vm->function_data :
		owner->function_data;

	unsigned count = owner == NULL ? vm->function_data_count :
		owner->function_data_count;
	unsigned i;

	for (i = 0; i < count; i++)
		if (items[i]->id == id)
			return items[i];
	return NULL;
}

void vm_set_function_code(struct VMFunctionData *function_data,
		       const unsigned char *code, unsigned code_size) {
	function_data->code_size = code_size;
	function_data->code = code_size == 0 ? NULL :
		(unsigned char *)S_malloc(code_size);

	if (code_size != 0)
		memcpy(function_data->code, code, code_size);
}

static bool exec_has_bytes(const struct IRReader *reader, unsigned count) {
	return reader->reader_cnt <= reader->byte_cnt &&
		count <= reader->byte_cnt - reader->reader_cnt;
}

static bool exec_read_byte(struct IRReader *reader, byte *value) {
	if (!exec_has_bytes(reader, 1))
		return false;
	*value = reader->bytes[reader->reader_cnt++];
	return true;
}

static bool exec_read_u32(struct IRReader *reader, uint32_t *value) {
	uint32_t result = 0;
	unsigned i;

	if (!exec_has_bytes(reader, 4))
		return false;
	for (i = 0; i < 4; i++)
		result |= (uint32_t)reader->bytes[reader->reader_cnt++] <<
			(i * 8);
	*value = result;
	return true;
}

static bool exec_read_i32(struct IRReader *reader, int32_t *value) {
	uint32_t raw;

	if (!exec_read_u32(reader, &raw))
		return false;
	memcpy(value, &raw, sizeof(*value));
	return true;
}

#ifdef DEBUG
static const char *get_instruction_name(unsigned char opcode) {
	switch (opcode) {
	case OP_SP_PUSH: return "sp_push";
	case OP_SP_POP: return "sp_pop";
	case OP_SP_LOAD: return "sp_load";
	case OP_SP_SAVE: return "sp_save";
	case OP_SP_INCRE: return "sp_incre";
	case OP_SP_DECRE: return "sp_decre";
	case OP_LOAD_CLASS: return "load_class";
	case OP_INCRE_CLASS: return "incre_class";
	case OP_DECRE_CLASS: return "decre_class";
	case OP_SAVE_CLASS: return "save_class";
	case OP_LOAD_GLOBAL: return "load_global";
	case OP_INCRE_GLOBAL: return "incre_global";
	case OP_DECRE_GLOBAL: return "decre_global";
	case OP_SAVE_GLOBAL: return "save_global";
	case OP_LOAD_ATTR: return "load_attr";
	case OP_INCRE_ATTR: return "incre_attr";
	case OP_DECRE_ATTR: return "decre_attr";
	case OP_SAVE_ATTR: return "save_attr";
	case OP_SYSCALL: return "syscall";
	case OP_CALL: return "call";
	case OP_CALL_ATTR: return "call_attr";
	case OP_CALL_CLASS: return "call_class";
	case OP_CALL_GLOBAL: return "call_global";
	case OP_LOAD_STR: return "load_str";
	case OP_GOTO: return "goto";
	case OP_LABEL: return "label";
	case OP_JE: return "je";
	case OP_JNE: return "jne";
	case OP_LDC_I4: return "ldc_i4";
	case OP_NEW_OBJECT: return "new_object";
	case OP_NEW_ARRAY: return "new_array";
	case OP_ARRAY_LOAD: return "array_load";
	case OP_ARRAY_SAVE: return "array_save";
	case OP_ARRAY_PUSH: return "array_push";
	case OP_ARRAY_REMOVE: return "array_remove";
	default: return NULL;
	}
}

static const char *get_expr_op_name(unsigned char opcode) {
	switch (opcode) {
	case OP_ADD: return "add";
	case OP_SUB: return "sub";
	case OP_MUL: return "mul";
	case OP_DIV: return "div";
	case OP_EQUAL: return "equal";
	case OP_NOTEQUAL: return "notequal";
	case OP_GREATER: return "greater";
	case OP_LESS: return "less";
	case OP_EQUALGREATER: return "eqgreater";
	case OP_EQUALLESS: return "eqless";
	case OP_ASSIGN: return "assign";
	case OP_OR: return "or";
	case OP_AND: return "and";
	default: return NULL;
	}
}
#endif

static bool exec_read_i32_values(struct IRReader *reader,
				 unsigned char opcode, int32_t *values,
				 unsigned count) {
	unsigned i;

	for (i = 0; i < count; i++) {
		if (!exec_read_i32(reader, &values[i]))
			return false;
	}

#ifdef DEBUG
	const char *name;
	name = get_instruction_name(opcode);
	if (name == NULL)
		return false;
	DEBUG_PRINTF("%s", name);
	for (i = 0; i < count; i++)
		DEBUG_PRINTF(" %d", values[i]);
	DEBUG_PRINTF("\n");
#endif
	
	return true;
}

static bool exec_read_f32(struct IRReader *reader, float *value) {
	uint32_t raw;

	if (!exec_read_u32(reader, &raw))
		return false;
	
	memcpy(value, &raw, sizeof(*value));
	return true;
}

static bool exec_read_f64(struct IRReader *reader, double *value) {
	uint64_t raw = 0;
	unsigned i;

	if (!exec_has_bytes(reader, 8))
		return false;
	for (i = 0; i < 8; i++)
		raw |= (uint64_t)reader->bytes[reader->reader_cnt++] <<
			(i * 8);
	memcpy(value, &raw, sizeof(*value));
	return true;
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
				const int val = (int) op.val;

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
				printf("print format not supported.\n");
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

#define DEFINE_VM_OPERATOR_SET(prefix, type, member)                         \
static bool prefix##_add(const union VMNumericValue *lhs_value,              \
			 const union VMNumericValue *rhs_value,             \
			 union VMNumericValue *result_value) {               \
	result_value->member = lhs_value->member + rhs_value->member;          \
	return true;                                                            \
}                                                                          \
static bool prefix##_sub(const union VMNumericValue *lhs_value,              \
			 const union VMNumericValue *rhs_value,             \
			 union VMNumericValue *result_value) {               \
	result_value->member = lhs_value->member - rhs_value->member;          \
	return true;                                                            \
}                                                                          \
static bool prefix##_mul(const union VMNumericValue *lhs_value,              \
			 const union VMNumericValue *rhs_value,             \
			 union VMNumericValue *result_value) {               \
	result_value->member = lhs_value->member * rhs_value->member;          \
	return true;                                                            \
}                                                                          \
static bool prefix##_div(const union VMNumericValue *lhs_value,              \
			 const union VMNumericValue *rhs_value,             \
			 union VMNumericValue *result_value) {               \
	if (rhs_value->member == (type)0)                                      \
		return false;                                                    \
	result_value->member = lhs_value->member / rhs_value->member;          \
	return true;                                                            \
}                                                                          \
static bool prefix##_equal(const union VMNumericValue *lhs_value,            \
			   const union VMNumericValue *rhs_value,           \
			   union VMNumericValue *result_value) {             \
	result_value->i32 = lhs_value->member == rhs_value->member;            \
	return true;                                                            \
}                                                                          \
static bool prefix##_notequal(const union VMNumericValue *lhs_value,         \
			      const union VMNumericValue *rhs_value,        \
			      union VMNumericValue *result_value) {          \
	result_value->i32 = lhs_value->member != rhs_value->member;            \
	return true;                                                            \
}                                                                          \
static bool prefix##_greater(const union VMNumericValue *lhs_value,          \
			     const union VMNumericValue *rhs_value,         \
			     union VMNumericValue *result_value) {           \
	result_value->i32 = lhs_value->member > rhs_value->member;             \
	return true;                                                            \
}                                                                          \
static bool prefix##_less(const union VMNumericValue *lhs_value,             \
			  const union VMNumericValue *rhs_value,            \
			  union VMNumericValue *result_value) {              \
	result_value->i32 = lhs_value->member < rhs_value->member;             \
	return true;                                                            \
}                                                                          \
static bool prefix##_equalgreater(                                          \
		const union VMNumericValue *lhs_value,                          \
		const union VMNumericValue *rhs_value,                          \
		union VMNumericValue *result_value) {                           \
	result_value->i32 = lhs_value->member >= rhs_value->member;            \
	return true;                                                            \
}                                                                          \
static bool prefix##_equalless(const union VMNumericValue *lhs_value,        \
			       const union VMNumericValue *rhs_value,       \
			       union VMNumericValue *result_value) {         \
	result_value->i32 = lhs_value->member <= rhs_value->member;            \
	return true;                                                            \
}                                                                          \
static bool prefix##_or(const union VMNumericValue *lhs_value,               \
			const union VMNumericValue *rhs_value,              \
			union VMNumericValue *result_value) {                \
	result_value->i32 = lhs_value->member != (type)0 ||                     \
		rhs_value->member != (type)0;                                    \
	return true;                                                            \
}                                                                          \
static bool prefix##_and(const union VMNumericValue *lhs_value,              \
			 const union VMNumericValue *rhs_value,             \
			 union VMNumericValue *result_value) {               \
	result_value->i32 = lhs_value->member != (type)0 &&                     \
		rhs_value->member != (type)0;                                    \
	return true;                                                            \
}

DEFINE_VM_OPERATOR_SET(i32, int32_t, i32)
DEFINE_VM_OPERATOR_SET(f32, float, f32)
DEFINE_VM_OPERATOR_SET(f64, double, f64)

#undef DEFINE_VM_OPERATOR_SET

static const VMOperatorFunc vm_operator_table[3][VM_OPERATOR_COUNT] = {
	{
		i32_add, i32_sub, i32_mul, i32_div,
		i32_equal, i32_notequal, i32_greater, i32_less,
		i32_equalgreater, i32_equalless, i32_or, i32_and,
	},
	{
		f32_add, f32_sub, f32_mul, f32_div,
		f32_equal, f32_notequal, f32_greater, f32_less,
		f32_equalgreater, f32_equalless, f32_or, f32_and,
	},
	{
		f64_add, f64_sub, f64_mul, f64_div,
		f64_equal, f64_notequal, f64_greater, f64_less,
		f64_equalgreater, f64_equalless, f64_or, f64_and,
	},
};

static int get_vm_operator_index(byte expr_opcode) {
	switch (expr_opcode) {
	case OP_ADD: return VM_OPERATOR_ADD;
	case OP_SUB: return VM_OPERATOR_SUB;
	case OP_MUL: return VM_OPERATOR_MUL;
	case OP_DIV: return VM_OPERATOR_DIV;
	case OP_EQUAL: return VM_OPERATOR_EQUAL;
	case OP_NOTEQUAL: return VM_OPERATOR_NOTEQUAL;
	case OP_GREATER: return VM_OPERATOR_GREATER;
	case OP_LESS: return VM_OPERATOR_LESS;
	case OP_EQUALGREATER: return VM_OPERATOR_EQUALGREATER;
	case OP_EQUALLESS: return VM_OPERATOR_EQUALLESS;
	case OP_OR: return VM_OPERATOR_OR;
	case OP_AND: return VM_OPERATOR_AND;
	default: return -1;
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
				   int op_level,
				   union VMNumericValue *value) {
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
		memcpy(&result->val, &result_value->f32,
		       sizeof(result_value->f32));
		break;
	}
	case 2: {
		result->op_type = OPRND_FLOAT64;
		memcpy(&result->val, &result_value->f64,
		       sizeof(result_value->f64));
		break;
	}
	}
}

bool exec_instruction(struct VM *vm, struct IRReader *reader,
		      unsigned char opcode) {
	int32_t arguments[5] = {0};
	bool ok = true;

	assert(vm->vm_stack != NULL);

	switch (opcode) {
	case OP_PUSH_NULL: {
		struct VMOperand operand = {OPRND_NULL, 0};

		DEBUG_PRINTF("push_null\n");
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

		ok = exec_read_byte(reader, &expr_opcode);
		if (!ok)
			break;

#ifdef DEBUG
		{
			const char *name = get_expr_op_name(expr_opcode);

			if (name == NULL) {
				ok = false;
				break;
			}
			DEBUG_PRINTF("%s\n", name);
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
		ok = exec_read_i32_values(reader, opcode, arguments, 1);
		if (ok && arguments[0] >= 0)
			vm->stack_pointer =
				(uint8_t *)vm->stack_pointer + arguments[0];
		else
			ok = false;
		
		break;
	}
	case OP_SP_POP: {
		ok = exec_read_i32_values(reader, opcode, arguments, 1);

		if (ok && arguments[0] >= 0)
			vm->stack_pointer =
				vm->stack_pointer - arguments[0];
		else
			ok = false;
		break;
	}
	case OP_SP_LOAD: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);

		assert(arguments[0] >= 0 && arguments[1] >= 0 &&
		       (size_t)arguments[1] <= sizeof(int64_t));

		if(!ok) break;
		
		int offset = arguments[0];
		size_t size = (size_t)arguments[1];
		
		int64_t value = 0;
		enum VMOPType type =
			(enum VMOPType)vm->stack_pointer_type[offset];
		struct VMOperand operand;

		memcpy(&value, vm->stack_pointer + offset, size);
		operand.op_type = type;
		operand.val = value;
		vm_stack_push(vm->vm_stack, operand);

		
		break;
	}
	case OP_SP_SAVE: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);

		assert(arguments[0] >= 0 && arguments[1] >= 0 &&
		       (size_t)arguments[1] <= sizeof(int64_t));

		if(!ok)
			break;
		
		struct VMOperand operand = vm_stack_pop(vm->vm_stack);
		int offset = arguments[0];
		size_t size = (size_t)arguments[1];

		memcpy(vm->stack_pointer + offset,
		       &operand.val, size);
		vm->stack_pointer_type[offset] = (char)operand.op_type;
		
		break;
	}
	case OP_SP_INCRE: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);

		int offset = arguments[0];
		size_t size = (size_t)arguments[1];
		
		int64_t value = 0;
		memcpy(&value, (uint8_t *)vm->stack_pointer + offset, size);
		value++;
		
		memcpy(vm->stack_pointer + offset, &value, size);
			
		break;
	}
	case OP_SP_DECRE: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		
		int offset = arguments[0];
		size_t size = (size_t)arguments[1];
		
		int64_t value = 0;
		memcpy(&value, (uint8_t *)vm->stack_pointer + offset, size);
		value--;
		
		memcpy(vm->stack_pointer + offset, &value, size);

		break;
	}
	case OP_LOAD_CLASS: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_INCRE_CLASS: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_DECRE_CLASS: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_SAVE_CLASS: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_LOAD_GLOBAL: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_INCRE_GLOBAL: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_DECRE_GLOBAL: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_SAVE_GLOBAL: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_LOAD_ATTR: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_INCRE_ATTR: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_DECRE_ATTR: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_SAVE_ATTR: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_SYSCALL: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		if (ok)
			handle_syscall(vm, arguments[0], arguments[1]);
		break;
	}
	case OP_CALL: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_CALL_ATTR: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_CALL_CLASS: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_CALL_GLOBAL: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_LOAD_STR: {
		ok = exec_read_i32_values(reader, opcode, arguments, 1);
		if (ok) {
			struct VMOperand operand = {OPRND_String, arguments[0]};

			vm_stack_push(vm->vm_stack, operand);
		}
		break;
	}
	case OP_RET: {
		DEBUG_PRINTF("ret\n");
		break;
	}
	case OP_RET_VAL: {
		DEBUG_PRINTF("ret_val\n");
		break;
	}
	case OP_GOTO: {
		ok = exec_read_i32_values(reader, opcode, arguments, 1);

		assert(arguments[0] > 0);
		
		if(ok){
			vm->ir_reader->reader_cnt = vm->jump_table[arguments[0]];
		}
		
		break;
	}
	case OP_LABEL: {
		ok = exec_read_i32_values(reader, opcode, arguments, 1);
		break;
	}
	case OP_JE: {
		ok = exec_read_i32_values(reader, opcode, arguments, 1);

		struct VMOperand cond = vm_stack_pop(vm->vm_stack);

		assert(cond.op_type == OPRND_BOOL);
		if(cond.val){
			vm->ir_reader->reader_cnt = vm->jump_table[arguments[0]];
		}
		
		break;
	}
	case OP_JNE: {
		ok = exec_read_i32_values(reader, opcode, arguments, 1);
		
		struct VMOperand cond = vm_stack_pop(vm->vm_stack);

		assert(cond.op_type == OPRND_BOOL);

		if(!cond.val){
			vm->ir_reader->reader_cnt = vm->jump_table[arguments[0]];
		}

		break;
	}
	case OP_NEG: {
		DEBUG_PRINTF("neg\n");
		break;
	}
	case OP_LDC_I4: {
		ok = exec_read_i32_values(reader, opcode, arguments, 1);
		if (ok) {
			struct VMOperand operand = {OPRND_INT32, arguments[0]};

			vm_stack_push(vm->vm_stack, operand);
		}
		break;
	}
	case OP_LDC_F4: {
		float value;

		ok = exec_read_f32(reader, &value);
		if (ok) {
			struct VMOperand operand = {OPRND_FLOAT32, 0};
			
			DEBUG_PRINTF("ldc_f4 %.9g\n", value);
			memcpy(&operand.val, &value, sizeof(value));
			vm_stack_push(vm->vm_stack, operand);
		}
		break;
	}
	case OP_LDC_F8: {
		double value;

		ok = exec_read_f64(reader, &value);
		if (ok) {
			struct VMOperand operand = {OPRND_FLOAT64, 0};

			DEBUG_PRINTF("ldc_f8 %.17g\n", value);
			memcpy(&operand.val, &value, sizeof(value));
			vm_stack_push(vm->vm_stack, operand);
		}
		break;
	}
	case OP_NEW_OBJECT: {
		ok = exec_read_i32_values(reader, opcode, arguments, 5);
		break;
	}
	case OP_NEW_ARRAY: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_ARRAY_LOAD: {
		ok = exec_read_i32_values(reader, opcode, arguments, 1);
		break;
	}
	case OP_ARRAY_SAVE: {
		ok = exec_read_i32_values(reader, opcode, arguments, 1);
		break;
	}
	case OP_ARRAY_LENGTH: {
		DEBUG_PRINTF("array_length\n");
		break;
	}
	case OP_ARRAY_PUSH: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	case OP_ARRAY_REMOVE: {
		ok = exec_read_i32_values(reader, opcode, arguments, 2);
		break;
	}
	default: {
		DEBUG_FPRINTF(stderr, "VM execution error at byte %u: "
			      "unknown opcode 0x%02x\n",
			      reader->reader_cnt - 1, opcode);
		return false;
	}
	}

	return ok;
}

void vm_exec_function(struct VM *vm,
                      struct VMFunctionData *function_data) {
	assert(vm->ir_reader != NULL);

	vm->ir_reader->bytes = function_data->code;
	vm->ir_reader->reader_cnt = 0;
	vm->ir_reader->byte_cnt = function_data->code_size;

	while(vm->ir_reader->reader_cnt < function_data->code_size){
		byte op_code = vm->ir_reader->bytes[vm->ir_reader->reader_cnt];
		vm->ir_reader->reader_cnt++;
		if (!exec_instruction(vm, vm->ir_reader, op_code))
			return;
	}
}        

void vm_stack_push(struct VMStack *vm_stack, struct VMOperand val){
	assert(vm_stack->index < 1024 * 256); // 256 KB

	vm_stack->stack[vm_stack->index++] = val;
}

struct VMOperand vm_stack_pop(struct VMStack *vm_stack){
	assert(vm_stack->index > 0);
	
	return vm_stack->stack[--vm_stack->index];
}
