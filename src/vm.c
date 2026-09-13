/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_vm.h"
#include "py68k_verify.h"
#include "py68k_native.h"
#include "py68k_global.h"
#include "py68k_function.h"
#include "py68k_frame.h"
#include "py68k_builtin.h"
#include "py68k_string.h"
#include "py68k_list.h"
#include "py68k_range.h"

#include <stddef.h>

static void py68_vm_error(Py68Runtime *runtime, Py68ErrorKind kind,
                          const char *message);
static void py68_vm_clear_stack(Py68Runtime *runtime);

static void py68_vm_error(Py68Runtime *runtime, Py68ErrorKind kind,
                          const char *message)
{
    Py68Location location;
    location.offset = 0; location.line = 0; location.column = 0; location.length = 0;
    py68_error_set(&runtime->error, kind, location, NULL, message);
}

static void py68_vm_append_traceback_text(Py68Runtime *runtime,
                                          const char *text)
{
    Py68U32 index;
    for (index = 0; text[index] != '\0' &&
         runtime->traceback_length < sizeof(runtime->traceback) - 1; ++index)
        runtime->traceback[runtime->traceback_length++] = text[index];
}

static void py68_vm_append_traceback_u32(Py68Runtime *runtime, Py68U32 value)
{
    char digits[10];
    Py68U32 count = 0;
    do {
        digits[count++] = (char)('0' + (value % 10));
        value /= 10;
    } while (value != 0 && count < sizeof(digits));
    while (count != 0 &&
           runtime->traceback_length < sizeof(runtime->traceback) - 1)
        runtime->traceback[runtime->traceback_length++] = digits[--count];
}

static void py68_vm_append_frame_line(Py68Runtime *runtime, Py68Code *code,
                                      Py68U32 ip)
{
    Py68U16 line = 0;
    Py68U16 name_index;
    py68_vm_append_traceback_text(runtime, "  at ");
    if (code != NULL && code->name_length != 0 && code->source_data != NULL) {
        for (name_index = 0; name_index < code->name_length &&
             runtime->traceback_length < sizeof(runtime->traceback) - 1;
             ++name_index)
            runtime->traceback[runtime->traceback_length++] =
                (char)code->source_data[code->name_offset + name_index];
    } else {
        py68_vm_append_traceback_text(runtime, "<module>");
    }
    py68_vm_append_traceback_text(runtime, " (");
    if (code != NULL && code->line_map != NULL &&
        ip < code->bytecode_length)
        line = code->line_map[ip];
    if (line != 0) {
        py68_vm_append_traceback_text(runtime, "line ");
        py68_vm_append_traceback_u32(runtime, line);
    } else {
        py68_vm_append_traceback_text(runtime, "line ?");
    }
    py68_vm_append_traceback_text(runtime, ")\n");
}

static void py68_vm_capture_traceback(Py68Runtime *runtime,
                                      Py68Code *current_code, Py68U32 ip)
{
    Py68U16 frame_index;
    runtime->traceback_length = 0;
    if (current_code != NULL)
        py68_vm_append_frame_line(runtime, current_code, ip);
    if (runtime->frame_count == 0) return;
    for (frame_index = runtime->frame_count; frame_index > 0; --frame_index) {
        Py68Frame *frame = &runtime->frames[frame_index - 1];
        if (frame->return_code == NULL) continue;
        py68_vm_append_frame_line(runtime, frame->return_code,
                                  frame->return_ip == 0 ? 0
                                                        : frame->return_ip - 1);
    }
    if (runtime->error.location.line == 0 && current_code != NULL &&
        current_code->line_map != NULL && ip < current_code->bytecode_length) {
        runtime->error.location.line = current_code->line_map[ip];
    }
}

static void py68_vm_fail(Py68Runtime *runtime, Py68Code *current_code,
                         Py68U32 ip)
{
    if (runtime->error.kind != PY68_ERROR_NONE)
        py68_vm_capture_traceback(runtime, current_code, ip);
    py68_vm_clear_stack(runtime);
    py68_frame_unwind(runtime);
}

static Py68Status py68_vm_grow(Py68Runtime *runtime)
{
    Py68U16 capacity = runtime->value_stack_capacity == 0 ? 32 :
                       (Py68U16)(runtime->value_stack_capacity * 2);
    Py68Value *values;
    if (capacity < runtime->value_stack_capacity) return PY68_STATUS_MEMORY_ERROR;
    values = (Py68Value *)py68_realloc(&runtime->allocator, PY68_MEM_STACK,
        runtime->value_stack,
        (Py68U32)runtime->value_stack_capacity * sizeof(Py68Value),
        (Py68U32)capacity * sizeof(Py68Value));
    if (values == NULL) return PY68_STATUS_MEMORY_ERROR;
    runtime->value_stack = values;
    runtime->value_stack_capacity = capacity;
    return PY68_STATUS_OK;
}

static Py68Status py68_vm_push(Py68Runtime *runtime, Py68Value value)
{
    Py68Status status;
    if (runtime->value_stack_count == runtime->value_stack_capacity) {
        status = py68_vm_grow(runtime);
        if (status != PY68_STATUS_OK) return status;
    }
    runtime->value_stack[runtime->value_stack_count++] = value;
    return PY68_STATUS_OK;
}

