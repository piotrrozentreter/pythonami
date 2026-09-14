# Bytecode

The stable opcode values are defined in `include/py68k_opcode.h`; metadata is exposed through `py68_opcode_info`. The compiler emits module and function-body bytecode with explicit big-endian operands, scalar/string constants, names, and branch patching. The verifier decodes instruction boundaries, validates indexes and branch targets, propagates stack depth through reachable CFG edges, and records maximum stack depth. The VM executes verified bytecode for integers, booleans, None, binary32 floats, strings, lists, tuples, dicts, sets, bound methods, exceptions, and modules.

Additional Level 0.3–0.5 opcodes (stable numbers; existing 0.1/0.2 opcodes unchanged):

- `OP_TRUE_DIVIDE` (0x15) — true divide, result is float
- `OP_ROT_TWO` (0x0E) — swap TOS and TOS1
- `OP_BUILD_TUPLE` (0x2C), `OP_BUILD_DICT` (0x2D), `OP_BUILD_SET` (0x2E) — same u16-count shape as `OP_BUILD_LIST` (`BUILD_DICT` count is pair count)
- `OP_LOAD_ATTR` (0x32), `OP_STORE_ATTR` (0x33) — u16 name-table index
- `OP_SETUP_TRY` (0x41) — s16 handler offset; `OP_POP_TRY` (0x42); `OP_RAISE` (0x43)
- `OP_CHECK_EXCEPT` (0x46) — s16 jump-if-no-match; stack is exception, matcher
- `OP_IMPORT_NAME` (0x44), `OP_IMPORT_FROM` (0x45) — u16 name-table index

Local variables use `OP_LOAD_LOCAL`/`OP_STORE_LOCAL` with a `u8` slot operand, verified against `code->local_count`. Reading a local before it has been assigned yields a runtime error rather than a stale/garbage value, because unassigned slots are initialized to the `PY68_VALUE_UNBOUND` sentinel by the frame setup code.

Function objects are represented in bytecode as nested code objects: a `Py68Code` may own an array of nested `Py68Code` structures, each referenced from the constant pool as `PY68_CONSTANT_CODE`. `OP_MAKE_FUNCTION` takes a `u16` constant-pool index operand; the verifier checks that the referenced constant is `PY68_CONSTANT_CODE` and that it indexes a valid nested code object before the VM is allowed to execute the instruction. At runtime, `OP_MAKE_FUNCTION` builds a `Py68Function` object bound to the current runtime, which is then callable through the existing `OP_CALL` dispatch alongside native functions.
