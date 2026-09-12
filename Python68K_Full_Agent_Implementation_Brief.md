# Python68K: Full Agent Implementation Brief

## 1. Mission

Create **Python68K**, a lightweight interpreter for a deliberately restricted Python-compatible language, implemented in portable C and compiled with **vbcc** for classic Amiga computers using a plain Motorola 68000 CPU.

The resulting AmigaDOS command must support:

```text
1> python hello.py
Hello from Python68K

1> python -c "print(2 + 3 * 4)"
14

1> python -V
Python68K 0.1.0
```

Python68K is not CPython and must not be described as a complete Python implementation. It implements a frozen, documented subset called **Python68K Language Level 0.1**.

### Primary target

```text
CPU: Motorola 68000
FPU: none
OS: AmigaOS 2.x or newer
C compiler: vbcc
Assembler: vasmm68k_mot
Linker: vlink
Executable format: Amiga Hunk
Runtime mode: AmigaDOS CLI application
```

### Secondary target

The same core runtime must compile on a modern host with GCC or Clang. Host builds are used for unit tests, sanitizers, fuzzing, memory-failure injection, and rapid development.

---

## 2. Non-negotiable architecture

```text
.py source
    |
    v
source loader
    |
    v
tokenizer: tokens + INDENT/DEDENT
    |
    v
recursive-descent statement parser + Pratt expression parser
    |
    v
AST allocated in a temporary arena
    |
    v
symbol and control-flow validation
    |
    v
AST-to-bytecode compiler
    |
    v
bytecode verifier + maximum-stack calculation
    |
    v
release source, tokens, and AST when no longer needed
    |
    v
stack-based virtual machine
    |
    +--> scalar values
    +--> reference-counted strings and lists
    +--> functions and call frames
    +--> native built-ins
    +--> host or Amiga platform layer
```

Mandatory decisions:

1. Production implementation is portable C.
2. Do not embed, port, or depend on CPython or MicroPython.
3. Do not interpret source directly during normal execution.
4. Compile the complete source unit to custom bytecode first.
5. Verify bytecode before executing it.
6. Use a switch-based stack VM initially.
7. Use signed 32-bit integers with checked arithmetic.
8. Use an explicitly documented 8-bit string encoding in version 0.1.
9. Use reference counting for heap values.
10. Reject cyclic lists until a cycle collector exists.
11. Route all allocations through one tracked allocator.
12. Keep all Amiga-specific headers and APIs below the platform interface.
13. Explicitly target `68000` and no FPU in release builds.
14. Never perform an unaligned word or longword read.
15. Unsupported constructs must produce intentional diagnostics.
16. Diagnostic locations are filename, line, and column.
17. Every allocation, stack growth, bytecode operand, and array index is checked.
18. No optimization may change documented semantics.

---

## 3. Supported language

### 3.1 Statements

```python
name = expression
name += expression
name -= expression
name *= expression
name //= expression
name %= expression

if condition:
    statements
elif condition:
    statements
else:
    statements

while condition:
    statements
else:
    statements

for name in range(...):
    statements
else:
    statements

def function(parameter1, parameter2):
    statements

return
return expression
break
continue
pass
expression
```

The loop `else` clauses may be postponed until after the core MVP, but the parser must either implement them correctly or reject them deliberately.

### 3.2 Expressions

```python
123
0x1234
0b1010
0o755
"text"
'text'
True
False
None
name

+value
-value
not value

left + right
left - right
left * right
left // right
left % right

left == right
left != right
left < right
left <= right
left > right
left >= right

left and right
left or right

function()
function(a, b)
list_value[index]
list_value[start:end]
[a, b, c]
(expression)
```

### 3.3 Runtime value types

```text
None
Boolean
signed 32-bit integer
8-bit string
list
range iterator/state
user function
native function
file handle, after core MVP
```

“No objects” means no user-visible general object system. Internal heap structures are still called objects in the implementation.

### 3.4 Unsupported in language level 0.1

```text
class and instances
import and modules
try, except, finally, raise
with
lambda
closures and nonlocal
async and await
yield and generators
match
comprehensions
sets
dictionaries
f-strings
bytes and bytearray
decorators
annotations
multiple assignment and unpacking
keyword arguments
default arguments
*args and **kwargs
global and nonlocal
eval and exec
arbitrary attribute access
user-defined operator overloading
arbitrary-precision integers
floating point
Unicode
```

Each recognized but unsupported keyword should have a targeted diagnostic instead of a generic parse error.

---

## 4. Full repository skeleton

```text
python68k/
├── README.md
├── LICENSE
├── CHANGELOG.md
├── Makefile
├── Makefile.host
├── Makefile.amiga
├── pyproject.toml
├── .editorconfig
├── .gitignore
├── config/
│   ├── py68k_config_default.h
│   ├── py68k_config_host.h
│   ├── py68k_config_amiga.h
│   └── vbcc/
│       ├── README.md
│       ├── amiga-debug.opts
│       └── amiga-release.opts
├── docs/
│   ├── architecture.md
│   ├── language-reference.md
│   ├── grammar.ebnf
│   ├── bytecode.md
│   ├── bytecode-file-format.md
│   ├── memory-model.md
│   ├── ownership.md
│   ├── diagnostics.md
│   ├── amiga-build.md
│   ├── host-build.md
│   ├── compatibility.md
│   ├── testing.md
│   └── porting.md
├── include/
│   ├── py68k_config.h
│   ├── py68k_types.h
│   ├── py68k_limits.h
│   ├── py68k_status.h
│   ├── py68k_location.h
│   ├── py68k_error.h
│   ├── py68k_memory.h
│   ├── py68k_buffer.h
│   ├── py68k_source.h
│   ├── py68k_token.h
│   ├── py68k_tokenizer.h
│   ├── py68k_ast.h
│   ├── py68k_ast_arena.h
│   ├── py68k_parser.h
│   ├── py68k_symbol.h
│   ├── py68k_value.h
│   ├── py68k_object.h
│   ├── py68k_string.h
│   ├── py68k_list.h
│   ├── py68k_function.h
│   ├── py68k_native.h
│   ├── py68k_code.h
│   ├── py68k_opcode.h
│   ├── py68k_compiler.h
│   ├── py68k_verify.h
│   ├── py68k_frame.h
│   ├── py68k_vm.h
│   ├── py68k_builtin.h
│   ├── py68k_traceback.h
│   ├── py68k_runtime.h
│   ├── py68k_platform.h
│   └── py68k_disassemble.h
├── src/
│   ├── main.c
│   ├── config.c
│   ├── status.c
│   ├── error.c
│   ├── memory.c
│   ├── buffer.c
│   ├── source.c
│   ├── token.c
│   ├── tokenizer.c
│   ├── ast.c
│   ├── ast_arena.c
│   ├── parser.c
│   ├── parser_statement.c
│   ├── parser_expression.c
│   ├── symbol.c
│   ├── value.c
│   ├── object.c
│   ├── string.c
│   ├── list.c
│   ├── function.c
│   ├── native.c
│   ├── code.c
│   ├── opcode.c
│   ├── compiler.c
│   ├── compiler_statement.c
│   ├── compiler_expression.c
│   ├── verify.c
│   ├── frame.c
│   ├── vm.c
│   ├── vm_arithmetic.c
│   ├── builtin.c
│   ├── builtin_io.c
│   ├── traceback.c
│   ├── runtime.c
│   └── disassemble.c
├── platform/
│   ├── host/
│   │   ├── platform_host.c
│   │   ├── console_host.c
│   │   ├── file_host.c
│   │   └── clock_host.c
│   └── amiga/
│       ├── platform_amiga.c
│       ├── console_amiga.c
│       ├── file_amiga.c
│       ├── clock_amiga.c
│       ├── amiga_startup.c
│       └── amiga_compat.h
├── generated/
│   ├── opcode_names.inc
│   ├── opcode_widths.inc
│   └── opcode_stack_effects.inc
├── tests/
│   ├── unit/
│   │   ├── test_types.c
│   │   ├── test_memory.c
│   │   ├── test_buffer.c
│   │   ├── test_tokenizer.c
│   │   ├── test_parser.c
│   │   ├── test_arithmetic.c
│   │   ├── test_string.c
│   │   ├── test_list.c
│   │   ├── test_compiler.c
│   │   ├── test_verifier.c
│   │   └── test_vm.c
│   ├── language/
│   │   ├── literals/
│   │   ├── operators/
│   │   ├── control_flow/
│   │   ├── functions/
│   │   ├── strings/
│   │   └── lists/
│   ├── negative/
│   │   ├── tokenizer/
│   │   ├── parser/
│   │   ├── compiler/
│   │   ├── verifier/
│   │   └── runtime/
│   ├── memory/
│   ├── differential/
│   ├── integration/
│   │   ├── host/
│   │   └── amiga/
│   └── fixtures/
├── examples/
│   ├── hello.py
│   ├── arithmetic.py
│   ├── conditionals.py
│   ├── loops.py
│   ├── fibonacci.py
│   ├── strings.py
│   ├── lists.py
│   └── arguments.py
└── tools/
    ├── run_tests.py
    ├── compare_cpython.py
    ├── generate_opcodes.py
    ├── inspect_bytecode.py
    ├── make_adf.py
    ├── run_fsuae.py
    └── check_68000_binary.py
```

