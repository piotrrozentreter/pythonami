#ifndef PY68K_SYMBOL_H
#define PY68K_SYMBOL_H

#include "py68k_ast.h"
#include "py68k_error.h"
#include "py68k_memory.h"
#include "py68k_source.h"
#include "py68k_status.h"

typedef enum Py68SymbolKind {
    PY68_SYMBOL_LOCAL = 0,
    PY68_SYMBOL_GLOBAL,
    PY68_SYMBOL_BUILTIN,
    PY68_SYMBOL_UNDEFINED
} Py68SymbolKind;

typedef struct Py68Symbol {
    Py68U32 offset;
    Py68U16 length;
    Py68U16 slot;
} Py68Symbol;

typedef struct Py68FunctionSymbols {
    Py68AstNode *function;
    Py68Symbol *parameters;
    Py68U16 parameter_count;
    Py68U16 parameter_capacity;
    Py68Symbol *locals;
    Py68U16 local_count;
    Py68U16 local_capacity;
} Py68FunctionSymbols;

typedef struct Py68SymbolAnalysis {
    Py68FunctionSymbols *functions;
    Py68U16 function_count;
    Py68U16 function_capacity;
    Py68Symbol *globals;
    Py68U16 global_count;
    Py68U16 global_capacity;
} Py68SymbolAnalysis;

void py68_symbol_analysis_initialize(Py68SymbolAnalysis *analysis);
void py68_symbol_analysis_destroy(Py68Allocator *allocator,
                                  Py68SymbolAnalysis *analysis);
Py68Status py68_symbol_analyze(Py68Allocator *allocator,
                               const Py68Source *source,
                               Py68AstNode *module,
                               Py68SymbolAnalysis *analysis,
                               Py68Error *error);
Py68SymbolKind py68_symbol_classify(const Py68Source *source,
                                    const Py68FunctionSymbols *function,
                                    const Py68SymbolAnalysis *analysis,
                                    Py68U32 offset, Py68U16 length,
                                    const char *const *builtins,
                                    Py68U16 builtin_count);
int py68_symbol_is_unbound(Py68U16 slot, Py68U16 parameter_count);
int py68_symbol_lookup_local(const Py68Source *source,
                             const Py68FunctionSymbols *function,
                             Py68U32 offset, Py68U16 length,
                             Py68U16 *slot_out);
const Py68FunctionSymbols *py68_symbol_find_function(
    const Py68SymbolAnalysis *analysis, const Py68AstNode *function_def);

#endif