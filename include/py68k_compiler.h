#ifndef PY68K_COMPILER_H
#define PY68K_COMPILER_H

#include "py68k_ast.h"
#include "py68k_code.h"
#include "py68k_error.h"
#include "py68k_source.h"
#include "py68k_status.h"

Py68Status py68_compile_module(Py68Allocator *allocator,
                               const Py68Source *source,
                               Py68AstNode *module,
                               Py68Code *code,
                               Py68Error *error);

#endif