---

## 5. Portable C rules

Use conservative ANSI C. Partial C99 support may be used only after verifying it with the selected vbcc release.

Required project integer types:

```c
typedef signed char Py68I8;
typedef unsigned char Py68U8;
typedef signed short Py68I16;
typedef unsigned short Py68U16;
typedef signed long Py68I32;
typedef unsigned long Py68U32;
```

Compile-time checks:

```c
typedef char Py68AssertChar8[(sizeof(char) == 1) ? 1 : -1];
typedef char Py68AssertShort16[(sizeof(short) == 2) ? 1 : -1];
typedef char Py68AssertLong32[(sizeof(long) == 4) ? 1 : -1];
```

Do not require variable-length arrays, C11 atomics, threads, `_Generic`, anonymous structures, compiler-specific packed structures, POSIX APIs, `mmap`, or computed goto.

Do not use signed C overflow as language behavior. Do not cast arbitrary byte pointers to word or longword pointers.

---

## 6. Core C structures

### 6.1 Status and location

```c
typedef enum Py68Status {
    PY68_STATUS_OK = 0,
    PY68_STATUS_EXIT = 1,
    PY68_STATUS_SOURCE_ERROR = 10,
    PY68_STATUS_RUNTIME_ERROR = 11,
    PY68_STATUS_MEMORY_ERROR = 12,
    PY68_STATUS_INTERNAL_ERROR = 20
} Py68Status;

typedef struct Py68Location {
    Py68U32 offset;
    Py68U32 line;
    Py68U16 column;
    Py68U16 length;
} Py68Location;
```

### 6.2 Error object

```c
typedef enum Py68ErrorKind {
    PY68_ERROR_NONE,
    PY68_ERROR_TOKEN,
    PY68_ERROR_SYNTAX,
    PY68_ERROR_NAME,
    PY68_ERROR_TYPE,
    PY68_ERROR_VALUE,
    PY68_ERROR_INDEX,
    PY68_ERROR_ZERO_DIVISION,
    PY68_ERROR_OVERFLOW,
    PY68_ERROR_RECURSION,
    PY68_ERROR_MEMORY,
    PY68_ERROR_IO,
    PY68_ERROR_BYTECODE,
    PY68_ERROR_INTERNAL
} Py68ErrorKind;

typedef struct Py68Error {
    Py68U16 kind;
    Py68U16 active;
    Py68Location location;
    const char *filename;       /* borrowed */
    char message[192];
} Py68Error;
```

### 6.3 Memory subsystem

```c
typedef enum Py68MemoryTag {
    PY68_MEM_RUNTIME,
    PY68_MEM_SOURCE,
    PY68_MEM_TOKEN,
    PY68_MEM_AST,
    PY68_MEM_SYMBOL,
    PY68_MEM_CODE,
    PY68_MEM_CONSTANT,
    PY68_MEM_STRING,
    PY68_MEM_LIST,
    PY68_MEM_FUNCTION,
    PY68_MEM_STACK,
    PY68_MEM_TEMP,
    PY68_MEM_TAG_COUNT
} Py68MemoryTag;

typedef struct Py68MemoryStats {
    Py68U32 current_bytes;
    Py68U32 peak_bytes;
    Py68U32 allocation_count;
    Py68U32 free_count;
    Py68U32 failed_count;
    Py68U32 bytes_by_tag[PY68_MEM_TAG_COUNT];
} Py68MemoryStats;

typedef struct Py68Allocator {
    Py68U32 memory_limit;
    Py68U32 fail_after_allocation;
    Py68MemoryStats stats;
} Py68Allocator;
```

API:

```c
void *py68_alloc(Py68Runtime *, Py68MemoryTag, Py68U32 size);
void *py68_realloc(Py68Runtime *, Py68MemoryTag,
                   void *, Py68U32 old_size, Py68U32 new_size);
void py68_free(Py68Runtime *, Py68MemoryTag, void *, Py68U32 size);
```

Check `count * element_size` before all array allocations.

### 6.4 Source and tokens

```c
typedef struct Py68Source {
    char *filename;             /* owned */
    Py68U8 *data;               /* owned; extra trailing NUL */
    Py68U32 length;
} Py68Source;

typedef struct Py68Token {
    Py68U16 kind;
    Py68U16 flags;
    Py68Location location;
} Py68Token;

typedef struct Py68TokenArray {
    Py68Token *items;
    Py68U32 count;
    Py68U32 capacity;
} Py68TokenArray;
```

Names and source strings refer to source spans during parsing. Copy them only when a code object or runtime value must outlive the source buffer.

### 6.5 AST list and common header

```c
typedef struct Py68AstNode Py68AstNode;

typedef struct Py68AstList {
    Py68AstNode **items;
    Py68U16 count;
    Py68U16 capacity;
} Py68AstList;

struct Py68AstNode {
    Py68U16 kind;
    Py68U16 flags;
    Py68Location location;
    union {
        struct { Py68AstList statements; } module;
        struct { Py68U32 name_offset; Py68U16 name_length;
                 Py68AstNode *value; } assign;
        struct { Py68U16 operator_kind; Py68AstNode *target;
                 Py68AstNode *value; } augmented_assign;
        struct { Py68AstNode *condition; Py68AstList body;
                 Py68AstList else_body; } if_statement;
        struct { Py68AstNode *condition; Py68AstList body;
                 Py68AstList else_body; } while_statement;
        struct { Py68U32 name_offset; Py68U16 name_length;
                 Py68AstNode *iterable; Py68AstList body;
                 Py68AstList else_body; } for_statement;
        struct { Py68U32 name_offset; Py68U16 name_length;
                 Py68AstList parameters; Py68AstList body; } function_def;
        struct { Py68AstNode *value; } return_statement;
        struct { Py68AstNode *value; } expression_statement;
        struct { Py68I32 value; } integer_literal;
        struct { Py68U32 offset; Py68U16 length; Py68U16 quote_flags; }
            string_literal;
        struct { Py68U32 offset; Py68U16 length; } name;
        struct { Py68U16 value; } boolean_literal;
        struct { Py68U16 operator_kind; Py68AstNode *operand; } unary;
        struct { Py68U16 operator_kind; Py68AstNode *left;
                 Py68AstNode *right; } binary;
        struct { Py68AstNode *callee; Py68AstList arguments; } call;
        struct { Py68AstNode *container; Py68AstNode *index; } index;
        struct { Py68AstNode *container; Py68AstNode *start;
                 Py68AstNode *end; } slice;
        struct { Py68AstList elements; } list_literal;
    } as;
};
```

Use a chunked AST arena. Individual AST nodes are not freed. Destroy the complete arena after successful bytecode compilation or on parse failure.

### 6.6 Value and object model

```c
typedef enum Py68ValueType {
    PY68_VALUE_NONE,
    PY68_VALUE_BOOL,
    PY68_VALUE_INT,
    PY68_VALUE_OBJECT
} Py68ValueType;

typedef struct Py68Object Py68Object;

typedef struct Py68Value {
    Py68U16 type;
    Py68U16 reserved;
    union {
        Py68I32 integer;
        Py68Object *object;
    } as;
} Py68Value;

typedef enum Py68ObjectType {
    PY68_OBJECT_STRING,
    PY68_OBJECT_LIST,
    PY68_OBJECT_CODE,
    PY68_OBJECT_FUNCTION,
    PY68_OBJECT_NATIVE_FUNCTION,
    PY68_OBJECT_FILE
} Py68ObjectType;

struct Py68Object {
    Py68U16 type;
    Py68U16 flags;
    Py68U32 reference_count;
    Py68Object *next_object;    /* debug/live-object tracking */
};
```

Use explicit retain/release operations. Document every returned or stored pointer as owned or borrowed.

### 6.7 Strings

```c
typedef struct Py68String {
    Py68Object base;
    Py68U32 length;
    Py68U32 hash;
    char data[1];
} Py68String;
```

Version 0.1 strings use ASCII or ISO-8859-1. Pick exactly one policy in the language reference. Embedded NUL is initially rejected. Runtime functions must use stored length, not `strlen`.

### 6.8 Lists

