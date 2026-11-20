# Ownership

Runtime values distinguish scalar-by-value values from reference-counted objects. `py68_value_retain` and `py68_value_release` operate on object values. Constructors return one owned reference; list copies retain before releasing replaced values; moves invalidate sources only after success. Native callbacks borrow arguments and return one owned result. Frame locals release their references during pop or unwind, and shutdown releases all tracked objects.
