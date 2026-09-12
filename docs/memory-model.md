# Memory Model

All interpreter-owned allocations go through the tagged allocator in `src/memory.c`. Phase 0 tracks current bytes, peak bytes, allocation/free counts, failures, and bytes by tag. Runtime shutdown must return current bytes to zero. Heap strings and lists are reference-counted, linked from `Py68Runtime.live_objects`, and freed only when their count reaches zero or during shutdown cleanup. List element storage is separately tracked and released after child values.