```c
typedef struct Py68List {
    Py68Object base;
    Py68U32 count;
    Py68U32 capacity;
    Py68Value *items;
} Py68List;
```

Before storing a list value, reject direct or indirect containment of the target list to prevent reference cycles in version 0.1.

### 6.9 Names and constants

```c
typedef struct Py68NameTable {
    Py68String **items;         /* owned references */
    Py68U16 count;
    Py68U16 capacity;
} Py68NameTable;

typedef struct Py68ConstantPool {
    Py68Value *items;           /* owned values */
    Py68U16 count;
    Py68U16 capacity;
} Py68ConstantPool;
```

### 6.10 Code object and line mapping

```c
typedef struct Py68LineEntry {
    Py68U16 bytecode_offset;
    Py68U16 source_line;
} Py68LineEntry;

typedef struct Py68CodeObject {
    Py68Object base;
    Py68String *name;           /* owned */
    Py68String *filename;       /* owned */
    Py68U16 argument_count;
    Py68U16 local_count;
    Py68U16 maximum_stack;
    Py68U16 flags;
    Py68U8 *bytecode;
    Py68U32 bytecode_length;
    Py68ConstantPool constants;
    Py68NameTable names;
    Py68NameTable locals;
    Py68LineEntry *lines;
    Py68U16 line_count;
} Py68CodeObject;
```

### 6.11 Functions and native functions

```c
typedef struct Py68Function {
    Py68Object base;
    Py68CodeObject *code;       /* owned */
} Py68Function;

typedef Py68Status (*Py68NativeCallback)(
    Py68Runtime *runtime,
    Py68U16 argument_count,
    Py68Value *arguments,
    Py68Value *result);

typedef struct Py68NativeFunction {
    Py68Object base;
    const char *name;           /* static */
    Py68U16 minimum_arguments;
    Py68U16 maximum_arguments;
    Py68NativeCallback callback;
} Py68NativeFunction;
```

### 6.12 Frame, stack, globals, and runtime

```c
typedef struct Py68Frame {
    Py68CodeObject *code;       /* borrowed while frame active */
    const Py68U8 *ip;
    Py68U16 stack_base;
    Py68U16 local_base;
} Py68Frame;

typedef struct Py68GlobalEntry {
    Py68String *name;           /* owned */
    Py68Value value;            /* owned */
    Py68U8 occupied;
} Py68GlobalEntry;

typedef struct Py68GlobalTable {
    Py68GlobalEntry *entries;
    Py68U16 count;
    Py68U16 capacity;
} Py68GlobalTable;

typedef struct Py68Runtime {
    Py68Allocator allocator;
    Py68Error error;
    Py68Value *value_stack;
    Py68U16 value_stack_count;
    Py68U16 value_stack_capacity;
    Py68Frame *frames;
    Py68U16 frame_count;
    Py68U16 frame_capacity;
    Py68GlobalTable globals;
    Py68Object *live_objects;
    Py68I32 requested_exit_code;
    Py68U16 trace_enabled;
} Py68Runtime;
```

---

## 7. Formal grammar

The grammar below defines the intended level 0.1 syntax. The parser may reject optional loop `else` until the corresponding implementation milestone, but it must not silently misparse it.

```ebnf
file_input       ::= { NEWLINE | statement } EOF ;

statement        ::= compound_statement
                   | simple_statement NEWLINE ;

simple_statement ::= assignment
                   | augmented_assignment
                   | return_statement
                   | break_statement
                   | continue_statement
                   | pass_statement
                   | expression_statement ;

assignment       ::= NAME "=" expression ;

augmented_assignment
                  ::= NAME augmented_operator expression ;

augmented_operator
                  ::= "+=" | "-=" | "*=" | "//=" | "%=" ;

return_statement ::= "return" [ expression ] ;
break_statement  ::= "break" ;
continue_statement
                  ::= "continue" ;
pass_statement   ::= "pass" ;
expression_statement
                  ::= expression ;

compound_statement
                  ::= if_statement
                   | while_statement
                   | for_statement
                   | function_definition ;

if_statement     ::= "if" expression ":" suite
                     { "elif" expression ":" suite }
                     [ "else" ":" suite ] ;

while_statement  ::= "while" expression ":" suite
                     [ "else" ":" suite ] ;

for_statement    ::= "for" NAME "in" expression ":" suite
                     [ "else" ":" suite ] ;

function_definition
                  ::= "def" NAME "(" [ parameter_list ] ")" ":" suite ;

parameter_list   ::= NAME { "," NAME } [ "," ] ;

suite            ::= simple_statement NEWLINE
                   | NEWLINE INDENT statement { statement } DEDENT ;

expression       ::= or_expression ;

or_expression    ::= and_expression { "or" and_expression } ;

and_expression   ::= not_expression { "and" not_expression } ;

not_expression   ::= "not" not_expression
                   | comparison ;

comparison       ::= additive_expression
                     [ comparison_operator additive_expression ] ;

comparison_operator
                  ::= "==" | "!=" | "<" | "<=" | ">" | ">=" ;

additive_expression
                  ::= multiplicative_expression
                      { ( "+" | "-" ) multiplicative_expression } ;

multiplicative_expression
                  ::= unary_expression
                      { ( "*" | "//" | "%" ) unary_expression } ;

unary_expression ::= "+" unary_expression
                   | "-" unary_expression
                   | postfix_expression ;

postfix_expression
                  ::= primary_expression { call_suffix | index_suffix } ;

call_suffix      ::= "(" [ argument_list ] ")" ;
argument_list    ::= expression { "," expression } [ "," ] ;

index_suffix     ::= "[" expression "]"
                   | "[" [ expression ] ":" [ expression ] "]" ;

primary_expression
                  ::= INTEGER
                   | STRING
                   | "True"
                   | "False"
                   | "None"
                   | NAME
                   | list_display
                   | "(" expression ")" ;

list_display     ::= "[" [ argument_list ] "]" ;
```

### Lexical grammar

```ebnf
NAME             ::= identifier_start { identifier_continue } ;
identifier_start ::= "A".."Z" | "a".."z" | "_" ;
identifier_continue
                  ::= identifier_start | "0".."9" ;

INTEGER          ::= decimal_integer
                   | hexadecimal_integer
                   | binary_integer
                   | octal_integer ;

decimal_integer ::= "0" | nonzero_digit { digit | "_" } ;
hexadecimal_integer
                  ::= "0" ( "x" | "X" ) hex_digit { hex_digit | "_" } ;
binary_integer   ::= "0" ( "b" | "B" ) binary_digit
                     { binary_digit | "_" } ;
octal_integer    ::= "0" ( "o" | "O" ) octal_digit
                     { octal_digit | "_" } ;

STRING           ::= single_quoted_string | double_quoted_string ;
COMMENT          ::= "#" { any_character_except_newline } ;
```

Initially implement these string escapes:

```text
\\  backslash
\'  apostrophe
\"  quote
\n  line feed
\r  carriage return
\t  tab
\xHH one 8-bit hexadecimal value
```

Reject unknown escapes rather than guessing.

### Indentation algorithm

Maintain an indentation stack initialized with zero.

At the start of a logical nonblank line outside parentheses or brackets:

1. Count leading spaces.
2. Reject a tab in leading indentation.
3. If width equals the stack top, emit no indentation token.
4. If greater, push width and emit one `INDENT`.
5. If smaller, pop and emit `DEDENT` until equal.
6. If no stack entry equals the width, report inconsistent dedentation.
7. At EOF, emit remaining `DEDENT` tokens and then `EOF`.

Blank lines and comment-only lines do not produce indentation changes. Newlines inside parentheses or brackets are non-significant. Track grouping depth and report an unmatched delimiter.

---

## 8. Token kinds

