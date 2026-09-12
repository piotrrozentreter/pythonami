#include "py68k_token.h"

#include <stddef.h>

void py68_token_array_initialize(Py68TokenArray *tokens)
{
    tokens->items = NULL;
    tokens->count = 0;
    tokens->capacity = 0;
}

void py68_token_array_destroy(Py68Allocator *allocator,
                              Py68TokenArray *tokens)
{
    py68_free(allocator, PY68_MEM_TOKEN, tokens->items,
              tokens->capacity * (Py68U32)sizeof(Py68Token));
    py68_token_array_initialize(tokens);
}