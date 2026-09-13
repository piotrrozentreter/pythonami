/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_TOKENIZER_H
#define PY68K_TOKENIZER_H

#include "py68k_error.h"
#include "py68k_memory.h"
#include "py68k_source.h"
#include "py68k_status.h"
#include "py68k_token.h"

Py68Status py68_tokenize(Py68Allocator *allocator,
                         const Py68Source *source,
                         Py68TokenArray *tokens,
                         Py68Error *error);

#endif