```c
typedef enum Py68TokenKind {
    PY68_TOKEN_EOF = 0,
    PY68_TOKEN_NEWLINE,
    PY68_TOKEN_INDENT,
    PY68_TOKEN_DEDENT,
    PY68_TOKEN_NAME,
    PY68_TOKEN_INTEGER,
    PY68_TOKEN_STRING,

    PY68_TOKEN_LEFT_PAREN,
    PY68_TOKEN_RIGHT_PAREN,
    PY68_TOKEN_LEFT_BRACKET,
    PY68_TOKEN_RIGHT_BRACKET,
    PY68_TOKEN_COLON,
    PY68_TOKEN_COMMA,

    PY68_TOKEN_PLUS,
    PY68_TOKEN_MINUS,
    PY68_TOKEN_STAR,
    PY68_TOKEN_FLOOR_DIVIDE,
    PY68_TOKEN_PERCENT,

    PY68_TOKEN_ASSIGN,
    PY68_TOKEN_PLUS_ASSIGN,
    PY68_TOKEN_MINUS_ASSIGN,
    PY68_TOKEN_STAR_ASSIGN,
    PY68_TOKEN_FLOOR_DIVIDE_ASSIGN,
    PY68_TOKEN_PERCENT_ASSIGN,

    PY68_TOKEN_EQUAL,
    PY68_TOKEN_NOT_EQUAL,
    PY68_TOKEN_LESS,
    PY68_TOKEN_LESS_EQUAL,
    PY68_TOKEN_GREATER,
    PY68_TOKEN_GREATER_EQUAL,

    PY68_TOKEN_IF,
    PY68_TOKEN_ELIF,
    PY68_TOKEN_ELSE,
    PY68_TOKEN_WHILE,
    PY68_TOKEN_FOR,
    PY68_TOKEN_IN,
    PY68_TOKEN_DEF,
    PY68_TOKEN_RETURN,
    PY68_TOKEN_BREAK,
    PY68_TOKEN_CONTINUE,
    PY68_TOKEN_PASS,
    PY68_TOKEN_AND,
    PY68_TOKEN_OR,
    PY68_TOKEN_NOT,
    PY68_TOKEN_TRUE,
    PY68_TOKEN_FALSE,
    PY68_TOKEN_NONE,

    PY68_TOKEN_UNSUPPORTED_KEYWORD
} Py68TokenKind;
```

---

## 9. AST node kinds

```c
typedef enum Py68AstKind {
    PY68_AST_MODULE,
    PY68_AST_ASSIGN,
    PY68_AST_AUGMENTED_ASSIGN,
    PY68_AST_IF,
    PY68_AST_WHILE,
    PY68_AST_FOR,
    PY68_AST_FUNCTION_DEF,
    PY68_AST_RETURN,
    PY68_AST_BREAK,
    PY68_AST_CONTINUE,
    PY68_AST_PASS,
    PY68_AST_EXPRESSION_STATEMENT,
    PY68_AST_INTEGER,
    PY68_AST_STRING,
    PY68_AST_BOOL,
    PY68_AST_NONE,
    PY68_AST_NAME,
    PY68_AST_UNARY,
    PY68_AST_BINARY,
    PY68_AST_CALL,
    PY68_AST_INDEX,
    PY68_AST_SLICE,
    PY68_AST_LIST
} Py68AstKind;
```

---

## 10. Opcode table

All multibyte operands are stored big-endian. `u8`, `u16`, `s16`, and `u32` denote encoded operand types. Stack effects describe successful execution and do not include exceptional paths.

| Value | Opcode | Operands | Stack before -> after | Meaning |
|---:|---|---|---|---|
| `0x00` | `OP_HALT` | none | `... -> ...` | End module execution |
| `0x01` | `OP_POP` | none | `..., value -> ...` | Discard top value |
| `0x02` | `OP_DUP` | none | `..., value -> ..., value, value` | Duplicate top value |
| `0x03` | `OP_LOAD_NONE` | none | `... -> ..., None` | Push None |
| `0x04` | `OP_LOAD_TRUE` | none | `... -> ..., True` | Push true |
| `0x05` | `OP_LOAD_FALSE` | none | `... -> ..., False` | Push false |
| `0x06` | `OP_LOAD_CONST` | `u16 index` | `... -> ..., constant` | Push constant pool item |
| `0x07` | `OP_LOAD_GLOBAL` | `u16 name` | `... -> ..., value` | Resolve global or builtin |
| `0x08` | `OP_STORE_GLOBAL` | `u16 name` | `..., value -> ...` | Store global, consuming value |
| `0x09` | `OP_LOAD_LOCAL` | `u16 slot` | `... -> ..., value` | Push local slot |
| `0x0A` | `OP_STORE_LOCAL` | `u16 slot` | `..., value -> ...` | Store local, consuming value |
| `0x0B` | `OP_NEGATE` | none | `..., value -> ..., result` | Checked integer negation |
| `0x0C` | `OP_POSITIVE` | none | `..., value -> ..., result` | Validate integer and retain value |
| `0x0D` | `OP_NOT` | none | `..., value -> ..., bool` | Push logical negation |
| `0x10` | `OP_ADD` | none | `..., a, b -> ..., result` | Integer add, string concat, or list concat if supported |
| `0x11` | `OP_SUBTRACT` | none | `..., a, b -> ..., result` | Checked subtraction |
| `0x12` | `OP_MULTIPLY` | none | `..., a, b -> ..., result` | Checked multiplication |
| `0x13` | `OP_FLOOR_DIVIDE` | none | `..., a, b -> ..., result` | Python floor division |
| `0x14` | `OP_MODULO` | none | `..., a, b -> ..., result` | Python modulo |
| `0x18` | `OP_EQUAL` | none | `..., a, b -> ..., bool` | Equality |
| `0x19` | `OP_NOT_EQUAL` | none | `..., a, b -> ..., bool` | Inequality |
| `0x1A` | `OP_LESS` | none | `..., a, b -> ..., bool` | Less than |
| `0x1B` | `OP_LESS_EQUAL` | none | `..., a, b -> ..., bool` | Less than or equal |
| `0x1C` | `OP_GREATER` | none | `..., a, b -> ..., bool` | Greater than |
| `0x1D` | `OP_GREATER_EQUAL` | none | `..., a, b -> ..., bool` | Greater than or equal |
| `0x20` | `OP_JUMP` | `s16 delta` | `... -> ...` | Relative unconditional branch |
| `0x21` | `OP_JUMP_IF_FALSE` | `s16 delta` | `..., value -> ...` | Consume value and branch if false |
| `0x22` | `OP_JUMP_IF_TRUE` | `s16 delta` | `..., value -> ...` | Consume value and branch if true |
| `0x23` | `OP_JUMP_IF_FALSE_OR_POP` | `s16 delta` | conditional | Implement short-circuit `and` |
| `0x24` | `OP_JUMP_IF_TRUE_OR_POP` | `s16 delta` | conditional | Implement short-circuit `or` |
| `0x28` | `OP_BUILD_LIST` | `u16 count` | `..., n values -> ..., list` | Build list from top values |
| `0x29` | `OP_LOAD_INDEX` | none | `..., container, index -> ..., value` | Read index |
| `0x2A` | `OP_STORE_INDEX` | none | `..., container, index, value -> ...` | Store index |
| `0x2B` | `OP_LOAD_SLICE` | none | `..., container, start, end -> ..., value` | Read slice; None means omitted bound |
| `0x30` | `OP_RANGE_INIT` | `u8 argc` | `..., args -> ..., range_state` | Validate and initialize range |
| `0x31` | `OP_RANGE_NEXT` | `s16 end_delta` | conditional | Push next range value or branch at end |
| `0x38` | `OP_MAKE_FUNCTION` | `u16 const_index` | `... -> ..., function` | Wrap code object constant |
| `0x39` | `OP_CALL` | `u8 argc` | `..., callee, args -> ..., result` | Invoke user or native function |
| `0x3A` | `OP_RETURN_VALUE` | none | `..., value -> caller ..., value` | Return explicit value |
| `0x3B` | `OP_RETURN_NONE` | none | `... -> caller ..., None` | Return None |
| `0x40` | `OP_PRINT_DEBUG` | none | `..., value -> ...` | Optional debug-only opcode, disabled in release format |
```

Reserved values must be rejected by the verifier. Do not renumber published opcodes without incrementing the bytecode format version.

Opcode enum:

```c
typedef enum Py68Opcode {
    OP_HALT = 0x00,
    OP_POP = 0x01,
    OP_DUP = 0x02,
    OP_LOAD_NONE = 0x03,
    OP_LOAD_TRUE = 0x04,
    OP_LOAD_FALSE = 0x05,
    OP_LOAD_CONST = 0x06,
    OP_LOAD_GLOBAL = 0x07,
    OP_STORE_GLOBAL = 0x08,
    OP_LOAD_LOCAL = 0x09,
    OP_STORE_LOCAL = 0x0A,
    OP_NEGATE = 0x0B,
    OP_POSITIVE = 0x0C,
    OP_NOT = 0x0D,
    OP_ADD = 0x10,
    OP_SUBTRACT = 0x11,
    OP_MULTIPLY = 0x12,
    OP_FLOOR_DIVIDE = 0x13,
    OP_MODULO = 0x14,
    OP_EQUAL = 0x18,
    OP_NOT_EQUAL = 0x19,
    OP_LESS = 0x1A,
    OP_LESS_EQUAL = 0x1B,
    OP_GREATER = 0x1C,
    OP_GREATER_EQUAL = 0x1D,
    OP_JUMP = 0x20,
    OP_JUMP_IF_FALSE = 0x21,
    OP_JUMP_IF_TRUE = 0x22,
    OP_JUMP_IF_FALSE_OR_POP = 0x23,
    OP_JUMP_IF_TRUE_OR_POP = 0x24,
    OP_BUILD_LIST = 0x28,
    OP_LOAD_INDEX = 0x29,
    OP_STORE_INDEX = 0x2A,
    OP_LOAD_SLICE = 0x2B,
    OP_RANGE_INIT = 0x30,
    OP_RANGE_NEXT = 0x31,
    OP_MAKE_FUNCTION = 0x38,
    OP_CALL = 0x39,
    OP_RETURN_VALUE = 0x3A,
    OP_RETURN_NONE = 0x3B,
    OP_PRINT_DEBUG = 0x40
} Py68Opcode;
```

The agent must maintain one machine-readable opcode definition in `tools/generate_opcodes.py` and generate or validate the C name, operand-width, and stack-effect tables from it.

---

## 11. Bytecode encoding and verifier

### Operand encoding

```c
static Py68U16 py68_read_u16_be(const Py68U8 **cursor)
{
    const Py68U8 *p = *cursor;
    Py68U16 value = (Py68U16)(((Py68U16)p[0] << 8) |
                               (Py68U16)p[1]);
    *cursor = p + 2;
    return value;
}

