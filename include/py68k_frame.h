#ifndef PY68K_FRAME_H
#define PY68K_FRAME_H

#include "py68k_runtime.h"

Py68Status py68_frame_push(Py68Runtime *runtime, Py68Code *code,
                           Py68Code *return_code, Py68U16 local_count,
                           Py68U16 argument_count, Py68Value *arguments,
                           Py68U32 return_ip);
void py68_frame_pop(Py68Runtime *runtime);
void py68_frame_unwind(Py68Runtime *runtime);
Py68Status py68_frame_get_local(Py68Runtime *runtime, Py68U16 slot,
                                Py68Value *result);
Py68Status py68_frame_set_local_copy(Py68Runtime *runtime, Py68U16 slot,
                                     Py68Value value);

#endif
