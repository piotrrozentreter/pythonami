/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_GENERATOR_H
#define PY68K_GENERATOR_H

#include "py68k_code.h"
#include "py68k_object.h"
#include "py68k_runtime.h"
#include "py68k_status.h"
#include "py68k_value.h"

typedef enum Py68GeneratorState {
    PY68_GENERATOR_CREATED = 0,
    PY68_GENERATOR_RUNNING,
    PY68_GENERATOR_SUSPENDED,
    PY68_GENERATOR_DONE
} Py68GeneratorState;

struct Py68Generator {
    Py68Object base;
    /* Borrowed, exactly like Py68Function::code: the bytecode lives in the
       defining module's code tree, which outlives every generator. */
    Py68Code *code;
    /* Borrowed defining module for LOAD_GLOBAL; NULL uses runtime->globals. */
    struct Py68Module *globals_owner;
    /* Owned local slots; borrowed by the activation frame while running. */
    Py68Value *locals;
    /* Owned operand values saved while suspended. Sized once at creation to
       code->maximum_stack so suspending never allocates (D-0045). */
    Py68Value *stack;
    Py68U16 local_count;
    Py68U16 stack_count;
    Py68U16 stack_capacity;
    Py68U16 state;
    Py68U32 resume_ip;
    /* Try blocks saved with depths relative to the activation stack base. */
    Py68TryBlock try_stack[PY68_TRY_MAX];
    Py68U16 try_count;
};
typedef struct Py68Generator Py68Generator;

Py68Status py68_generator_new(struct Py68Runtime *runtime, Py68Code *code,
                              struct Py68Module *globals_owner,
                              Py68U16 argument_count, Py68Value *arguments,
                              Py68Generator **result);
/* Moves `count` operand values, ownership included, into the generator. */
Py68Status py68_generator_store_stack(Py68Generator *generator,
                                      Py68Value *values, Py68U16 count);
/* Releases locals and saved operands and marks the generator DONE. User
   `finally` blocks are not executed; see D-0045. Idempotent. */
void py68_generator_finish(struct Py68Runtime *runtime,
                           Py68Generator *generator);
void py68_generator_destroy(struct Py68Runtime *runtime,
                            Py68Generator *generator);

#endif
