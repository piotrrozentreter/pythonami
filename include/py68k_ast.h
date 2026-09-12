#ifndef PY68K_AST_H
#define PY68K_AST_H

#include "py68k_location.h"
#include "py68k_memory.h"
#include "py68k_types.h"

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

void py68_ast_list_initialize(Py68AstList *list);
void py68_ast_list_destroy(Py68Allocator *allocator, Py68AstList *list);

#endif