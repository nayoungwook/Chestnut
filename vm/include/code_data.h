#ifndef CODE_DATA_H
#define CODE_DATA_H

#include <stdbool.h>

typedef struct VMFunctionData FunctionData;
typedef struct VMClassData ClassData;

struct VMFunctionData {
	unsigned id;

	const char *name;
	const char *return_type;
	const char **argument_types;

	unsigned argument_count;
	bool is_constructor;

	unsigned char *code;
	unsigned code_size;
};

struct VMClassData {
	unsigned id;
	const char *name;
	unsigned parent_id;
	unsigned size;

	FunctionData **function_data;
	unsigned function_data_count;
	unsigned function_data_capacity;
};

#endif
