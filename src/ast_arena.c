#include "py68k_ast_arena.h"

#include <stddef.h>

static Py68U32 py68_ast_list_size(Py68U16 capacity)
{
    if (capacity > (Py68U16)~(Py68U16)0 /
                    (Py68U16)sizeof(Py68AstNode *)) {
        return 0;
    }
    return (Py68U32)capacity * (Py68U32)sizeof(Py68AstNode *);
}

void py68_ast_list_initialize(Py68AstList *list)
{
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void py68_ast_list_destroy(Py68Allocator *allocator, Py68AstList *list)
{
    py68_free(allocator, PY68_MEM_AST, list->items,
              py68_ast_list_size(list->capacity));
    py68_ast_list_initialize(list);
}

void py68_ast_arena_initialize(Py68AstArena *arena,
                               Py68Allocator *allocator)
{
    arena->allocator = allocator;
    arena->nodes = NULL;
    arena->lists = NULL;
    arena->list_count = 0;
    arena->list_capacity = 0;
}

void py68_ast_arena_destroy(Py68AstArena *arena)
{
    Py68U16 index;
    Py68AstArenaNode *node = arena->nodes;
    Py68AstArenaNode *next;
    for (index = 0; index < arena->list_count; ++index) {
        py68_ast_list_destroy(arena->allocator, arena->lists[index]);
    }
    py68_free(arena->allocator, PY68_MEM_AST, arena->lists,
              (Py68U32)arena->list_capacity *
              (Py68U32)sizeof(Py68AstList *));
    while (node != NULL) {
        next = node->next;
        py68_free(arena->allocator, PY68_MEM_AST, node,
                  (Py68U32)sizeof(Py68AstArenaNode));
        node = next;
    }
    arena->nodes = NULL;
    arena->lists = NULL;
    arena->list_count = 0;
    arena->list_capacity = 0;
}

static Py68Status py68_ast_arena_track_list(Py68AstArena *arena,
                                            Py68AstList *list)
{
    Py68U16 index;
    Py68U16 new_capacity;
    Py68AstList **replacement;
    Py68U32 old_size;
    Py68U32 new_size;

    for (index = 0; index < arena->list_count; ++index) {
        if (arena->lists[index] == list) return PY68_STATUS_OK;
    }
    if (arena->list_count == arena->list_capacity) {
        new_capacity = arena->list_capacity == 0 ? 4 :
                       (Py68U16)(arena->list_capacity * 2);
        old_size = (Py68U32)arena->list_capacity *
                   (Py68U32)sizeof(Py68AstList *);
        new_size = (Py68U32)new_capacity * (Py68U32)sizeof(Py68AstList *);
        replacement = (Py68AstList **)py68_realloc(
            arena->allocator, PY68_MEM_AST, arena->lists, old_size, new_size);
        if (replacement == NULL) return PY68_STATUS_MEMORY_ERROR;
        arena->lists = replacement;
        arena->list_capacity = new_capacity;
    }
    arena->lists[arena->list_count++] = list;
    return PY68_STATUS_OK;
}

Py68Status py68_ast_arena_new(Py68AstArena *arena, Py68AstKind kind,
                              Py68Location location, Py68AstNode **node_out)
{
    Py68AstArenaNode *node;
    node = (Py68AstArenaNode *)py68_alloc(
        arena->allocator, PY68_MEM_AST, (Py68U32)sizeof(Py68AstArenaNode));
    if (node == NULL) {
        return PY68_STATUS_MEMORY_ERROR;
    }
    node->node.kind = (Py68U16)kind;
    node->node.flags = 0;
    node->node.location = location;
    node->next = arena->nodes;
    arena->nodes = node;
    *node_out = &node->node;
    return PY68_STATUS_OK;
}

Py68Status py68_ast_list_append(Py68AstArena *arena, Py68AstList *list,
                                Py68AstNode *node)
{
    Py68AstNode **items;
    Py68U16 new_capacity;
    Py68U32 old_size;
    Py68U32 new_size;

    if (py68_ast_arena_track_list(arena, list) != PY68_STATUS_OK) {
        return PY68_STATUS_MEMORY_ERROR;
    }

    if (list->count == list->capacity) {
        new_capacity = list->capacity == 0 ? 4 :
                       (Py68U16)(list->capacity * 2);
        if (new_capacity < list->capacity ||
            (new_size = py68_ast_list_size(new_capacity)) == 0) {
            return PY68_STATUS_MEMORY_ERROR;
        }
        old_size = py68_ast_list_size(list->capacity);
        items = (Py68AstNode **)py68_realloc(
            arena->allocator, PY68_MEM_AST, list->items, old_size, new_size);
        if (items == NULL) {
            return PY68_STATUS_MEMORY_ERROR;
        }
        list->items = items;
        list->capacity = new_capacity;
    }
    list->items[list->count++] = node;
    return PY68_STATUS_OK;
}