static Py68I16 py68_read_i16_be(const Py68U8 **cursor)
{
    return (Py68I16)py68_read_u16_be(cursor);
}

static Py68U32 py68_read_u32_be(const Py68U8 **cursor)
{
    const Py68U8 *p = *cursor;
    Py68U32 value = ((Py68U32)p[0] << 24) |
                    ((Py68U32)p[1] << 16) |
                    ((Py68U32)p[2] << 8) |
                     (Py68U32)p[3];
    *cursor = p + 4;
    return value;
}
```

The verifier must reject:

- unknown or debug-only opcodes in release bytecode
- truncated instructions
- invalid constant, name, and local indexes
- branch destinations outside the code
- branches into operand bytes
- stack underflow
- mismatched stack depth at control-flow joins
- value stack depth beyond configured maximum
- malformed call operands
- invalid `RANGE_NEXT` destinations
- reachable fall-through beyond the code array
- module code without `HALT`
- function code without a return instruction on each reachable terminal path

Verification algorithm:

1. Linear-decode the instruction boundaries.
2. Record valid instruction offsets in a bitset or byte array.
3. Validate all indexes and branch destinations.
4. Perform a worklist control-flow traversal.
5. Propagate stack depth to successors.
6. Fail when a successor has an inconsistent prior stack depth.
7. Record calculated maximum stack.
8. Compare it with or assign it to the code object metadata.

---

## 12. Compiler rules

### Short-circuit `and`

```text
compile left
JUMP_IF_FALSE_OR_POP end
compile right
end:
```

### Short-circuit `or`

```text
compile left
JUMP_IF_TRUE_OR_POP end
compile right
end:
```

### Assignment

At module scope:

```text
compile value
STORE_GLOBAL name_index
```

Inside a function:

```text
compile value
STORE_LOCAL slot
```

### `if`

```text
compile condition
JUMP_IF_FALSE next_clause
compile body
JUMP end
next_clause:
compile elif/else
end:
```

### `while`

```text
loop_start:
compile condition
JUMP_IF_FALSE loop_end
compile body
JUMP loop_start
loop_end:
```

Maintain loop context with patch lists for `break` and `continue`. `continue` targets condition evaluation. `break` targets after the optional loop `else`; normal exhaustion targets the `else` body.

### Functions

1. Perform function-local symbol collection before compiling the body.
2. Assign each parameter and local name a 16-bit slot.
3. Compile the function body into a nested code object.
4. Add an implicit `RETURN_NONE` when control can reach the end.
5. Add the code object to the enclosing constant pool.
6. Emit `MAKE_FUNCTION` and store the resulting function.

Nested function definitions may be rejected in level 0.1 to avoid closures.

---

## 13. Integer semantics

Values are signed 32-bit integers. Detect overflow for parsing, unary negation, addition, subtraction, and multiplication.

Required behavior:

```python
-7 // 3 == -3
-7 % 3 == 2
7 // -3 == -3
7 % -3 == -2
```

C division cannot be used without correction when operands have opposite signs and the division is inexact.

Required APIs:

```c
int py68_checked_negate(Py68I32 value, Py68I32 *result);
int py68_checked_add(Py68I32 left, Py68I32 right, Py68I32 *result);
int py68_checked_subtract(Py68I32 left, Py68I32 right, Py68I32 *result);
int py68_checked_multiply(Py68I32 left, Py68I32 right, Py68I32 *result);
int py68_floor_divmod(Py68I32 left, Py68I32 right,
                      Py68I32 *quotient, Py68I32 *remainder);
```

Division by zero raises `ZeroDivisionError`. Dividing the minimum 32-bit integer by `-1` raises `OverflowError`.

---

## 14. Truthiness and comparisons

False values:

```text
None
False
integer zero
empty string
empty list
```

All other supported values are true.

Equality:

- integers and booleans follow the documented Python68K numeric policy
- strings compare by length and bytes
- lists compare element by element if implemented, otherwise reject list comparison deliberately
- functions compare by identity
- order comparison is valid initially only for integers and strings

Do not compare unrelated raw pointers to define language ordering.

---

## 15. Strings and lists

Strings:

- immutable
- length-prefixed
- optional cached hash
- bounds-checked indexing
- negative indexes normalized as `length + index`
- slicing clamps bounds to the valid range
- concatenation checks total allocation size

Lists:

- mutable dynamic arrays
- capacity growth checks overflow and memory limit
- list stores owned references
- replacing an item retains the new value before releasing the old one
- negative indexes are normalized
- out-of-range access raises `IndexError`
- direct and indirect cycles are rejected in version 0.1

---

## 16. VM and call behavior

Use a switch dispatch loop. Do not optimize dispatch until all semantics and tests pass.

```c
Py68Status py68_vm_execute(Py68Runtime *runtime, Py68CodeObject *module)
{
    /* Push module frame, then dispatch until HALT or error. */
}
```

Invocation stack layout must be documented. One workable design:

```text
... callee arg0 arg1 ... argN-1
```

`OP_CALL argc`:

1. Verify the stack contains callee and arguments.
2. For native functions, invoke the validated callback and replace call operands with the result.
3. For user functions, verify exact argument count.
4. Allocate or reserve local slots.
5. Move or retain arguments into parameter local slots.
6. Push a frame containing caller continuation and bases.
7. Begin executing the callee code.
8. On return, release remaining locals and temporaries, restore caller state, and push result.

Use explicit call and value stacks rather than C recursion. A script recursion limit failure must not overflow the native machine stack.

Tracebacks must resemble:

```text
ZeroDivisionError: division by zero
  at divide (math.py:8)
  at calculate (math.py:14)
  at <module> (math.py:20)
```

---

## 17. Built-ins

MVP:

```python
print(*values)
len(value)
range(stop)
range(start, stop)
range(start, stop, step)
int(value)
str(value)
bool(value)
abs(value)
min(a, b)
max(a, b)
list_append(list_value, item)
list_pop(list_value)
exit(code)
```

After the core runtime:

```python
fopen(path, mode)
fclose(handle)
fread(handle, count)
freadline(handle)
fwrite(handle, string)
exists(path)
remove(path)
rename(old, new)
getenv(name)
```

Native callbacks validate argument count and types before accessing values. No script-visible raw pointers.

---

## 18. Platform interface

```c
typedef struct Py68PlatformFile Py68PlatformFile;

Py68Status py68_platform_initialize(Py68Runtime *runtime);
void py68_platform_shutdown(Py68Runtime *runtime);

void *py68_platform_alloc(Py68U32 size);
void py68_platform_free(void *memory, Py68U32 size);

Py68Status py68_platform_write_stdout(
    Py68Runtime *, const char *, Py68U32 length);
Py68Status py68_platform_write_stderr(
    Py68Runtime *, const char *, Py68U32 length);
Py68Status py68_platform_read_file(
    Py68Runtime *, const char *path, Py68U8 **data, Py68U32 *length);
Py68I32 py68_platform_clock_ticks(void);
```

No tokenizer, parser, compiler, object, or VM file may include an AmigaOS header.

---

## 19. CLI contract

```text
python script.py [arguments...]
python -c "source"
python -V
python --help
python --check script.py
python --disassemble script.py
python --memory-stats script.py
```

Predefine `argv` as a list of strings. Do not implement a full `sys` module initially.

Suggested process exit mapping:

```text
0  success
5  explicit controlled nonzero exit where appropriate
10 source, syntax, or script runtime error
20 interpreter internal failure
```

Document the final mapping and make every path deterministic.

---

## 20. vbcc and host builds

Release builds must explicitly select the target:

```text
-cpu=68000
-fpu=0
```

Conceptual Makefile fragment:

```make
VBCC ?= /opt/vbcc
VC := $(VBCC)/bin/vc

