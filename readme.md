# Chestnut

Chestnut is a programming language compiler and virtual machine (VM) written in C. It compiles `.cn` source files into bytecode. The repository also includes a VM.

The project is currently focused on core language and runtime development. The goal is to add **JIT compilation and SDL3 integration**, and **redesign the heap and stack for game development**, expanding Chestnut into an environment for building and running games.

## Current Features

The following features reflect the current source code. Syntax and behavior may change as development continues.

| Area | Implementation |
| --- | --- |
| Compilation pipeline | Tokenization, parsing, AST construction, semantic analysis, IR and bytecode generation |
| Type checking | Checks for variable assignments, function and constructor arguments, and return values |
| Core syntax | `var`, `func`, `return`, `if` / `else`, `for` |
| Operators | Arithmetic, comparison, logical, increment and decrement operators; `+=`, `-=`, `*=`, `/=` |
| Functions | Arguments, return values, and recursive calls |
| Objects | Classes, field initialization, constructors, `new`, inheritance, method overriding, and `super(...)` |
| Arrays | `array<T>`, array literals, indexed reads and writes, and `length` |
| Output | Built-in `print(...)` function |
| Multiple source files | Compile multiple `.cn` files in one shared context with separate outputs |
| VM | Bytecode interpretation, function and class metadata, string pooling, and heap and stack management |
| Debugging | Compiler metadata and VM bytecode output, with optional instruction timing |

## Building and Running

### Requirements

- GCC and GNU Make.
- On Windows, a MinGW GCC toolchain and Make can be used.
- The VM contains platform-specific memory mapping paths for Windows and Unix/macOS (`mmap`). Build and runtime behavior should be verified on each platform.
- The current build does not require SDL3 or a JIT library.

Build from the project root:

```sh
make
```

The Makefile uses `gcc -Wall -O2` by default and produces `chestnut.exe` on Windows or `chestnut` on other platforms. If your Make executable is named `mingw32-make`, use that command instead of `make`.

The CLI compiles all `.cn` inputs in one shared context and writes bytecode beside each source: `test.cn` becomes `test.cb`. Class IDs start at 1 in each file. For example, `chestnut foo.cn bar.cn` produces `foo.cb` and `bar.cb`. Source files share symbols and types, so references across files are resolved during compilation. Each output contains the shared class metadata using ordinary `META_CLASS` entries, with sequential class IDs starting at 1, and its own code. Pass the related `.cb` files together to supply all class and function bodies. No separate class-reference metadata is emitted. A `.cn` input compiles without executing. A `.cb` input runs its `main` function on the VM: `chestnut test.cb`. All `.cn` inputs are compiled together before any `.cb` input is executed. For example, `chestnut test.cn test.cb` compiles and then runs the program. All `.cb` inputs load into one VM: metadata is registered first and `main` runs once. Matching class names reuse the same runtime class; conflicting IDs for different classes are reassigned and class operands are rewritten through a per-file ID map. Class and string references are relocated per file. For example, run `chestnut test.cn test2.cn`, then `chestnut test.cb test2.cb`. Recompile older bytecode to use the shared `META_CLASS` metadata layout. With no arguments, the CLI prints usage. Other extensions are rejected.

## Syntax Example

Save the following as `hello.cn` and compile it with `./chestnut hello.cn` or `.\chestnut.exe hello.cn`:

```text
func main(): void {
    var values: array<int> = {2, 3, 4};
    var total: int = 0;

    for (var i: int = 0; i < values.length; i++) {
        total = total + values[i];
    }

    if (total > 0) {
        print("total: ", total, "\n");
    }
}
```

The repository includes these examples:

| File | Covers |
| --- | --- |
| [`test_basic.cn`](test_basic.cn) | Output, variables, loops, conditionals, and numeric operations |
| [`fibo.cn`](fibo.cn) | A recursive Fibonacci function |
| [`test.cn`](test.cn), [`test2.cn`](test2.cn) | Shared-context cross-file compilation example, classes, inheritance, constructors, and arrays |
| [`tests/class_initializer.cn`](tests/class_initializer.cn) | Field initialization across several types and nested objects |
| [`tests/compound_assign.cn`](tests/compound_assign.cn) | Compound assignment operators |
| [`tests/type_error.cn`](tests/type_error.cn) | A return type mismatch error case |
| [`tests/todo_features.cn`](tests/todo_features.cn) | Source cases for inheritance and arrays; no standalone `main` function |

## Architecture

```text
.cn source files
  → Tokenization and declaration collection
  → Parsing / AST construction
  → Symbol registration and semantic / type checking
  → IR / bytecode generation
  → One .cb output per source file
```

### Current Memory Model

- The VM reserves a fixed **16 MiB heap** and a **1 MiB stack** for function execution by default.
- A separate operand stack holds **262,144 `VMOperand` entries** for expression evaluation.
- Heap allocation advances an allocation pointer. Objects are referenced through handles in a mapping table.
- `vm_free` makes handles available for reuse, but does not reclaim or compact the allocated heap space. Automatic garbage collection is not implemented yet.

Memory constants and structures are defined in [`vm/include/vm.h`](vm/include/vm.h). Allocation logic is in [`vm/heap.c`](vm/heap.c).

## Current Limitations

- **JIT compilation and SDL3 integration are not implemented yet.** Execution currently uses an interpreter.
- The compiler recognizes array `push(...)` and `remove(...)` calls and emits instructions, but their VM handlers are empty. Calls in existing examples do not imply that these array operations are complete.
- Fixed memory regions and a simple allocator require improvements to memory reuse and lifetime management for long-running games.
- `tests/` contains source cases; the Makefile does not provide an automated test target.
- The root `todo` file includes some features that have already been implemented. Check the source code when determining feature status.

## Roadmap

The main goals are **JIT compilation, SDL3 integration, and heap and stack improvements for game development**. The items below describe proposed work toward those goals. Implementation details and priorities may change based on development and profiling results.

## Contributing

Contributions can include bug fixes, language and VM improvements, tests, performance measurements, and documentation.

1. For bugs, open an issue with a minimal `.cn` reproduction, the command used, expected and actual results, and your OS and compiler details.
2. Discuss major syntax changes or JIT and memory architecture changes in an issue before implementation.
3. Fork the repository or create a working branch, keeping each change focused on one purpose.
4. Run the relevant examples and add reproduction or regression cases under `tests/` when behavior changes.
5. Describe the purpose, resulting behavior, validation commands and results, and remaining limitations in your pull request.

### Coding Guidelines

- Set the tab width to 4 spaces.
- Use K&R brace style.
- Use descriptive variable names, even when they are long.
- Add brief comments to functions with complex logic and to complex structs to explain their purpose or behavior.
- AI tools are allowed, but contributors **must personally perform an initial review of all AI-generated changes before submitting them.**
- Delegating the entire task to AI is strictly prohibited. AI-assisted work must be guided by the contributor's understanding of the codebase, and contributors must understand and take responsibility for every change they submit.

## License

Chestnut is licensed under the [MIT License](LICENSE).
