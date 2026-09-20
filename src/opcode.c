/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_opcode.h"

#define INFO(width, effect, text) { width, effect, text }

const Py68OpcodeInfo *py68_opcode_info(Py68U8 opcode)
{
    static const Py68OpcodeInfo halt = INFO(1, 0, "HALT");
    static const Py68OpcodeInfo pop = INFO(1, -1, "POP");
    static const Py68OpcodeInfo dup = INFO(1, 1, "DUP");
    static const Py68OpcodeInfo load_none = INFO(1, 1, "LOAD_NONE");
    static const Py68OpcodeInfo load_true = INFO(1, 1, "LOAD_TRUE");
    static const Py68OpcodeInfo load_false = INFO(1, 1, "LOAD_FALSE");
    static const Py68OpcodeInfo load_const = INFO(3, 1, "LOAD_CONST");
    static const Py68OpcodeInfo load_global = INFO(3, 1, "LOAD_GLOBAL");
    static const Py68OpcodeInfo store_global = INFO(3, -1, "STORE_GLOBAL");
    static const Py68OpcodeInfo load_local = INFO(3, 1, "LOAD_LOCAL");
    static const Py68OpcodeInfo store_local = INFO(3, -1, "STORE_LOCAL");
    static const Py68OpcodeInfo unary = INFO(1, 0, "UNARY");
    static const Py68OpcodeInfo rot_two = INFO(1, 0, "ROT_TWO");
    static const Py68OpcodeInfo unpack = INFO(3, 0, "UNPACK");
    static const Py68OpcodeInfo binary = INFO(1, -1, "BINARY");
    static const Py68OpcodeInfo jump = INFO(3, 0, "JUMP");
    static const Py68OpcodeInfo jump_pop = INFO(3, -1, "JUMP_POP");
    /* Fall-through pops TOS; the jump path keeps TOS (verifier special-cases). */
    static const Py68OpcodeInfo jump_or_pop = INFO(3, -1, "JUMP_OR_POP");
    static const Py68OpcodeInfo build_list = INFO(3, 0, "BUILD_LIST");
    static const Py68OpcodeInfo load_attr = INFO(3, 0, "LOAD_ATTR");
    static const Py68OpcodeInfo store_attr = INFO(3, -2, "STORE_ATTR");
    static const Py68OpcodeInfo setup_try = INFO(3, 0, "SETUP_TRY");
    static const Py68OpcodeInfo pop_try = INFO(1, 0, "POP_TRY");
    static const Py68OpcodeInfo raise_op = INFO(1, -1, "RAISE");
    static const Py68OpcodeInfo import_name = INFO(3, 1, "IMPORT_NAME");
    static const Py68OpcodeInfo import_from = INFO(3, 1, "IMPORT_FROM");
    static const Py68OpcodeInfo check_except = INFO(3, -1, "CHECK_EXCEPT");
    static const Py68OpcodeInfo load_index = INFO(1, -1, "LOAD_INDEX");
    static const Py68OpcodeInfo store_index = INFO(1, -3, "STORE_INDEX");
    static const Py68OpcodeInfo load_slice = INFO(1, -2, "LOAD_SLICE");
    static const Py68OpcodeInfo list_append = INFO(2, -1, "LIST_APPEND");
    static const Py68OpcodeInfo set_add = INFO(2, -1, "SET_ADD");
    static const Py68OpcodeInfo map_add = INFO(2, -2, "MAP_ADD");
    static const Py68OpcodeInfo range_init = INFO(2, 0, "RANGE_INIT");
    static const Py68OpcodeInfo range_next = INFO(3, 1, "RANGE_NEXT");
    static const Py68OpcodeInfo make_function = INFO(3, 1, "MAKE_FUNCTION");
    static const Py68OpcodeInfo call = INFO(2, 0, "CALL");
    static const Py68OpcodeInfo return_value = INFO(1, -1, "RETURN_VALUE");
    static const Py68OpcodeInfo return_none = INFO(1, 1, "RETURN_NONE");
    static const Py68OpcodeInfo yield_value = INFO(1, -1, "YIELD_VALUE");
    static const Py68OpcodeInfo debug = INFO(1, -1, "PRINT_DEBUG");

    switch (opcode) {
    case OP_HALT: return &halt; case OP_POP: return &pop; case OP_DUP: return &dup;
    case OP_LOAD_NONE: return &load_none; case OP_LOAD_TRUE: return &load_true;
    case OP_LOAD_FALSE: return &load_false; case OP_LOAD_CONST: return &load_const;
    case OP_LOAD_GLOBAL: return &load_global; case OP_STORE_GLOBAL: return &store_global;
    case OP_LOAD_LOCAL: return &load_local; case OP_STORE_LOCAL: return &store_local;
    case OP_NEGATE: case OP_POSITIVE: case OP_NOT: return &unary;
    case OP_ROT_TWO: return &rot_two;
    case OP_UNPACK: return &unpack;
    case OP_ADD: case OP_SUBTRACT: case OP_MULTIPLY: case OP_FLOOR_DIVIDE:
    case OP_MODULO: case OP_TRUE_DIVIDE: case OP_EQUAL: case OP_NOT_EQUAL:
    case OP_LESS:
    case OP_LESS_EQUAL: case OP_GREATER: case OP_GREATER_EQUAL:
    case OP_CONTAINS: case OP_NOT_CONTAINS:
    case OP_IS: case OP_IS_NOT: return &binary;
    case OP_JUMP: return &jump; case OP_JUMP_IF_FALSE: case OP_JUMP_IF_TRUE:
        return &jump_pop;
    case OP_JUMP_IF_FALSE_OR_POP: case OP_JUMP_IF_TRUE_OR_POP:
        return &jump_or_pop;
    case OP_BUILD_LIST: case OP_BUILD_TUPLE: case OP_BUILD_DICT:
    case OP_BUILD_SET: return &build_list;
    case OP_LIST_APPEND: return &list_append;
    case OP_SET_ADD: return &set_add;
    case OP_MAP_ADD: return &map_add;
    case OP_LOAD_ATTR: return &load_attr;
    case OP_STORE_ATTR: return &store_attr;
    case OP_SETUP_TRY: return &setup_try;
    case OP_POP_TRY: return &pop_try;
    case OP_RAISE: return &raise_op;
    case OP_IMPORT_NAME: return &import_name;
    case OP_IMPORT_FROM: return &import_from;
    case OP_CHECK_EXCEPT: return &check_except;
    case OP_LOAD_INDEX: return &load_index;
    case OP_STORE_INDEX: return &store_index; case OP_LOAD_SLICE: return &load_slice;
    case OP_RANGE_INIT: return &range_init; case OP_RANGE_NEXT: return &range_next;
    case OP_MAKE_FUNCTION: return &make_function; case OP_CALL: return &call;
    case OP_RETURN_VALUE: return &return_value; case OP_RETURN_NONE: return &return_none;
    case OP_YIELD_VALUE: return &yield_value;
    case OP_PRINT_DEBUG: return &debug; default: return 0;
    }
}
