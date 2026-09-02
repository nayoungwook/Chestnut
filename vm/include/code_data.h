#ifndef CODE_DATA_H
#define CODE_DATA_H

#include <stdbool.h>
#include <stdint.h>

#define VM_INSTRUCTION_MAX_OPERANDS 5

typedef struct VMFunctionData FunctionData;
typedef struct VMClassData ClassData;
typedef struct VMVariableData VariableData;

union VMInstructionOperands {
    uint8_t u8;
    uint32_t u32[VM_INSTRUCTION_MAX_OPERANDS];
    int32_t i32[VM_INSTRUCTION_MAX_OPERANDS];
    float f32;
    double f64;
};

struct VMInstruction {
    uint8_t opcode;
    uint8_t operand_count;
    union VMInstructionOperands operands;
};

struct VMFunctionData {
    unsigned id;
    unsigned stack_size;

    const char *name;
    const char *return_type;
    const char **argument_types;

    unsigned argument_count;
    bool is_constructor;

    struct VMInstruction *instructions;
    unsigned instruction_count;
};

struct VMVariableData {
    unsigned id;
    unsigned offset;
    unsigned size;
    unsigned operand_type;

    const char *name;
    const char *type;
};

struct VMClassData {
    unsigned id;
    const char *name;
    unsigned parent_id;
    unsigned size;

    FunctionData *initializer;

    FunctionData **function_data;
    /* Member functions are stored in slots indexed by function id. */
    unsigned function_data_count;
    unsigned function_data_capacity;

    VariableData **variable_data;
    unsigned variable_data_count;
    unsigned variable_data_capacity;
};

#endif