COMMON_DEFINES := -DPY68K_LANGUAGE_LEVEL=1
AMIGA_TARGET := +aos68k
AMIGA_CPU := -cpu=68000 -fpu=0
AMIGA_DEBUG := -g -use-framepointer -no-delayed-popping
AMIGA_RELEASE := -O=2

amiga-debug:
	$(VC) $(AMIGA_TARGET) $(AMIGA_CPU) $(AMIGA_DEBUG) \
        $(COMMON_DEFINES) $(AMIGA_SOURCES) -o pythonami-debug

amiga-release:
	$(VC) $(AMIGA_TARGET) $(AMIGA_CPU) $(AMIGA_RELEASE) \
        $(COMMON_DEFINES) $(AMIGA_SOURCES) -o pythonami
```

The agent must inspect the installed vbcc configuration and adjust the target selector to the real local configuration. Keep options in checked-in response files if command length becomes problematic.

Host debug build should use strict warnings. When available, run AddressSanitizer and UndefinedBehaviorSanitizer on host tests. Sanitizers are host tools and are not Amiga runtime dependencies.

---

## 21. Testing requirements

### Unit tests

- integer type widths
- big-endian decoding from even and odd byte offsets
- allocation tracking
- injected allocation failure
- tokenization and indentation
- string escapes
- integer literal boundaries
- expression precedence
- every AST statement
- symbol classification
- each opcode emission
- branch patching
- verifier control-flow joins
- checked arithmetic
- truthiness
- strings and lists
- reference retain/release
- global table
- calls and returns
- traceback line mapping

### Differential tests

For accepted subset programs:

1. Execute with desktop Python.
2. Execute with host Python68K.
3. Compare stdout.
4. Execute Amiga Python68K in the selected emulator.
5. Compare stdout and exit class.

Document expected differences caused by 32-bit integer limits and 8-bit strings.

### Negative tests

- tabs in indentation
- inconsistent dedentation
- unmatched delimiters
- invalid escape
- malformed numeric literal
- unsupported keyword
- missing colon
- empty required suite
- return outside a function
- break or continue outside a loop
- duplicate parameter
- nested function if unsupported
- undefined name
- wrong argument count
- invalid operand types
- integer overflow
- division by zero
- index error
- recursion limit
- value-stack limit
- malformed bytecode
- allocation failure at every allocation point

### Memory-failure sweep

Host tests must support failing allocation number N. Re-run representative programs with N advanced through every allocation point. Every run must return a controlled error, avoid invalid access, and clean up all earlier allocations.

---

## 22. Normative reference-count ownership rules

This section is normative. If another section is ambiguous about ownership, this section takes precedence.

### 22.1 Ownership vocabulary

- **Owned value/reference**: the holder is responsible for eventually releasing it or transferring ownership.
- **Borrowed value/reference**: usable only while the documented owner remains alive; it must not be released by the borrower.
- **Moved value/reference**: ownership is transferred from source to destination; the source must be invalidated or treated as no longer owned.
- **Immortal/static value**: no retain or release is required. This applies to `None`, booleans, integer immediates, static native-function names, and other explicitly documented static data.

`Py68Value` is passed by value as a C structure. Copying a `Py68Value` containing an object pointer does **not** implicitly retain the object. A copied object value becomes owned only after `py68_value_retain` succeeds.

Required API:

```c
Py68Value py68_value_none(void);
Py68Value py68_value_bool(int truth);
Py68Value py68_value_int(Py68I32 value);
Py68Value py68_value_from_object(Py68Object *object); /* borrows object */

void py68_object_retain(Py68Object *object);
void py68_object_release(Py68Runtime *runtime, Py68Object *object);
void py68_value_retain(Py68Value value);
void py68_value_release(Py68Runtime *runtime, Py68Value value);
```

`py68_object_retain(NULL)` and release of `NULL` must be harmless. Retain-counter overflow is an internal/runtime error and must not wrap. Newly allocated heap objects begin with reference count 1 and are returned as owned references.

### 22.2 Ownership matrix

| Operation | Input ownership | Destination ownership | Required action |
|---|---|---|---|
| Construct scalar `None`, bool, or int | none | owned-by-value scalar | no retain/release |
| Allocate heap object | none | caller owns reference count 1 | caller releases or transfers |
| Push a borrowed value onto VM stack | borrowed | stack owns one reference | retain before push |
| Push an owned value onto VM stack by move | owned | stack owns existing reference | do not retain; invalidate source |
| Peek VM stack | stack-owned | caller borrows | no retain/release |
| Pop VM stack by move | stack-owned | caller receives ownership | decrement stack count; do not release |
| Discard VM stack top | stack-owned | none | pop and release |
| Load constant/name/local/global | container-owned | stack owns new reference | retain before push |
| Store global/local by move | stack-owned | table/slot owns reference | release old destination, move new value |
| Store list element by move | caller-owned | list owns reference | retain new first if alias is possible; release old; transfer new |
| Read list element | list-owned | stack owns new reference | retain before push |
| Native function arguments | VM stack-owned | callback borrows | callback must not release arguments |
| Native function result | callback creates owned result | VM takes ownership | callback returns one owned value |
| User-function parameters | caller stack owns arguments | callee locals own moved/retained values | follow call algorithm below |
| Function return value | callee stack owns | caller stack owns | move value before unwinding frame |
| Constant pool insertion | caller-owned or borrowed per API | pool owns one reference | retain or move according to named API |
| Global-table lookup | table-owned | caller borrows | retain only if storing beyond lookup |
| Runtime shutdown | runtime-owned containers | none | release all values and objects exactly once |

Provide distinct APIs when both copy and move behavior are useful:

```c
Py68Status py68_stack_push_copy(Py68Runtime *, Py68Value borrowed);
Py68Status py68_stack_push_move(Py68Runtime *, Py68Value *owned);
Py68Status py68_stack_pop_move(Py68Runtime *, Py68Value *owned_result);
void py68_stack_pop_discard(Py68Runtime *);

Py68Status py68_list_set_copy(
    Py68Runtime *, Py68List *, Py68I32 index, Py68Value borrowed);
Py68Status py68_list_set_move(
    Py68Runtime *, Py68List *, Py68I32 index, Py68Value *owned);
```

Move APIs set the source to `None` after successful transfer. On failure, ownership remains with the caller unless the function contract explicitly says otherwise.

### 22.3 Aliasing rule

When replacing an owned value in a container, retain or otherwise secure the incoming value **before** releasing the outgoing value. This is required because both values may point to the same object.

```c
/* Correct copy replacement order. */
py68_value_retain(new_value);
old_value = slot->value;
slot->value = new_value;
py68_value_release(runtime, old_value);
```

### 22.4 Function-call ownership

For `OP_CALL argc`, the VM owns the callee and argument values on the operand stack.

- A native callback borrows the arguments and returns one owned result.
- A user call moves or retains arguments into freshly initialized local slots.
- After a user frame is established, remove the callee and argument cells from the caller-visible stack without releasing values transferred to locals.
- On normal return, move the return value to a temporary owned variable, release all remaining callee operand values and locals, pop the frame, then move the return value onto the caller stack.
- On call setup failure, release no value twice. Either leave the original call segment unchanged or roll it back through one documented cleanup path.

### 22.5 Container destruction

Object destructors release owned children before freeing storage:

- string: free one allocation; it owns no child values
- list: release all `count` elements, free item array, free list object
- code object: release name, filename, constants, name/local strings and nested code constants; free bytecode and line table
- function: release its code object
- native function: release only dynamically owned fields; static name is not released
- file: close the platform handle if still open, then free wrapper

During runtime teardown, destructors must be safe when initialization was only partial. Counts and pointers must describe only successfully initialized elements.

### 22.6 Cycle policy

Level 0.1 has no tracing collector. Before storing a list inside a list, perform an identity-based depth-first containment check with an explicit work stack and visited set or bounded traversal. Reject an operation that would make the destination reachable from the inserted list. Report `ValueError: cyclic lists are not supported`. Do not use C recursion for this traversal.

---

## 23. Normative identifiers, interning, globals, and built-ins

### 23.1 Identifier interning

Every identifier used by executable code must have one canonical `Py68String` within the owning compilation/runtime context.

- Token objects continue to refer to source spans and allocate no identifier strings.
- During symbol analysis or code generation, convert a source span to an interned string.
- A code object's `names` and `locals` tables contain unique string references.
- Bytecode addresses names and locals using unsigned 16-bit indexes.
- Fail compilation if more than 65,535 distinct names, locals, or constants are required by one code object.
- Interning equality is byte length plus exact byte equality. Pointer identity may be used only after both values are known to be interned by the same runtime.

Use deterministic 32-bit FNV-1a hashing over unsigned bytes:

```c
#define PY68_FNV_OFFSET 2166136261UL
#define PY68_FNV_PRIME  16777619UL

