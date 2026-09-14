/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_RUNTIME_H
#define PY68K_RUNTIME_H

#include "py68k_error.h"
#include "py68k_code.h"
#include "py68k_memory.h"
#include "py68k_object.h"
#include "py68k_platform.h"
#include "py68k_status.h"
#include "py68k_types.h"

#define PY68_TRY_MAX 8
#define PY68_PATH_MAX 256

typedef struct Py68TryBlock {
    Py68U32 handler_ip;
    Py68U16 stack_depth;
} Py68TryBlock;

typedef struct Py68Frame {
    Py68Code *code;
    Py68Code *return_code;
    Py68Value *locals;
    /* Borrowed defining module for LOAD_GLOBAL; NULL uses runtime->globals. */
    struct Py68Module *globals_owner;
    Py68U16 local_count;
    Py68U16 argument_count;
    Py68U32 return_ip;
    Py68TryBlock try_stack[PY68_TRY_MAX];
    Py68U16 try_count;
} Py68Frame;

typedef struct Py68GlobalEntry {
    const Py68U8 *name;      /* borrowed: bytes owned by a live source buffer */
    Py68U16 name_length;
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
    Py68Object *live_objects;
    Py68I32 requested_exit_code;
    Py68U16 trace_enabled;
    char traceback[512];
    Py68U16 traceback_length;
    Py68Value current_exception;
    char script_dir[PY68_PATH_MAX];
    struct Py68Object **import_modules;
    Py68U16 import_count;
    Py68U16 import_capacity;
    struct Py68List *sys_path;
    struct Py68List *sys_argv;
    struct Py68Object *sys_module;
    struct Py68NativeFunction *active_native;
    /* Set while executing an imported module body so MAKE_FUNCTION can bind
       LOAD_GLOBAL to that module's globals. */
    struct Py68Module *executing_module;
};

void py68_runtime_initialize_struct(Py68Runtime *runtime);
Py68Status py68_runtime_initialize(Py68Runtime *runtime);
void py68_runtime_shutdown(Py68Runtime *runtime);

#endif
