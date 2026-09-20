/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_generator.h"

#include <stddef.h>

Py68Status py68_generator_new(Py68Runtime *runtime, Py68Code *code,
                              struct Py68Module *globals_owner,
                              Py68U16 argument_count, Py68Value *arguments,
                              Py68Generator **result)
{
    Py68Generator *generator;
    Py68U16 local_count;
    Py68U16 capacity;
    Py68U16 index;

    if (runtime == NULL || code == NULL || result == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    if (argument_count > code->local_count) return PY68_STATUS_RUNTIME_ERROR;
    local_count = code->local_count;
    capacity = code->maximum_stack;
    generator = (Py68Generator *)py68_alloc(&runtime->allocator,
                                            PY68_MEM_FUNCTION,
                                            sizeof(Py68Generator));
    if (generator == NULL) return PY68_STATUS_MEMORY_ERROR;
    generator->locals = NULL;
    generator->stack = NULL;
    if (local_count != 0) {
        generator->locals = (Py68Value *)py68_alloc(
            &runtime->allocator, PY68_MEM_STACK,
            (Py68U32)local_count * sizeof(Py68Value));
        if (generator->locals == NULL) {
            py68_free(&runtime->allocator, PY68_MEM_FUNCTION, generator,
                      sizeof(Py68Generator));
            return PY68_STATUS_MEMORY_ERROR;
        }
    }
    if (capacity != 0) {
        generator->stack = (Py68Value *)py68_alloc(
            &runtime->allocator, PY68_MEM_STACK,
            (Py68U32)capacity * sizeof(Py68Value));
        if (generator->stack == NULL) {
            py68_free(&runtime->allocator, PY68_MEM_STACK, generator->locals,
                      (Py68U32)local_count * sizeof(Py68Value));
            py68_free(&runtime->allocator, PY68_MEM_FUNCTION, generator,
                      sizeof(Py68Generator));
            return PY68_STATUS_MEMORY_ERROR;
        }
    }
    generator->base.type = PY68_OBJECT_GENERATOR;
    generator->base.flags = 0;
    generator->base.reference_count = 1;
    generator->base.next_object = runtime->live_objects;
    runtime->live_objects = &generator->base;
    generator->code = code;
    generator->globals_owner = globals_owner;
    generator->local_count = local_count;
    generator->stack_count = 0;
    generator->stack_capacity = capacity;
    generator->state = PY68_GENERATOR_CREATED;
    generator->resume_ip = 0;
    generator->try_count = 0;
    for (index = 0; index < local_count; ++index)
        generator->locals[index] = py68_value_unbound();
    for (index = 0; index < argument_count; ++index) {
        generator->locals[index] = arguments[index];
        py68_value_retain(generator->locals[index]);
    }
    *result = generator;
    return PY68_STATUS_OK;
}

Py68Status py68_generator_store_stack(Py68Generator *generator,
                                     Py68Value *values, Py68U16 count)
{
    Py68U16 index;
    if (generator == NULL) return PY68_STATUS_INTERNAL_ERROR;
    if (count > generator->stack_capacity) return PY68_STATUS_INTERNAL_ERROR;
    for (index = 0; index < count; ++index)
        generator->stack[index] = values[index];
    generator->stack_count = count;
    return PY68_STATUS_OK;
}

void py68_generator_finish(Py68Runtime *runtime, Py68Generator *generator)
{
    Py68U16 index;
    Py68Value *locals;
    Py68Value *stack;
    Py68U16 local_count;
    Py68U16 stack_count;
    Py68U16 stack_capacity;

    if (generator == NULL) return;
    /* Detach before releasing: a released value may reach this generator
       again, and it must then see an already-finished object. */
    locals = generator->locals;
    stack = generator->stack;
    local_count = generator->local_count;
    stack_count = generator->stack_count;
    stack_capacity = generator->stack_capacity;
    generator->locals = NULL;
    generator->stack = NULL;
    generator->local_count = 0;
    generator->stack_count = 0;
    generator->stack_capacity = 0;
    generator->try_count = 0;
    generator->state = PY68_GENERATOR_DONE;
    for (index = 0; index < stack_count; ++index)
        py68_value_release(runtime, stack[index]);
    py68_free(&runtime->allocator, PY68_MEM_STACK, stack,
              (Py68U32)stack_capacity * sizeof(Py68Value));
    for (index = 0; index < local_count; ++index)
        py68_value_release(runtime, locals[index]);
    py68_free(&runtime->allocator, PY68_MEM_STACK, locals,
              (Py68U32)local_count * sizeof(Py68Value));
}

void py68_generator_destroy(Py68Runtime *runtime, Py68Generator *generator)
{
    if (generator == NULL) return;
    py68_generator_finish(runtime, generator);
    py68_free(&runtime->allocator, PY68_MEM_FUNCTION, generator,
              sizeof(Py68Generator));
}
