# Bytecode

The stable opcode values are defined in `include/py68k_opcode.h`; metadata is exposed through `py68_opcode_info`. Phase 4 emits basic module bytecode with explicit big-endian operands, scalar/string constants, names, and branch patching. The verifier decodes instruction boundaries, validates indexes and branch targets, propagates stack depth through reachable CFG edges, and records maximum stack depth. The scalar VM executes verified integer/boolean/None bytecode with explicit value and frame stacks.

Logical `and` emits `JUMP_IF_FALSE_OR_POP`; logical `or` emits
`JUMP_IF_TRUE_OR_POP`. The jump edge preserves the left operand as the
expression result, while the fall-through edge releases it before compiling
the right operand. The verifier tracks those edges with different stack
depths so short-circuit joins remain valid.
