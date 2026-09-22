# Debug Statistics

Run `pythonami` with `--debug` to emit a statistics report to stderr after a
source execution. Script output remains on stdout. The report is labeled
`top-level source`: source-derived fields describe the top-level source being
executed, not imported modules.

The report is emitted after verification and, when applicable, VM execution,
but before the temporary source, token, AST, and bytecode structures are
destroyed. Therefore `memory_current_bytes` and `live_object_count` are useful
execution snapshots, not expected-to-be-zero shutdown checks. The allocator
must still reach zero during normal runtime shutdown; unit tests check that
separately.

## Fields

| Field | Meaning |
| --- | --- |
| `memory_current_bytes` | Bytes currently tracked by the tagged allocator when the report is emitted. |
| `memory_peak_bytes` | Highest tracked byte total reached during the run. |
| `allocation_count` | Number of successful allocator allocation operations so far, including allocations made while setting up and executing the run. |
| `free_count` | Number of allocator free operations so far. This is an operation count, not a byte count. |
| `failed_count` | Number of allocation or reallocation requests rejected by the allocator. A nonzero value can come from an injected test failure or a resource limit; it does not by itself identify the failing operation. |
| `global_count` | Number of slots in the runtime global-name table. |
| `builtin_count` | Number of builtin entries installed in the runtime registry. |
| `source_file_count` | Number of source files represented by this report. For the current top-level report this is normally `1`; it is `0` when source loading fails before a source object exists. |
| `source_bytes` | Byte length of the top-level source text. |
| `source_lines` | Number of lines calculated from the top-level source text. |
| `token_count` | Number of tokens produced by tokenization of the top-level source. |
| `bytecode_bytes` | Length in bytes of the compiled top-level bytecode instruction stream. |
| `maximum_stack` | Maximum value-stack depth required by the verified top-level bytecode. |
| `frame_count` | Number of active VM call frames when the report is emitted. A completed top-level run normally reports `0`. |
| `value_stack_count` | Number of values on the VM value stack when the report is emitted. A completed top-level run normally reports `0`. |
| `live_object_count` | Number of reference-counted heap objects still linked in the runtime live-object list at report time. Immediate scalar values are not included. |

## Reading the example

```text
memory_current_bytes=159999
memory_peak_bytes=229284
allocation_count=5786
free_count=4498
failed_count=3
global_count=22
builtin_count=67
source_file_count=1
source_bytes=4985
source_lines=221
token_count=1290
bytecode_bytes=217
maximum_stack=3
frame_count=0
value_stack_count=0
live_object_count=113
```

This indicates that the run compiled one 4,985-byte, 221-line top-level
source file into 1,290 tokens and 217 bytes of bytecode. Its verified code
needed at most three value-stack entries. The VM had finished its active
frames and value-stack work when the report was taken, while 113 reference-
counted objects and 159,999 allocator-tracked bytes were still live in the
runtime snapshot. The run reached a 229,284-byte high-water mark.

The allocator recorded 5,786 successful allocation operations and 4,498 free
operations. Those totals describe allocator activity, not the number of live
objects or the amount of memory still allocated. Three requests were rejected
and should be investigated alongside the program result and any configured
allocator limit or failure injection. The runtime had 22 global slots and 67
builtin entries at report time.

## Validation

The focused host check verifies that `--debug` writes only to stderr, is
deterministic for repeated execution, remains present after a runtime failure,
and includes every field above:

```text
make -f Makefile.host debug-stats-test
```

Normal execution and `--debug -V` do not emit this report.