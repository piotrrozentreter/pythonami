# Memory Model

All interpreter-owned allocations go through the tagged allocator in `src/memory.c`. Phase 0 tracks current bytes, peak bytes, allocation/free counts, failures, and bytes by tag. Runtime shutdown must return current bytes to zero.

There is **no tracing garbage collector**. Heap objects (strings, lists, tuples, dicts, sets, functions, modules, exceptions, files, …) are **reference-counted**, linked from `Py68Runtime.live_objects`, and freed when the count reaches zero (or force-freed during shutdown). Scalars (int/float/bool/None) are immediates and are not refcounted.

**Scope exit:** popping a call frame releases every local slot (`py68_frame_pop`). A large local list is therefore freed on function return if nothing else retains it (return value, global, or another live container). Rebinding a name (`x = None`) also releases the previous value. There is no `del` statement; mid-function early release uses rebinding. Container cycles are **rejected** on insert (`ValueError`) rather than collected.

See also `docs/ownership.md` and brief §22 / D-0016.