Py68U32 py68_hash_bytes(const char *data, Py68U32 length)
{
    Py68U32 hash = PY68_FNV_OFFSET;
    Py68U32 index;
    for (index = 0; index < length; ++index) {
        hash ^= (Py68U8)data[index];
        hash *= PY68_FNV_PRIME;
    }
    return hash;
}
```

Hash overflow is intentional unsigned 32-bit wraparound. Do not randomize hashes in language level 0.1 because deterministic builds and tests are preferred.

### 23.2 Intern table

Use open addressing with linear probing.

```c
typedef struct Py68InternEntry {
    Py68String *string;         /* owned; NULL means empty */
    Py68U32 hash;
} Py68InternEntry;

typedef struct Py68InternTable {
    Py68InternEntry *entries;
    Py68U16 count;
    Py68U16 capacity;
} Py68InternTable;
```

Rules:

- capacity is zero or a power of two
- initial nonzero capacity is 16
- grow before insertion would exceed 70 percent occupancy
- growth doubles capacity and rehashes entries
- level 0.1 does not remove interned identifiers during runtime, so tombstones are unnecessary in the intern table
- the table owns one reference to each string
- compilation may use a temporary intern table, but strings stored in code objects must remain owned by those code objects after the temporary table is destroyed

### 23.3 Global table

Use open addressing with linear probing and no deletion in level 0.1.

```c
typedef struct Py68GlobalEntry {
    Py68String *name;           /* owned; NULL means empty */
    Py68Value value;            /* owned when occupied */
    Py68U32 hash;
} Py68GlobalEntry;

typedef struct Py68GlobalTable {
    Py68GlobalEntry *entries;
    Py68U16 count;
    Py68U16 capacity;
} Py68GlobalTable;
```

Rules:

- capacity and growth policy match the intern table
- globals are looked up by stored hash, length, and bytes
- assigning an existing global replaces its value using the alias-safe ownership rule
- assigning a new global stores an owned interned name and owned value
- no user-visible global deletion exists in level 0.1
- global iteration order is unspecified

### 23.4 Built-in table and shadowing

Built-ins are stored in a separate immutable table populated during runtime initialization. Lookup order is:

```text
function local slot
-> module global table
-> built-in table
-> NameError
```

At module level, globals may shadow built-ins:

```python
print = 7
```

After this assignment, resolving `print` in that module returns integer 7. There is no syntax in level 0.1 for deleting the global to reveal the built-in again. Function locals may also shadow globals and built-ins.

Built-in registration failures must abort runtime initialization and release all built-ins registered earlier.

---

## 24. Normative function symbol collection and local-slot allocation

Each code object has one symbol-analysis pass before bytecode generation. Do not decide whether a name is local while emitting individual expressions.

### 24.1 Module symbols

At module scope, assignment targets and function-definition names are globals. Module code uses `LOAD_GLOBAL` and `STORE_GLOBAL`. There are no module local slots.

### 24.2 Function prepass

Before compiling a function body:

1. Validate parameters from left to right.
2. Reject duplicate parameter names.
3. Add parameters to the local-symbol table in source order.
4. Walk the complete function body without entering nested function bodies.
5. Collect every simple-name assignment target, augmented-assignment target, loop target, and function-definition target.
6. Add each first-seen non-parameter local in deterministic source order.
7. Validate `return`, `break`, and `continue` context independently.
8. Freeze local slots before emitting any bytecode.

Level 0.1 rejects nested function definitions. Therefore closure, cell, free-variable, `global`, and `nonlocal` analysis is not required.

### 24.3 Slot assignment

- parameters occupy slots `0 .. argument_count - 1`
- other locals follow in first-assignment source order
- one name has exactly one slot for the whole function
- there is no local slot reuse in level 0.1
- more than 65,535 locals is a compile error
- every local slot is initialized to a distinguished internal `UNBOUND` state at function entry, except parameter slots
- `UNBOUND` is VM-internal and is not a user-visible Python68K value
- `LOAD_LOCAL` of `UNBOUND` raises `NameError` or a documented `UnboundLocalError` subtype

Required example:

```python
x = 10

def f():
    print(x)
    x = 20
```

Because `x` is assigned anywhere in `f`, every reference to `x` in `f` is local. The first statement attempts to load an unbound local and must fail. It must not read module global `x`.

### 24.4 Function-definition binding

At module scope, compile a function definition as a code-object constant, `MAKE_FUNCTION`, then `STORE_GLOBAL` using its declared name. Recursion works because the body resolves the function name as a global when no local assignment makes it local.

If nested functions are enabled in a future language level, this section must be replaced by explicit local, cell, free, and global classifications. Do not add partial closure behavior.

### 24.5 Control-context stacks

During validation and compilation, maintain explicit context:

```c
typedef struct Py68LoopContext {
    Py68U32 continue_target;
    Py68PatchList break_patches;
    Py68PatchList continue_patches;
    struct Py68LoopContext *outer;
} Py68LoopContext;

typedef struct Py68CompileContext {
    Py68CodeObject *code;
    Py68SymbolTable symbols;
    Py68LoopContext *loop;
    Py68U16 inside_function;
} Py68CompileContext;
```

`return` requires `inside_function`. `break` and `continue` require a current loop. Function compilation begins with no inherited loop context.

---

## 25. Normative constant pool and code-constant representation

### 25.1 In-memory constant kinds

The in-memory constant pool stores owned `Py68Value` entries. Only these constant types are valid:

```text
None
Boolean
signed 32-bit integer
string
code object
```

Lists, functions, native functions, file handles, range state, and arbitrary runtime objects are not valid compile-time constants.

A nested code object is stored as `PY68_VALUE_OBJECT` whose object type is `PY68_OBJECT_CODE`. `OP_MAKE_FUNCTION` validates this object type before constructing a function.

### 25.2 Deduplication

Within one code object:

- `None`, `True`, and `False` each have at most one entry if placed in the pool
- equal integers may share one entry
- equal strings by length and bytes share one entry
- code objects are never deduplicated by content
- constant insertion order is deterministic and based on first encounter during source-order compilation
- more than 65,535 constants is a compile error

The dedicated `LOAD_NONE`, `LOAD_TRUE`, and `LOAD_FALSE` opcodes should normally avoid scalar entries for those values.

### 25.3 Constant-pool API

```c
Py68Status py68_constant_add_copy(
    Py68Runtime *, Py68ConstantPool *, Py68Value borrowed,
    Py68U16 *index_out);

Py68Status py68_constant_add_move(
    Py68Runtime *, Py68ConstantPool *, Py68Value *owned,
    Py68U16 *index_out);

Py68Value py68_constant_get_borrowed(
    const Py68ConstantPool *, Py68U16 index);
```

Copy insertion retains when a new object constant is stored. Move insertion transfers ownership and replaces the source with `None` on success. On failure, move insertion leaves ownership with the caller.

### 25.4 Serialized constant tags for Phase 7

When serialized bytecode is implemented, use these stable tags:

| Tag | Meaning | Payload |
|---:|---|---|
| `0x00` | None | none |
| `0x01` | False | none |
| `0x02` | True | none |
| `0x03` | Integer | big-endian signed 32-bit |
| `0x04` | String | `u32 byte_length`, then bytes |
| `0x05` | Code object | `u32 serialized_length`, then nested code record |

All lengths and indexes are big-endian. Deserialize with checked bounds before allocating. Reject unknown tags, lengths extending beyond the enclosing section, nesting beyond the configured code-object depth, duplicate malformed metadata, and trailing bytes where the format forbids them.

### 25.5 Serialized file header

Phase 7 bytecode files begin with:

```c
typedef struct Py68BytecodeHeaderLogical {
    Py68U8 magic[4];            /* 'P', '6', '8', 'K' */
    Py68U16 format_version;     /* initially 1 */
    Py68U16 language_level;     /* initially 1 */
    Py68U32 flags;
    Py68U32 payload_length;
    Py68U32 checksum;
} Py68BytecodeHeaderLogical;
```

This is a logical layout only. Do not read or write the C struct directly because padding and host byte order are not portable. Encode fields byte by byte. The checksum algorithm must be specified before Phase 7 and covered by test vectors.

### 25.6 Branch displacement convention

Every `s16 delta` branch is relative to the instruction pointer immediately after the complete branch instruction, including its operand bytes.

```text
branch_target = offset_after_operand + signed_delta
```

Compilation fails if the displacement is outside `-32768 .. 32767`. Patch calculations use a wider temporary integer and validate before narrowing. The verifier uses exactly the same convention.

A single code object may have more than 32 KB of bytecode only if every branch remains representable and all metadata indexes remain valid. However, level 0.1 may impose a simpler documented maximum code length of 32,767 or 65,535 bytes. The implementation must choose and enforce one limit consistently.

---

## 26. Normative error propagation, rollback, and VM unwinding

### 26.1 Single active error

`Py68Runtime.error` holds at most one active error. The first failure sets it. Cleanup functions must not overwrite it unless no error is active. A secondary cleanup problem may set an internal diagnostic only in debug logging.

Required helpers:

```c
Py68Status py68_error_set(
    Py68Runtime *, Py68ErrorKind, Py68Location,
    const char *filename, const char *message);

