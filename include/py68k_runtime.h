#ifndef PY68K_RUNTIME_H
#define PY68K_RUNTIME_H

#include "py68k_error.h"
#include "py68k_code.h"
#include "py68k_memory.h"
#include "py68k_object.h"
#include "py68k_intern.h"
#include "py68k_platform.h"
#include "py68k_status.h"
#include "py68k_types.h"

typedef struct Py68Frame {
    Py68Code *code;
    Py68Code *return_code;
    Py68Value *locals;
    Py68U16 local_count;
    Py68U16 argument_count;
    Py68U32 return_ip;
    Py68U32 instruction_offset;
} Py68Frame;

typedef struct Py68TraceEntry {
    char filename[64];
    char function_name[32];
    Py68U16 function_name_length;
    Py68U32 offset;
    Py68U32 line;
    Py68U16 column;
} Py68TraceEntry;

typedef struct Py68GlobalEntry {
    Py68U16 name_index;
    Py68String *name_string;
    Py68Value value;
    Py68U16 occupied;
} Py68GlobalEntry;

struct Py68Runtime {
    Py68Allocator allocator;
    Py68Error error;
    Py68Value *value_stack;
    Py68U16 value_stack_count;
    Py68U16 value_stack_capacity;
    Py68Frame *frames;
    Py68U16 frame_count;
    Py68U16 frame_capacity;
    Py68U16 recursion_limit;
    Py68GlobalEntry *globals;
    Py68U16 global_count;
    Py68U16 global_capacity;
    Py68GlobalEntry *builtins;
    Py68U16 builtin_count;
    Py68U16 builtin_capacity;
    Py68InternTable interned_names;
    Py68Object *live_objects;
    Py68I32 requested_exit_code;
    Py68U16 trace_enabled;
    Py68TraceEntry traceback[16];
    Py68U16 traceback_count;
};

void py68_runtime_initialize_struct(Py68Runtime *runtime);
Py68Status py68_runtime_initialize(Py68Runtime *runtime);
void py68_runtime_shutdown(Py68Runtime *runtime);

#endif
