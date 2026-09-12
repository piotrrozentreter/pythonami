#ifndef PY68K_AST_ARENA_H
#define PY68K_AST_ARENA_H

#include "py68k_ast.h"
#include "py68k_status.h"

typedef struct Py68AstArenaNode {
    Py68AstNode node;
    struct Py68AstArenaNode *next;
} Py68AstArenaNode;

typedef struct Py68AstArena {
    Py68Allocator *allocator;
    Py68AstArenaNode *nodes;
    Py68AstList **lists;
    Py68U16 list_count;
    Py68U16 list_capacity;
} Py68AstArena;

void py68_ast_arena_initialize(Py68AstArena *arena,
                               Py68Allocator *allocator);
void py68_ast_arena_destroy(Py68AstArena *arena);
Py68Status py68_ast_arena_new(Py68AstArena *arena, Py68AstKind kind,
                              Py68Location location, Py68AstNode **node_out);
Py68Status py68_ast_list_append(Py68AstArena *arena, Py68AstList *list,
                                Py68AstNode *node);

#endif