int py68_error_is_active(const Py68Runtime *);
void py68_error_clear(Py68Runtime *);
```

Functions that can fail return `Py68Status`. They do not return a valid output value on failure. Output parameters must be initialized to a safe state by the caller or left unchanged on failure according to the function contract.

### 26.2 Status propagation

Use this pattern:

```c
status = py68_operation(runtime, input, &result);
if (status != PY68_STATUS_OK) {
    goto cleanup;
}
```

Do not set an error and then return `PY68_STATUS_OK`. Do not return an error status without an active error, except for the explicit process-exit status. Internal assertions in release builds must become controlled internal errors where malformed source or bytecode could trigger them.

### 26.3 Transactional mutation

Operations that can allocate while mutating a container must either:

- complete successfully and commit the new state, or
- fail while leaving the original state valid and semantically unchanged.

Examples:

- grow a list into a temporary pointer; update `items` and `capacity` only after allocation succeeds
- build a new hash table completely before replacing the old table
- append bytecode only after buffer capacity is secured
- add a constant only after all required retains or copies succeed

### 26.4 VM error path

On an opcode failure:

1. Preserve the first active error and current source location.
2. Record traceback information from active frames before destroying them, or format the traceback while frames still exist.
3. Stop dispatch immediately.
4. Release every owned value above each frame's stack base.
5. Release every initialized local slot in the frame.
6. Pop frames from innermost to outermost.
7. Release remaining module operand values.
8. Leave global values alive until runtime/module teardown so diagnostics and post-run memory checks remain deterministic.
9. Return the corresponding non-success `Py68Status`.

Implement one VM cleanup routine:

```c
void py68_vm_unwind_all(Py68Runtime *runtime);
```

It must be idempotent when stacks are already empty and safe after partially completed call setup.

### 26.5 Native callback failure

A native callback borrows its arguments. On success it returns one owned result. On failure:

- it returns a non-OK status
- it leaves `result` as `None` or another documented safe scalar
- it releases all temporary objects it allocated
- it does not release borrowed arguments
- it does not mutate input containers unless the operation's documented semantics are mutating and the mutation was committed transactionally

The VM retains ownership of the original call segment until callback success. After success, it releases the callee and argument values, then moves the owned result onto the stack.

### 26.6 Explicit `exit`

`exit(code)` is not a runtime error. It sets `requested_exit_code`, returns `PY68_STATUS_EXIT`, and triggers normal frame/stack unwinding without printing an error traceback. The CLI maps the requested value to the documented AmigaDOS-compatible process status policy.

### 26.7 Parser and compiler cleanup

Tokenizer, parser, symbol analysis, and bytecode compilation each have one top-level owner object and one cleanup path. On failure:

- release the source only after no diagnostic needs its content
- destroy the whole AST arena rather than individual nodes
- release all partially built constants, names, bytecode buffers, and nested code objects
- leave no partially published code object in the runtime

### 26.8 Shutdown invariant

After `py68_vm_unwind_all`, module/global teardown, intern-table destruction, built-in destruction, and platform shutdown:

```text
value_stack_count == 0
frame_count == 0
live_objects == NULL
allocator.stats.current_bytes == 0
allocator.stats.allocation_count == allocator.stats.free_count
```

If allocation and free counts can differ because `realloc` is counted separately, document the exact accounting formula. `current_bytes == 0` and no live objects are normative.

---

## 27. Implementation phases

### Phase 0: bootstrap

Deliver repository layout, portable types, status/error handling, tracked allocator, platform stdout/stderr, host build, vbcc build, `-V`, and `--help`.

Acceptance:

```text
python -V
Python68K 0.1.0
```

### Phase 1: tokenizer and expressions

Deliver source loading, tokenizer, indentation tokens, string and integer literals, Pratt expression parser, AST arena, `print`, and temporary AST evaluation for `-c` only.

Acceptance:

```text
python -c "print(2 + 3 * 4)"
14
```

Temporary AST evaluation must be clearly isolated and removed from normal execution during Phase 3.

### Phase 2: statements and control flow

Deliver assignment, augmented assignment, `if`, `elif`, `else`, `while`, `break`, `continue`, `pass`, file execution, source diagnostics, and symbol validation.

### Phase 3: bytecode VM

Deliver opcode metadata, compiler, constants, names, disassembler, branch patching, verifier, stack VM, and removal of normal AST execution.

All previous tests must execute through verified bytecode.

### Phase 4: functions

Deliver code objects, locals, parameters, calls, explicit and implicit returns, recursion limit, and traceback mapping.

Acceptance:

```python
def fibonacci(n):
    if n < 2:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)

print(fibonacci(10))
```

Expected output:

```text
55
```

### Phase 5: strings, lists, and iteration

Deliver runtime strings, dynamic lists, `len`, indexing, slicing as scoped, `range`, `for`, `list_append`, and `list_pop`.

### Phase 6: AmigaDOS integration

Deliver `argv`, file access, environment access, process exit codes, Amiga integration tests, memory statistics, and emulator automation.

### Phase 7: optional serialized bytecode

Only after source execution is stable, add:

```text
python --compile input.py output.p68c
python output.p68c
```

Do not read or write CPython `.pyc` files. Define a versioned, big-endian project format with magic, format version, language level, flags, section lengths, constants, names, code, line mapping, and checksum.

---

## 28. Agent rules

1. Work one phase at a time.
2. Keep host builds and tests passing after each logical change.
3. Add a regression test before fixing a defect.
4. Treat diagnostics as public behavior.
5. Do not add dependencies required on the Amiga target.
6. Do not optimize without measurements.
7. Never remove validation to gain speed.
8. Check every allocation.
9. Check every size addition and multiplication.
10. Keep pointer ownership explicit.
11. Retain a new value before releasing the value it replaces.
12. Keep cleanup paths deterministic.
13. Compile without warnings under the selected host and vbcc configurations.
14. Do not submit placeholder success paths.
15. Update grammar, bytecode, and compatibility documents with behavior changes.
16. Do not renumber stable opcodes without a bytecode version change.
17. Do not accept unsupported Python syntax accidentally.
18. Ensure the final executable contains no instruction requiring a CPU newer than 68000.

Ownership comments:

```c
/* Borrowed reference. */
Py68String *name;

/* Owned reference. This function must release it or transfer ownership. */
Py68Object *result;
```

---

## 29. Definition of done

The MVP is complete only when:

- `python script.py` runs on an emulated or real Motorola 68000 Amiga.
- `python -c` works.
- scripts execute through verified bytecode, not an AST evaluator.
- functions, calls, recursion, and tracebacks work.
- `if`, `while`, and `for range` work.
- integer division and modulo match the documented semantics.
- strings and lists work within the documented subset.
- unsupported constructs produce intentional diagnostics.
- malformed bytecode is rejected before execution.
- parser, recursion, frame, value-stack, and memory limits fail cleanly.
- host memory tests report no leaks.
- injected allocation failures are handled safely.
- host, differential, and Amiga integration suites pass.
- release build explicitly uses `-cpu=68000 -fpu=0`.
- no required instruction is newer than the Motorola 68000.
- all user-visible incompatibilities are documented.

---

## 30. First instruction to the coding agent

```text
Implement Phase 0 only.

Create the complete repository skeleton, but implement only the files needed
for the bootstrap executable. Build a host executable with GCC or Clang and an
Amiga Hunk executable with vbcc using an explicit Motorola 68000, no-FPU target.

Implement:
- portable integer definitions and compile-time width checks
- Py68Status, Py68Location, and Py68Error
- tracked and tagged allocator
- host and Amiga stdout/stderr platform functions
- Python68K runtime initialization and shutdown
- command parsing for -V and --help
- allocator unit tests, including an injected allocation failure
- exact build instructions

Constraints:
- no tokenizer yet
- no Amiga headers outside platform/amiga
- no unchecked allocation
- no compiler warnings
- no placeholder functions returning false success

Acceptance:
1. host-debug and host-release build
2. amiga-debug and amiga-release build
3. `python -V` prints `Python68K 0.1.0`
4. allocator tests pass
5. clean shutdown reports zero live allocations
```
