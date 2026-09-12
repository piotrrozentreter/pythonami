#ifndef PY68K_BUILTIN_H
#define PY68K_BUILTIN_H

#include "py68k_runtime.h"

Py68Status py68_builtin_set_copy(Py68Runtime *runtime, Py68U16 name_index,
                                 Py68Value value);
Py68Status py68_builtin_get_copy(Py68Runtime *runtime, Py68U16 name_index,
                                 Py68Value *result);
void py68_builtin_clear(Py68Runtime *runtime);
Py68Status py68_builtin_print(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_len(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_range(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_list_pop(Py68Runtime *runtime, Py68U16 argument_count,
                                Py68Value *arguments, Py68Value *result);

#endif