Py68Status py68_vm_push_owned(Py68Runtime *runtime, Py68Value value)
{
    return py68_vm_push(runtime, value);
}

static Py68Status py68_vm_pop(Py68Runtime *runtime, Py68Value *value)
{
    if (runtime->value_stack_count == 0) {
        py68_vm_error(runtime, PY68_ERROR_BYTECODE, "value stack underflow");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *value = runtime->value_stack[--runtime->value_stack_count];
    return PY68_STATUS_OK;
}

static int py68_vm_truth(Py68Value value)
{
    if (value.type == PY68_VM_NONE) return 0;
    if (value.type == PY68_VM_BOOL || value.type == PY68_VM_INT)
        return value.as.integer != 0;
    if (value.type == PY68_VALUE_OBJECT && value.as.object != NULL) {
        if (value.as.object->type == PY68_OBJECT_STRING)
            return ((Py68String *)value.as.object)->length != 0;
        if (value.as.object->type == PY68_OBJECT_LIST)
            return ((Py68List *)value.as.object)->count != 0;
    }
    return 1;
}

static void py68_vm_clear_stack(Py68Runtime *runtime)
{
    while (runtime->value_stack_count != 0) {
        Py68Value value = runtime->value_stack[--runtime->value_stack_count];
        py68_value_release(runtime, value);
    }
}

static int py68_vm_add(Py68I32 left, Py68I32 right, Py68I32 *result)
{
    if ((right > 0 && left > (Py68I32)0x7fffffff - right) ||
        (right < 0 && left < (Py68I32)-2147483647 - 1 - right)) return 0;
    *result = left + right;
    return 1;
}

static int py68_vm_subtract(Py68I32 left, Py68I32 right, Py68I32 *result)
{
    if ((right < 0 && left > (Py68I32)0x7fffffff + right) ||
        (right > 0 && left < (Py68I32)-2147483647 - 1 + right)) return 0;
    *result = left - right;
    return 1;
}

static int py68_vm_multiply(Py68I32 left, Py68I32 right, Py68I32 *result)
{
    if (left == 0 || right == 0) { *result = 0; return 1; }
    if (left == -1 && right == (Py68I32)-2147483647 - 1) return 0;
    if (right == -1 && left == (Py68I32)-2147483647 - 1) return 0;
    if (left > 0) {
        if (right > 0 && left > 2147483647L / right) return 0;
        if (right < 0 && right < -2147483647L / left - 1) return 0;
    } else {
        if (right > 0 && left < -2147483647L / right - 1) return 0;
        if (right < 0 && left < 2147483647L / right) return 0;
    }
    *result = left * right;
    return 1;
}

static Py68Status py68_vm_binary(Py68Runtime *runtime, Py68U8 opcode)
{
    Py68Value left, right, result;
    Py68I32 quotient, remainder, integer;
    if (py68_vm_pop(runtime, &right) != PY68_STATUS_OK ||
        py68_vm_pop(runtime, &left) != PY68_STATUS_OK) {
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (opcode == OP_ADD && left.type == PY68_VALUE_OBJECT &&
        right.type == PY68_VALUE_OBJECT && left.as.object != NULL &&
        right.as.object != NULL) {
        if (left.as.object->type == PY68_OBJECT_STRING &&
            right.as.object->type == PY68_OBJECT_STRING) {
            Py68String *string;
            Py68Status status = py68_string_concat(
                runtime, (Py68String *)left.as.object,
                (Py68String *)right.as.object, &string);
            py68_value_release(runtime, left);
            py68_value_release(runtime, right);
            if (status != PY68_STATUS_OK) return status;
            return py68_vm_push(runtime, py68_value_from_object(&string->base));
        }
        if (left.as.object->type == PY68_OBJECT_LIST &&
            right.as.object->type == PY68_OBJECT_LIST) {
            Py68List *list;
            Py68Status status = py68_list_concat(
                runtime, (Py68List *)left.as.object,
                (Py68List *)right.as.object, &list);
            py68_value_release(runtime, left);
            py68_value_release(runtime, right);
            if (status != PY68_STATUS_OK) return status;
            return py68_vm_push(runtime, py68_value_from_object(&list->base));
        }
    }
    if (opcode == OP_EQUAL || opcode == OP_NOT_EQUAL) {
        if (left.type == PY68_VALUE_NONE || right.type == PY68_VALUE_NONE) {
            integer = (left.type == PY68_VALUE_NONE &&
                       right.type == PY68_VALUE_NONE);
            if (opcode == OP_NOT_EQUAL) integer = !integer;
            py68_value_release(runtime, left);
            py68_value_release(runtime, right);
            result.type = PY68_VM_BOOL;
            result.reserved = 0;
            result.as.integer = integer;
            return py68_vm_push(runtime, result);
        }
    }
    if ((left.type != PY68_VM_INT && left.type != PY68_VM_BOOL) ||
        (right.type != PY68_VM_INT && right.type != PY68_VM_BOOL)) {
        py68_value_release(runtime, left);
        py68_value_release(runtime, right);
        py68_vm_error(runtime, PY68_ERROR_TYPE, "integer operands required");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    result.type = PY68_VM_INT; result.reserved = 0;
    if (opcode == OP_ADD) {
        if (!py68_vm_add(left.as.integer, right.as.integer, &integer)) goto overflow;
    } else if (opcode == OP_SUBTRACT) {
        if (!py68_vm_subtract(left.as.integer, right.as.integer, &integer)) goto overflow;
    } else if (opcode == OP_MULTIPLY) {
        if (!py68_vm_multiply(left.as.integer, right.as.integer, &integer)) goto overflow;
    } else if (opcode == OP_FLOOR_DIVIDE || opcode == OP_MODULO) {
        if (right.as.integer == 0) {
            py68_vm_error(runtime, PY68_ERROR_ZERO_DIVISION, "division by zero");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        if (left.as.integer == (Py68I32)-2147483647 - 1 && right.as.integer == -1)
            goto overflow;
        quotient = left.as.integer / right.as.integer;
        remainder = left.as.integer % right.as.integer;
        if (remainder != 0 && ((left.as.integer < 0) != (right.as.integer < 0))) {
            --quotient; remainder += right.as.integer;
        }
        integer = opcode == OP_FLOOR_DIVIDE ? quotient : remainder;
    } else if (opcode == OP_EQUAL || opcode == OP_NOT_EQUAL ||
               opcode == OP_LESS || opcode == OP_LESS_EQUAL ||
               opcode == OP_GREATER || opcode == OP_GREATER_EQUAL) {
        result.type = PY68_VM_BOOL;
        if (opcode == OP_EQUAL) integer = left.as.integer == right.as.integer;
        else if (opcode == OP_NOT_EQUAL) integer = left.as.integer != right.as.integer;
        else if (opcode == OP_LESS) integer = left.as.integer < right.as.integer;
        else if (opcode == OP_LESS_EQUAL) integer = left.as.integer <= right.as.integer;
        else if (opcode == OP_GREATER) integer = left.as.integer > right.as.integer;
        else integer = left.as.integer >= right.as.integer;
    } else goto bad_opcode;
    result.as.integer = integer;
    return py68_vm_push(runtime, result);
overflow:
    py68_vm_error(runtime, PY68_ERROR_OVERFLOW, "integer overflow");
    return PY68_STATUS_RUNTIME_ERROR;
bad_opcode:
    py68_vm_error(runtime, PY68_ERROR_BYTECODE, "unsupported arithmetic opcode");
    return PY68_STATUS_RUNTIME_ERROR;
}

Py68Status py68_vm_execute(Py68Runtime *runtime, Py68Code *code)
{
    Py68Code *current_code = code;
    Py68U32 ip = 0;
    Py68U32 target;
    Py68U16 index;
    Py68U8 opcode;
    Py68Value value, duplicate;
    Py68Status status;

    status = py68_verify_code(current_code, &runtime->error);
    if (status != PY68_STATUS_OK) return status;
    runtime->frame_count = 0;
    status = py68_frame_push(runtime, current_code, NULL, 0, 0, NULL, 0);
    if (status != PY68_STATUS_OK) return status;
    while (ip < current_code->bytecode_length) {
        opcode = current_code->bytecode[ip];
        switch (opcode) {
        case OP_HALT:
            if (runtime->frame_count > 1) {
                Py68Code *caller_code =
                    runtime->frames[runtime->frame_count - 1].return_code;
                Py68U32 caller_ip =
                    runtime->frames[runtime->frame_count - 1].return_ip;
                py68_frame_pop(runtime);
                current_code = caller_code;
                ip = caller_ip;
                status = py68_vm_push(runtime, py68_value_none());
                break;
            }
            py68_frame_pop(runtime);
            return PY68_STATUS_OK;
        case OP_LOAD_NONE: case OP_LOAD_TRUE: case OP_LOAD_FALSE:
            value = py68_value_none();
            value.type = opcode == OP_LOAD_NONE ? PY68_VM_NONE : PY68_VM_BOOL;
            value.as.integer = opcode == OP_LOAD_TRUE;
            status = py68_vm_push(runtime, value); ip += 1; break;
        case OP_LOAD_CONST:
            index = (Py68U16)(((Py68U16)current_code->bytecode[ip + 1] << 8) |
                              current_code->bytecode[ip + 2]);
            if (current_code->constants[index].kind == PY68_CONSTANT_STRING) {
                Py68String *string;
                Py68Constant *constant = &current_code->constants[index];
                if (current_code->source_data == NULL ||
                    constant->offset + constant->length > current_code->source_length) {
                    py68_vm_error(runtime, PY68_ERROR_BYTECODE, "string constant out of range");
                    status = PY68_STATUS_RUNTIME_ERROR; ip = current_code->bytecode_length; break;
                }
                status = py68_string_new_copy(runtime,
                    (const char *)current_code->source_data + constant->offset,
                    constant->length, &string);
                if (status == PY68_STATUS_OK)
                    status = py68_vm_push(runtime, py68_value_from_object(&string->base));
                ip += 3;
                break;
            }
            if (current_code->constants[index].kind != PY68_CONSTANT_INTEGER) {
                py68_vm_error(runtime, PY68_ERROR_TYPE, "integer constant required");
                status = PY68_STATUS_RUNTIME_ERROR; ip = current_code->bytecode_length; break;
            }
            value = py68_value_int(current_code->constants[index].integer);
            status = py68_vm_push(runtime, value); ip += 3; break;
        case OP_LOAD_GLOBAL: {
            const Py68U8 *name_bytes;
            Py68U16 name_length;
            index = (Py68U16)(((Py68U16)current_code->bytecode[ip + 1] << 8) |
                              current_code->bytecode[ip + 2]);
            name_bytes = current_code->source_data +
                        current_code->name_offsets[index];
            name_length = current_code->name_lengths[index];
            status = py68_global_get_copy(runtime, name_bytes, name_length,
                                          &value);
            if (status != PY68_STATUS_OK)
                status = py68_builtin_get_copy(runtime, name_bytes,
                                               name_length, &value);
            if (status == PY68_STATUS_OK)
                status = py68_vm_push(runtime, value);
            else
                py68_vm_error(runtime, PY68_ERROR_NAME, "name is not defined");
            ip += 3; break;
        }
        case OP_STORE_GLOBAL: {
            const Py68U8 *name_bytes;
            Py68U16 name_length;
            index = (Py68U16)(((Py68U16)current_code->bytecode[ip + 1] << 8) |
                              current_code->bytecode[ip + 2]);
            name_bytes = current_code->source_data +
                        current_code->name_offsets[index];
            name_length = current_code->name_lengths[index];
            status = py68_vm_pop(runtime, &value);
            if (status == PY68_STATUS_OK) {
                status = py68_global_set_copy(runtime, name_bytes,
                                              name_length, value);
                py68_value_release(runtime, value);
            }
            ip += 3; break;
        }
        case OP_LOAD_LOCAL:
            index = (Py68U16)(((Py68U16)current_code->bytecode[ip + 1] << 8) |
                              current_code->bytecode[ip + 2]);
            status = py68_frame_get_local(runtime, index, &value);
            if (status == PY68_STATUS_OK && value.type == PY68_VALUE_UNBOUND) {
                py68_vm_error(runtime, PY68_ERROR_NAME,
                              "local variable referenced before assignment");
                status = PY68_STATUS_RUNTIME_ERROR;
            } else if (status == PY68_STATUS_OK) {
                status = py68_vm_push(runtime, value);
            }
            ip += 3; break;
        case OP_STORE_LOCAL:
            index = (Py68U16)(((Py68U16)current_code->bytecode[ip + 1] << 8) |
                              current_code->bytecode[ip + 2]);
            status = py68_vm_pop(runtime, &value);
            if (status == PY68_STATUS_OK) {
                status = py68_frame_set_local_copy(runtime, index, value);
                py68_value_release(runtime, value);
            }
            ip += 3; break;
        case OP_MAKE_FUNCTION: {
            Py68Code *nested_code;
            Py68Function *made_function;
            index = (Py68U16)(((Py68U16)current_code->bytecode[ip + 1] << 8) |
                              current_code->bytecode[ip + 2]);
            if (index >= current_code->constant_count ||
                current_code->constants[index].kind != PY68_CONSTANT_CODE ||
                current_code->constants[index].integer >=
                    current_code->nested_count) {
                py68_vm_error(runtime, PY68_ERROR_BYTECODE,
                              "invalid function constant");
                status = PY68_STATUS_RUNTIME_ERROR;
                ip = current_code->bytecode_length;
                break;
            }
            nested_code = &current_code->nested[
                current_code->constants[index].integer];
            status = py68_function_new(runtime, nested_code,
                                       nested_code->argument_count,
                                       nested_code->local_count,
                                       &made_function);
            if (status == PY68_STATUS_OK) {
                status = py68_vm_push(runtime,
                    py68_value_from_object(&made_function->base));
            }
            ip += 3; break;
        }
        case OP_POP:
            status = py68_vm_pop(runtime, &value);
            if (status == PY68_STATUS_OK)
                py68_value_release(runtime, value);
            ip += 1;
            break;
        case OP_STORE_INDEX: {
            Py68Value value_val, index_val, container_val;
            status = py68_vm_pop(runtime, &value_val);
            if (status == PY68_STATUS_OK)
                status = py68_vm_pop(runtime, &index_val);
            if (status == PY68_STATUS_OK)
                status = py68_vm_pop(runtime, &container_val);
            if (status != PY68_STATUS_OK) break;
            if (index_val.type != PY68_VALUE_INT &&
                index_val.type != PY68_VALUE_BOOL) {
                py68_value_release(runtime, value_val);
                py68_value_release(runtime, index_val);
                py68_value_release(runtime, container_val);
                py68_vm_error(runtime, PY68_ERROR_TYPE, "index must be integer");
                status = PY68_STATUS_RUNTIME_ERROR;
                ip = current_code->bytecode_length;
                break;
            }
            if (container_val.type == PY68_VALUE_OBJECT &&
                container_val.as.object != NULL &&
                container_val.as.object->type == PY68_OBJECT_LIST) {
                status = py68_list_set_copy(
                    runtime, (Py68List *)container_val.as.object,
                    index_val.as.integer, value_val);
                if (status != PY68_STATUS_OK) {
                    py68_vm_error(runtime, PY68_ERROR_INDEX,
                                  "list index out of range");
                    status = PY68_STATUS_RUNTIME_ERROR;
                    ip = current_code->bytecode_length;
                }
            } else if (container_val.type == PY68_VALUE_OBJECT &&
                       container_val.as.object != NULL &&
                       container_val.as.object->type == PY68_OBJECT_STRING) {
                py68_vm_error(runtime, PY68_ERROR_TYPE,
                              "strings do not support item assignment");
                status = PY68_STATUS_RUNTIME_ERROR;
                ip = current_code->bytecode_length;
            } else {
                py68_vm_error(runtime, PY68_ERROR_TYPE,
                              "container does not support item assignment");
                status = PY68_STATUS_RUNTIME_ERROR;
                ip = current_code->bytecode_length;
            }
            py68_value_release(runtime, value_val);
            py68_value_release(runtime, index_val);
            py68_value_release(runtime, container_val);
            ip += 1;
            break;
        }
        case OP_BUILD_LIST: {
            Py68U16 count = (Py68U16)(((Py68U16)current_code->bytecode[ip + 1] << 8) |
                                      current_code->bytecode[ip + 2]);
            Py68List *list;
            Py68U16 item;
            if (runtime->value_stack_count < count ||
                py68_list_new(runtime, &list) != PY68_STATUS_OK) {
                py68_vm_error(runtime, PY68_ERROR_MEMORY, "list construction failed");
                status = PY68_STATUS_RUNTIME_ERROR;
                ip = current_code->bytecode_length;
                break;
            }
            for (item = 0; item < count; ++item) {
                Py68Value element = runtime->value_stack[
                    runtime->value_stack_count - count + item];
                status = py68_list_append_copy(runtime, list, element);
                if (status != PY68_STATUS_OK) break;
            }
            if (status == PY68_STATUS_OK) {
                while (count != 0) {
                    --count;
                    py68_value_release(runtime, runtime->value_stack[
                        runtime->value_stack_count - 1]);
                    --runtime->value_stack_count;
                }
                status = py68_vm_push(runtime, py68_value_from_object(&list->base));
            } else {
                py68_object_release(runtime, &list->base);
            }
            ip += 3;
            break;
        }
        case OP_RANGE_INIT: {
            Py68U8 argc = current_code->bytecode[ip + 1];
            Py68I32 start = 0;
            Py68I32 stop = 0;
            Py68I32 step = 1;
            Py68Range *range_obj = NULL;
            Py68Value arg0, arg1, arg2;
            if (argc < 1 || argc > 3 || runtime->value_stack_count < argc) {
                py68_vm_error(runtime, PY68_ERROR_TYPE, "range arguments invalid");
                status = PY68_STATUS_RUNTIME_ERROR;
                ip = current_code->bytecode_length;
                break;
            }
            if (argc == 1) {
                status = py68_vm_pop(runtime, &arg0);
                if (status != PY68_STATUS_OK) break;
                if (arg0.type == PY68_VALUE_OBJECT && arg0.as.object != NULL &&
                    arg0.as.object->type == PY68_OBJECT_RANGE) {
                    status = py68_vm_push(runtime, arg0);
                    ip += 2;
                    break;
                }
                if (arg0.type == PY68_VALUE_OBJECT && arg0.as.object != NULL &&
                    arg0.as.object->type == PY68_OBJECT_LIST) {
                    status = py68_range_new_list(runtime, (Py68List *)arg0.as.object,
                                                 &range_obj);
                    py68_value_release(runtime, arg0);
                    if (status == PY68_STATUS_OK) {
                        status = py68_vm_push(runtime,
                            py68_value_from_object(&range_obj->base));
                    }
                    ip += 2;
                    break;
                }
                if (arg0.type != PY68_VALUE_INT) {
                    py68_value_release(runtime, arg0);
                    py68_vm_error(runtime, PY68_ERROR_TYPE, "integer required for range");
                    status = PY68_STATUS_RUNTIME_ERROR;
                    ip = current_code->bytecode_length;
                    break;
                }
                stop = arg0.as.integer;
            } else if (argc == 2) {
                status = py68_vm_pop(runtime, &arg1);
                if (status == PY68_STATUS_OK) status = py68_vm_pop(runtime, &arg0);
                if (status != PY68_STATUS_OK ||
                    arg0.type != PY68_VALUE_INT || arg1.type != PY68_VALUE_INT) {
                    py68_vm_error(runtime, PY68_ERROR_TYPE, "integers required for range");
                    status = PY68_STATUS_RUNTIME_ERROR;
                    ip = current_code->bytecode_length;
                    break;
                }
                start = arg0.as.integer;
                stop = arg1.as.integer;
            } else {
                status = py68_vm_pop(runtime, &arg2);
                if (status == PY68_STATUS_OK) status = py68_vm_pop(runtime, &arg1);
                if (status == PY68_STATUS_OK) status = py68_vm_pop(runtime, &arg0);
                if (status != PY68_STATUS_OK ||
                    arg0.type != PY68_VALUE_INT || arg1.type != PY68_VALUE_INT ||
                    arg2.type != PY68_VALUE_INT) {
                    py68_vm_error(runtime, PY68_ERROR_TYPE, "integers required for range");
                    status = PY68_STATUS_RUNTIME_ERROR;
                    ip = current_code->bytecode_length;
                    break;
                }
                start = arg0.as.integer;
                stop = arg1.as.integer;
                step = arg2.as.integer;
            }
            if (step == 0) {
                py68_vm_error(runtime, PY68_ERROR_VALUE, "range step cannot be zero");
                status = PY68_STATUS_RUNTIME_ERROR;
                ip = current_code->bytecode_length;
                break;
            }
            status = py68_range_new(runtime, start, stop, step, &range_obj);
            if (status == PY68_STATUS_OK) {
                status = py68_vm_push(runtime,
                    py68_value_from_object(&range_obj->base));
            }
            ip += 2;
            break;
        }
        case OP_RANGE_NEXT: {
            Py68I16 displacement = (Py68I16)(((Py68U16)current_code->bytecode[ip + 1] << 8) |
                                              current_code->bytecode[ip + 2]);
            Py68Value range_val;
            Py68Range *range_obj;
            Py68Value next_val;
            int has_next = 0;
            if (runtime->value_stack_count == 0) {
                py68_vm_error(runtime, PY68_ERROR_BYTECODE, "value stack underflow");
                status = PY68_STATUS_RUNTIME_ERROR;
                ip = current_code->bytecode_length;
                break;
            }
            range_val = runtime->value_stack[runtime->value_stack_count - 1];
            if (range_val.type != PY68_VALUE_OBJECT || range_val.as.object == NULL ||
                range_val.as.object->type != PY68_OBJECT_RANGE) {
                py68_vm_error(runtime, PY68_ERROR_TYPE, "range iterator required");
                status = PY68_STATUS_RUNTIME_ERROR;
                ip = current_code->bytecode_length;
                break;
            }
            range_obj = (Py68Range *)range_val.as.object;
            status = py68_range_next_value(runtime, range_obj, &next_val, &has_next);
            if (status != PY68_STATUS_OK) {
                ip = current_code->bytecode_length;
                break;
            }
            if (has_next) {
                status = py68_vm_push(runtime, next_val);
                ip += 3;
            } else {
                status = py68_vm_pop(runtime, &range_val);
                py68_value_release(runtime, range_val);
                target = (Py68U32)((Py68I32)(ip + 3) + (Py68I32)displacement);
                ip = target;
                status = PY68_STATUS_OK;
            }
            break;
        }
        case OP_DUP:
            if (runtime->value_stack_count == 0) {
                py68_vm_error(runtime, PY68_ERROR_BYTECODE, "value stack underflow");
                status = PY68_STATUS_RUNTIME_ERROR;
            } else {
                duplicate = runtime->value_stack[runtime->value_stack_count - 1];
                status = py68_vm_push(runtime, duplicate);
            }
            ip += 1; break;
        case OP_LOAD_INDEX: {
            Py68Value index_val, container_val, item_val;
            status = py68_vm_pop(runtime, &index_val);
            if (status == PY68_STATUS_OK)
                status = py68_vm_pop(runtime, &container_val);
            if (status != PY68_STATUS_OK) break;
            if (index_val.type != PY68_VALUE_INT) {
                py68_value_release(runtime, index_val);
                py68_value_release(runtime, container_val);
                py68_vm_error(runtime, PY68_ERROR_TYPE, "index must be integer");
                status = PY68_STATUS_RUNTIME_ERROR;
                ip = current_code->bytecode_length;
                break;
            }
            if (container_val.type == PY68_VALUE_OBJECT &&
                container_val.as.object != NULL &&
                container_val.as.object->type == PY68_OBJECT_LIST) {
                status = py68_list_get_copy(runtime,
                    (Py68List *)container_val.as.object,
                    index_val.as.integer, &item_val);
                if (status != PY68_STATUS_OK) {
                    py68_vm_error(runtime, PY68_ERROR_INDEX, "list index out of range");
                    status = PY68_STATUS_RUNTIME_ERROR;
                    ip = current_code->bytecode_length;
                } else {
                    status = py68_vm_push(runtime, item_val);
                }
            } else if (container_val.type == PY68_VALUE_OBJECT &&
                       container_val.as.object != NULL &&
                       container_val.as.object->type == PY68_OBJECT_STRING) {
                Py68String *character;
                status = py68_string_get_char(
                    runtime, (Py68String *)container_val.as.object,
                    index_val.as.integer, &character);
                if (status != PY68_STATUS_OK) {
                    py68_vm_error(runtime, PY68_ERROR_INDEX,
                                  "string index out of range");
                    status = PY68_STATUS_RUNTIME_ERROR;
                    ip = current_code->bytecode_length;
                } else {
                    status = py68_vm_push(
                        runtime, py68_value_from_object(&character->base));
                }
            } else {
                py68_vm_error(runtime, PY68_ERROR_TYPE, "container does not support indexing");
                status = PY68_STATUS_RUNTIME_ERROR;
                ip = current_code->bytecode_length;
            }
            py68_value_release(runtime, index_val);
            py68_value_release(runtime, container_val);
            ip += 1;
            break;
        }
        case OP_LOAD_SLICE: {
            Py68Value end_val, start_val, container_val;
            int start_omitted;
            int end_omitted;
            status = py68_vm_pop(runtime, &end_val);
            if (status == PY68_STATUS_OK)
                status = py68_vm_pop(runtime, &start_val);
            if (status == PY68_STATUS_OK)
                status = py68_vm_pop(runtime, &container_val);
            if (status != PY68_STATUS_OK) break;
            start_omitted = start_val.type == PY68_VALUE_NONE;
            end_omitted = end_val.type == PY68_VALUE_NONE;
            if ((!start_omitted && start_val.type != PY68_VALUE_INT) ||
                (!end_omitted && end_val.type != PY68_VALUE_INT)) {
                py68_value_release(runtime, end_val);
                py68_value_release(runtime, start_val);
                py68_value_release(runtime, container_val);
                py68_vm_error(runtime, PY68_ERROR_TYPE,
                              "slice bounds must be integers or None");
                status = PY68_STATUS_RUNTIME_ERROR;
                ip = current_code->bytecode_length;
                break;
            }
            if (container_val.type == PY68_VALUE_OBJECT &&
                container_val.as.object != NULL &&
                container_val.as.object->type == PY68_OBJECT_STRING) {
                Py68String *sliced;
                status = py68_string_slice(
                    runtime, (Py68String *)container_val.as.object,
                    start_omitted ? 0 : start_val.as.integer,
                    end_omitted ? 0 : end_val.as.integer, start_omitted,
                    end_omitted, &sliced);
                if (status == PY68_STATUS_OK)
                    status = py68_vm_push(
                        runtime, py68_value_from_object(&sliced->base));
            } else if (container_val.type == PY68_VALUE_OBJECT &&
                       container_val.as.object != NULL &&
                       container_val.as.object->type == PY68_OBJECT_LIST) {
                Py68List *sliced;
                status = py68_list_slice(
                    runtime, (Py68List *)container_val.as.object,
                    start_omitted ? 0 : start_val.as.integer,
                    end_omitted ? 0 : end_val.as.integer, start_omitted,
                    end_omitted, &sliced);
                if (status == PY68_STATUS_OK)
                    status = py68_vm_push(
                        runtime, py68_value_from_object(&sliced->base));
            } else {
                py68_vm_error(runtime, PY68_ERROR_TYPE,
                              "container does not support slicing");
                status = PY68_STATUS_RUNTIME_ERROR;
                ip = current_code->bytecode_length;
            }
            py68_value_release(runtime, end_val);
            py68_value_release(runtime, start_val);
            py68_value_release(runtime, container_val);
            if (status != PY68_STATUS_OK &&
                status != PY68_STATUS_RUNTIME_ERROR) {
                py68_vm_error(runtime, PY68_ERROR_MEMORY, "slice failed");
                status = PY68_STATUS_RUNTIME_ERROR;
                ip = current_code->bytecode_length;
            }
            ip += 1;
            break;
        }
        case OP_ADD: case OP_SUBTRACT: case OP_MULTIPLY: case OP_FLOOR_DIVIDE:
        case OP_MODULO: case OP_EQUAL: case OP_NOT_EQUAL: case OP_LESS:
        case OP_LESS_EQUAL: case OP_GREATER: case OP_GREATER_EQUAL:
            status = py68_vm_binary(runtime, opcode); ip += 1; break;
        case OP_NEGATE:
            status = py68_vm_pop(runtime, &value);
            if (status == PY68_STATUS_OK && value.type == PY68_VM_INT) {
                if (value.as.integer == (Py68I32)-2147483647 - 1) {
                    py68_vm_error(runtime, PY68_ERROR_OVERFLOW, "integer overflow");
                    status = PY68_STATUS_RUNTIME_ERROR;
                } else { value.as.integer = -value.as.integer; status = py68_vm_push(runtime, value); }
            } else if (status == PY68_STATUS_OK) {
                py68_vm_error(runtime, PY68_ERROR_TYPE, "integer operand required");
                status = PY68_STATUS_RUNTIME_ERROR;
            }
            ip += 1; break;
        case OP_POSITIVE:
            status = py68_vm_pop(runtime, &value);
            if (status == PY68_STATUS_OK && value.type == PY68_VM_INT)
                status = py68_vm_push(runtime, value);
            else if (status == PY68_STATUS_OK) {
                py68_vm_error(runtime, PY68_ERROR_TYPE, "integer operand required");
                status = PY68_STATUS_RUNTIME_ERROR;
            }
            ip += 1; break;
        case OP_NOT: {
            int truth;
            status = py68_vm_pop(runtime, &value);
            if (status == PY68_STATUS_OK) {
                truth = py68_vm_truth(value);
                py68_value_release(runtime, value);
                status = py68_vm_push(runtime, py68_value_bool(!truth));
            }
            ip += 1; break;
        }
        case OP_JUMP:
            target = (Py68U32)((Py68I32)(ip + 3) +
                (Py68I32)(Py68I16)(((Py68U16)current_code->bytecode[ip + 1] << 8) |
                                   current_code->bytecode[ip + 2]));
            ip = target; status = PY68_STATUS_OK; break;
        case OP_JUMP_IF_FALSE: case OP_JUMP_IF_TRUE:
            status = py68_vm_pop(runtime, &value);
            if (status == PY68_STATUS_OK &&
                ((opcode == OP_JUMP_IF_FALSE && !py68_vm_truth(value)) ||
                 (opcode == OP_JUMP_IF_TRUE && py68_vm_truth(value)))) {
                ip = (Py68U32)((Py68I32)(ip + 3) +
                    (Py68I32)(Py68I16)(((Py68U16)current_code->bytecode[ip + 1] << 8) |
                                       current_code->bytecode[ip + 2]));
            } else ip += 3;
            if (status == PY68_STATUS_OK) py68_value_release(runtime, value);
            break;
        case OP_JUMP_IF_FALSE_OR_POP: case OP_JUMP_IF_TRUE_OR_POP:
            if (runtime->value_stack_count == 0) {
                py68_vm_error(runtime, PY68_ERROR_BYTECODE, "value stack underflow");
                status = PY68_STATUS_RUNTIME_ERROR;
                break;
            }
            value = runtime->value_stack[runtime->value_stack_count - 1];
            if ((opcode == OP_JUMP_IF_FALSE_OR_POP && !py68_vm_truth(value)) ||
                (opcode == OP_JUMP_IF_TRUE_OR_POP && py68_vm_truth(value))) {
                ip = (Py68U32)((Py68I32)(ip + 3) +
                    (Py68I32)(Py68I16)(((Py68U16)current_code->bytecode[ip + 1] << 8) |
                                       current_code->bytecode[ip + 2]));
            } else {
                --runtime->value_stack_count;
                py68_value_release(runtime, value);
                ip += 3;
            }
            status = PY68_STATUS_OK;
            break;
        case OP_CALL: {
            Py68U8 argument_count = current_code->bytecode[ip + 1];
            Py68Value callee;
            Py68Value result;
            Py68Value *arguments;
            Py68U16 argument_index;
            if (runtime->value_stack_count < (Py68U16)argument_count + 1) {
                py68_vm_error(runtime, PY68_ERROR_BYTECODE,
                              "call stack underflow");
                status = PY68_STATUS_RUNTIME_ERROR;
                ip = current_code->bytecode_length;
                break;
            }
            arguments = &runtime->value_stack[
                runtime->value_stack_count - argument_count];
            callee = runtime->value_stack[
                runtime->value_stack_count - argument_count - 1];
            if (callee.type != PY68_VALUE_OBJECT || callee.as.object == NULL) {
                py68_vm_error(runtime, PY68_ERROR_TYPE, "callable required");
                status = PY68_STATUS_RUNTIME_ERROR;
                ip = current_code->bytecode_length;
                break;
            }
            if (callee.as.object->type == PY68_OBJECT_NATIVE_FUNCTION) {
                status = py68_native_call(
                    (Py68NativeFunction *)callee.as.object, runtime,
                    argument_count, arguments, &result);
            } else if (callee.as.object->type == PY68_OBJECT_FUNCTION) {
                Py68Function *function = (Py68Function *)callee.as.object;
                Py68Code *callee_code = function->code;
                Py68U16 local_count = function->local_count;
                status = py68_function_check_arguments(function, argument_count);
                if (status == PY68_STATUS_OK)
                    status = py68_verify_code(callee_code, &runtime->error);
                if (status == PY68_STATUS_OK)
                    status = py68_frame_push(runtime, callee_code,
                                             current_code, local_count,
                                             argument_count, arguments, ip + 2);
                if (status == PY68_STATUS_OK) {
                    runtime->value_stack_count = (Py68U16)(
                        runtime->value_stack_count - argument_count - 1);
                    py68_value_release(runtime, callee);
                    while (argument_count != 0) {
                        --argument_count;
                        py68_value_release(runtime, arguments[argument_count]);
                    }
                    current_code = callee_code;
                    ip = 0;
                }
                break;
            } else {
                py68_vm_error(runtime, PY68_ERROR_TYPE, "unsupported callable");
                status = PY68_STATUS_RUNTIME_ERROR;
            }
            if (status == PY68_STATUS_OK) {
                runtime->value_stack_count = (Py68U16)(
                    runtime->value_stack_count - argument_count - 1);
                py68_value_release(runtime, callee);
                for (argument_index = 0; argument_index < argument_count;
                     ++argument_index)
                    py68_value_release(runtime, arguments[argument_index]);
                status = py68_vm_push(runtime, result);
            }
            if (status != PY68_STATUS_OK) {
                ip = current_code->bytecode_length;
                break;
            }
            ip += 2;
            break;
        }
        case OP_RETURN_VALUE: case OP_RETURN_NONE: {
            Py68Value return_value;
            Py68Code *caller_code;
            Py68U32 caller_ip;
            if (runtime->frame_count <= 1) {
                py68_vm_error(runtime, PY68_ERROR_BYTECODE,
                              "return outside call frame");
                status = PY68_STATUS_RUNTIME_ERROR;
                break;
            }
            if (opcode == OP_RETURN_VALUE) {
                status = py68_vm_pop(runtime, &return_value);
                if (status != PY68_STATUS_OK) break;
            } else {
                return_value = py68_value_none();
            }
            caller_code = runtime->frames[runtime->frame_count - 1].return_code;
            caller_ip = runtime->frames[runtime->frame_count - 1].return_ip;
            py68_frame_pop(runtime);
            current_code = caller_code;
            ip = caller_ip;
            status = py68_vm_push(runtime, return_value);
            break;
        }
        default:
            py68_vm_error(runtime, PY68_ERROR_BYTECODE, "opcode not implemented");
            status = PY68_STATUS_RUNTIME_ERROR; ip = current_code->bytecode_length; break;
        }
        if (status != PY68_STATUS_OK) {
            if (status != PY68_STATUS_EXIT)
                py68_vm_fail(runtime, current_code, ip);
            else {
                py68_vm_clear_stack(runtime);
                py68_frame_unwind(runtime);
            }
            return status;
        }
    }
    py68_vm_clear_stack(runtime);
    py68_frame_unwind(runtime);
    py68_vm_error(runtime, PY68_ERROR_BYTECODE, "VM fell off bytecode");
    return PY68_STATUS_RUNTIME_ERROR;
}
