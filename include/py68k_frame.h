/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_FRAME_H
#define PY68K_FRAME_H

#include "py68k_runtime.h"

Py68Status py68_frame_push(Py68Runtime *runtime, Py68Code *code,
                           Py68Code *return_code, Py68U16 local_count,
                           Py68U16 argument_count, Py68Value *arguments,
                           Py68U32 return_ip,
                           struct Py68Module *globals_owner);
/* Push an activation for a suspended generator: the frame borrows the
   generator's locals and its operands start at stack_base (D-0045). */
Py68Status py68_frame_push_generator(Py68Runtime *runtime,
                                     struct Py68Generator *generator,
                                     Py68Code *return_code, Py68U32 return_ip,
                                     Py68U16 stack_base, Py68U16 resume_kind);
/* Pops the top frame. A generator activation is finished: the generator is
   marked DONE and its locals and saved operands are released. */
void py68_frame_pop(Py68Runtime *runtime);
/* Pops a generator activation that yielded, leaving the generator's locals
   and saved operands intact for the next resume. */
void py68_frame_pop_suspend(Py68Runtime *runtime);
void py68_frame_unwind(Py68Runtime *runtime);
Py68Status py68_frame_get_local(Py68Runtime *runtime, Py68U16 slot,
                                Py68Value *result);
Py68Status py68_frame_set_local_copy(Py68Runtime *runtime, Py68U16 slot,
                                     Py68Value value);

#endif
