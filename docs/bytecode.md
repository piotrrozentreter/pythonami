# Bytecode

The stable opcode values are defined in `include/py68k_opcode.h`; metadata is exposed through `py68_opcode_info`. Phase 4 emits basic module bytecode with explicit big-endian operands, scalar/string constants, names, and branch patching. The verifier decodes instruction boundaries, validates indexes and branch targets, propagates stack depth through reachable CFG edges, and records maximum stack depth. The scalar VM executes verified integer/boolean/None bytecode with explicit value and frame stacks.

Logical `and` emits `JUMP_IF_FALSE_OR_POP`; logical `or` emits
`JUMP_IF_TRUE_OR_POP`. The jump edge preserves the left operand as the
expression result, while the fall-through edge releases it before compiling
the right operand. The verifier tracks those edges with different stack
depths so short-circuit joins remain valid.
Function definitions compile into owned nested code constants. Parameters and
assignment targets receive deterministic local slots before body compilation.
`LOAD_LOCAL` and `STORE_LOCAL` access the active frame, while `MAKE_FUNCTION`
transfers nested-code ownership to the created function object. `STORE_GLOBAL`
binds the function by its interned canonical name.

Before entering a user-function frame, the VM interns that callee's names as
well. This is required for recursive global lookup because a function's name
indexes are local to its own code object.

Function local slots begin in an internal `UNBOUND` state. Reading an
uninitialized local raises a name error; assigning or augmented-assigning the
slot replaces that state with the resulting value.
Source-span reporting for unbound-local runtime errors helps in identifying
the exact location in the code where the error occurred, providing better
debugging information for developers.
Compiled code also retains the source filename as a borrowed reference while
the source buffer remains alive, allowing the error object to identify the
originating file.
Arithmetic runtime failures use the active opcode offset as their diagnostic
span, so errors such as division by zero and integer overflow point back to
the operation that raised them.
Index type, bounds, and container errors use the same active `LOAD_INDEX`
opcode span.
Range validation and callable-type failures use the active `RANGE_INIT`,
`RANGE_NEXT`, or `CALL` instruction span.
The runtime captures the first source-aware VM error's active frames in a
fixed 16-entry traceback buffer without allocating during error handling.
Entries retain borrowed filenames and source locations.
