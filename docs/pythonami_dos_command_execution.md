# DOS Command Execution in PythonAmi

## Proposal

This document proposes a Python-compatible way to execute AmigaDOS commands in **PythonAmi**. The public API follows real CPython functions where practical, especially `os.system()`, `subprocess.run()`, `subprocess.Popen()`, and `shutil.which()`, while keeping the first implementation feasible on Motorola 68000 systems with limited memory.

The recommended implementation strategy is incremental:

1. Implement `os.system()` as the smallest synchronous primitive.
2. Implement a restricted `subprocess.run()` on top of an internal process backend.
3. Add output capture through temporary files or DOS pipes.
4. Add a limited `subprocess.Popen()` only after process handles, lifetime, and cleanup are reliable.

---

## 1. Compatibility goals

PythonAmi should preserve familiar Python behavior where the AmigaDOS model permits it.

```python
import os

status = os.system("list SYS:")
if status != 0:
    print("Command failed:", status)
```

```python
import subprocess

result = subprocess.run(
    ["list", "SYS:"],
    capture_output=True,
    text=True,
)

print(result.returncode)
print(result.stdout)
```

Primary goals:

- Familiar APIs for Python programmers.
- Correct argument quoting for AmigaDOS.
- Predictable return-code handling.
- No hidden dependence on Unix `fork()`, POSIX file descriptors, or Windows process handles.
- Operation on a plain M68000 without an MMU.
- Bounded allocations and explicit cleanup on all error paths.

Non-goals for the first version:

- Full CPython `subprocess` compatibility.
- Unix signals and process groups.
- Arbitrary descriptor inheritance.
- Shell pipeline emulation inside PythonAmi.
- Thread-safe concurrent process management.

---

## 2. Proposed public APIs

## 2.1 `os.system(command)`

### Python-compatible signature

```python
os.system(command: str) -> int
```

### Proposed behavior

- Execute `command` through AmigaDOS.
- Display command output through the PythonAmi process's current input and output streams.
- Wait until the command completes.
- Return the AmigaDOS command return code directly.
- Raise `TypeError` if `command` is not a string.
- Raise `ValueError` if `command` contains an embedded NUL character.
- Raise `OSError` if PythonAmi cannot start the command at all.

Unlike POSIX CPython, PythonAmi should **not encode the result as a wait-status word**. Returning the AmigaDOS result directly is more useful and avoids pretending that POSIX wait semantics exist. This intentional difference must be documented.

### Example

```python
import os

rc = os.system('copy "RAM:input file" "RAM:output file"')

if rc == 0:
    print("Copy completed")
elif rc == 5:
    print("Warning")
elif rc == 10:
    print("Error")
else:
    print("DOS return code:", rc)
```

PythonAmi should expose symbolic constants if practical:

```python
os.RETURN_OK       # 0
os.RETURN_WARN     # 5
os.RETURN_ERROR    # 10
os.RETURN_FAIL     # 20
```

Applications should still accept other integer return codes because programs may define their own values.

---

## 2.2 `subprocess.run()`

### Initial supported signature

```python
subprocess.run(
    args,
    *,
    stdin=None,
    stdout=None,
    stderr=None,
    capture_output=False,
    shell=False,
    cwd=None,
    env=None,
    text=False,
    encoding=None,
    errors=None,
    timeout=None,
    check=False,
) -> CompletedProcess
```

Not every parameter needs to be supported in version 1. Unsupported combinations should raise `NotImplementedError`, rather than being silently ignored.

### `CompletedProcess`

```python
class CompletedProcess:
    args: str | list[str]
    returncode: int
    stdout: bytes | str | None
    stderr: bytes | str | None

    def check_returncode(self): ...
```

### Minimum viable behavior

Version 1 should support:

- `args` as a command string.
- `args` as a sequence of strings.
- Synchronous execution.
- `check=True`.
- `capture_output=True`.
- `stdout=subprocess.PIPE`.
- `stderr=subprocess.PIPE` where the OS/backend supports a separate error stream.
- `text=True` using PythonAmi's default text encoding.
- An explicit `encoding` and `errors` policy.
- `cwd` by temporarily changing the child command's current directory without changing PythonAmi's global current directory, if supported by the selected AmigaOS API.

Version 1 may reject:

- `timeout`.
- Custom `env` mappings.
- `stdin=subprocess.PIPE`.
- Asynchronous use.
- `shell=False` when no direct executable-launch backend exists.

### Examples

```python
import subprocess

result = subprocess.run("version", capture_output=True, text=True)
print(result.stdout)
```

```python
import subprocess

subprocess.run(["copy", "RAM:source", "RAM:destination"], check=True)
```

```python
import subprocess

try:
    subprocess.run(["delete", "RAM:missing-file"], check=True)
except subprocess.CalledProcessError as exc:
    print("Command:", exc.cmd)
    print("Return code:", exc.returncode)
```

---

## 2.3 `subprocess.call()` and `subprocess.check_call()`

These can be thin wrappers around `run()`.

```python
def call(args, **kwargs):
    return run(args, **kwargs).returncode


def check_call(args, **kwargs):
    run(args, check=True, **kwargs)
    return 0
```

Proposed signatures:

```python
subprocess.call(args, **kwargs) -> int
subprocess.check_call(args, **kwargs) -> int
```

---

## 2.4 `subprocess.check_output()`

```python
subprocess.check_output(args, *, text=False, encoding=None, errors=None, **kwargs)
```

Conceptual implementation:

```python
def check_output(args, **kwargs):
    if "stdout" in kwargs:
        raise ValueError("stdout argument is not allowed")
    return run(args, stdout=PIPE, check=True, **kwargs).stdout
```

---

## 2.5 Restricted `subprocess.Popen()`

A complete CPython-compatible `Popen` is expensive and should not block the first release.

A later PythonAmi version may support:

```python
process = subprocess.Popen(
    ["type", "RAM:large-file"],
    stdout=subprocess.PIPE,
    text=True,
)

output, errors = process.communicate()
print(process.returncode)
```

Initial restrictions may include:

- No `poll()` unless an AmigaOS process-completion mechanism is installed.
- No `terminate()`, `kill()`, or signal support.
- No simultaneous unbounded stdout and stderr buffering.
- `communicate()` may internally use temporary files.
- Only one active child process per interpreter in the M68000 build.

If true asynchronous execution is not implemented, constructing `Popen` should raise `NotImplementedError`. It should not secretly execute synchronously because that would violate user expectations.

---

## 2.6 `shutil.which()`

A compatible executable lookup helper is valuable independently of `subprocess`.

```python
shutil.which(command, mode=os.F_OK | os.X_OK, path=None) -> str | None
```

For AmigaDOS, the lookup should consider:

1. An explicit path or volume-qualified name in `command`.
2. The current directory.
3. The AmigaDOS command path.
4. The resident command list, if the backend can query it.

Example:

```python
from shutil import which

copy_command = which("Copy")
if copy_command is None:
    print("Copy command not found")
```

Command-name matching should follow the filesystem and DOS rules of the running AmigaOS version. PythonAmi should not impose Unix case sensitivity.

---

## 3. String commands versus argument sequences

Python programmers expect this to be safe:

```python
subprocess.run(["copy", "RAM:my file", "RAM:backup file"])
```

PythonAmi must convert the sequence into a correctly quoted AmigaDOS command line.

### Recommended rule

- With `args: str`, pass the command string through unchanged.
- With `args: sequence[str]`, quote each element using one centralized AmigaDOS quoting routine.
- Reject empty sequences.
- Reject non-string elements.
- Reject embedded NUL characters.

### Internal helper

```python
subprocess.list2cmdline(args) -> str
```

This name exists in CPython, but PythonAmi's implementation should use **AmigaDOS quoting**, not Windows quoting.

### Suggested quoting algorithm

For each argument:

1. If it is empty, emit `""`.
2. If it contains whitespace or AmigaDOS-special characters, wrap it in double quotes.
3. Escape embedded double quotes according to the actual AmigaDOS command-line rules supported by the target OS.
4. Preserve ordinary characters exactly.
5. Detect overflow while calculating the output length.

Do not concatenate command strings with untrusted input. Prefer a sequence:

```python
# Preferred
subprocess.run(["type", user_selected_file])

# Unsafe if the value contains command syntax
subprocess.run("type " + user_selected_file)
```

The sequence form prevents accidental token splitting, but if the final command still passes through a command interpreter, PythonAmi must clearly document that it is not a complete security boundary against every shell metacharacter.

---

## 4. `shell` semantics

CPython uses `shell=False` by default. PythonAmi should preserve the parameter but define it precisely.

### `shell=True`

- The command is interpreted by AmigaDOS.
- Redirection, command separators, aliases, and resident commands may be available according to the host environment.
- `args` should preferably be a string.

### `shell=False`

Preferred long-term behavior:

- Resolve an executable or command explicitly.
- Start it without allowing command separators or shell redirection to be reinterpreted.
- Pass a constructed argument string to the executable using the native process API.

If the initial backend can only invoke the DOS command interpreter, PythonAmi should either:

- support only `shell=True`, or
- document that sequence arguments are quoted but still interpreted by DOS.

It must not claim true `shell=False` isolation unless the backend actually bypasses shell parsing.

---

## 5. Output redirection and capture

`capture_output=True` is equivalent to:

```python
stdout=subprocess.PIPE
stderr=subprocess.PIPE
```

### Recommended M68000 implementation

Use temporary files for the first implementation:

1. Create unique files in `T:` or `RAM:T/`.
2. Open them through AmigaDOS.
3. attach the handles as child output and error output;
4. run the command synchronously;
5. seek/read the captured output after completion;
6. close and delete all temporary files in one cleanup path.

Advantages:

- Avoids pipe deadlocks.
- Avoids requiring a reader task.
- Keeps the implementation synchronous.
- Works with output larger than available memory until the final read.

Disadvantages:

- Slower than pipes.
- Requires writable temporary storage.
- Captured output still needs a size policy before conversion to a Python object.

### Capture limit

PythonAmi should support an implementation limit to avoid exhausting memory:

```python
subprocess.MAX_CAPTURE_SIZE
```

The default should be selected for the target build, not copied from desktop CPython. If output exceeds the limit, recommended behavior is:

- terminate reading;
- complete child cleanup;
- raise `subprocess.OutputLimitExceeded`;
- include the partial captured output if memory permits.

Silently truncating output is not recommended.

### Combining stderr and stdout

Support the real Python idiom:

```python
result = subprocess.run(
    ["command"],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
```

If AmigaDOS cannot provide a separate error stream for a selected launch method, PythonAmi should state that `stderr=PIPE` is unavailable there and allow `stderr=STDOUT` as the fallback.

---

## 6. Text and byte modes

Default behavior should match Python:

```python
result.stdout  # bytes when capture is enabled
```

With text mode:

```python
result = subprocess.run(
    ["type", "RAM:readme"],
    capture_output=True,
    text=True,
    encoding="latin-1",
    errors="replace",
)

result.stdout  # str
```

Recommended defaults:

- Binary capture returns `bytes` unchanged.
- `text=True` decodes bytes after the child exits.
- `encoding=None` uses PythonAmi's configured system encoding.
- `errors=None` uses `strict` unless the interpreter defines a different documented default.
- Newline conversion should use the same text-I/O layer as `open(..., text mode)`.

Avoid assuming UTF-8 on classic Amiga systems. The configured encoding may be an Amiga code page or Latin-1-compatible encoding.

---

## 7. Exceptions

Implement the real Python exception structure where practical.

```python
class SubprocessError(Exception):
    pass


class CalledProcessError(SubprocessError):
    def __init__(self, returncode, cmd, output=None, stderr=None):
        self.returncode = returncode
        self.cmd = cmd
        self.output = output
        self.stdout = output
        self.stderr = stderr


class TimeoutExpired(SubprocessError):
    def __init__(self, cmd, timeout, output=None, stderr=None):
        self.cmd = cmd
        self.timeout = timeout
        self.output = output
        self.stdout = output
        self.stderr = stderr
```

Additional PythonAmi-specific exception:

```python
class OutputLimitExceeded(SubprocessError):
    def __init__(self, cmd, limit, output=None, stderr=None):
        self.cmd = cmd
        self.limit = limit
        self.output = output
        self.stdout = output
        self.stderr = stderr
```

Use exceptions consistently:

- Command starts and returns a nonzero code: normal result, unless `check=True`.
- Command cannot be launched: `OSError` or a suitable subclass.
- Executable not found in a true direct-launch path: `FileNotFoundError`.
- Unsupported feature: `NotImplementedError`.
- Invalid parameter combination: `ValueError`.
- Invalid argument type: `TypeError`.

Do not turn every DOS nonzero result into `OSError`; the child did execute in that case.

---

## 8. Native backend architecture

A clean separation will let the same Python layer support multiple AmigaOS targets.

```text
Python modules
    os.system
    subprocess.run
    subprocess.call
    subprocess.check_call
    subprocess.check_output
             |
             v
Internal PythonAmi process API
    _dos_exec(command, options)
    _dos_wait(process)
    _dos_close(process)
             |
             v
AmigaOS backend
    DOS command execution
    process creation
    BPTR/file-handle conversion
    current directory handling
    result and I/O-error retrieval
```

### Internal request structure

Conceptual C structure:

```c
struct PyAmiExecRequest {
    const char *command;
    BPTR input;
    BPTR output;
    BPTR error_output;
    BPTR current_dir;
    unsigned long stack_size;
    unsigned long flags;
};

struct PyAmiExecResult {
    long return_code;
    long io_error;
    unsigned long flags;
};
```

Keep native Amiga types inside the backend. Python-visible code should not manipulate `BPTR`, DOS packets, message ports, or task pointers.

### Conceptual synchronous primitive

```c
int
pyami_dos_execute(const struct PyAmiExecRequest *request,
                  struct PyAmiExecResult *result)
{
    int started = 0;

    /* Validate pointers and command length. */
    /* Acquire or duplicate required DOS handles. */
    /* Invoke selected AmigaDOS execution function. */
    /* Save the command result immediately. */
    /* Save IoErr() immediately where meaningful. */
    /* Release every acquired resource. */
    /* Return whether launch infrastructure succeeded. */

    return started;
}
```

The exact native function should be selected by the PythonAmi target matrix and verified against the relevant AmigaOS SDK. Possible backends may differ between classic AmigaOS releases and compatible systems, so this proposal intentionally keeps the public API independent of one native call.

---

## 9. M68000-specific implementation notes

### 9.1 Stack size

Child commands may require more stack than the PythonAmi default.

Possible extension:

```python
subprocess.run(args, ami_stack_size=16384)
```

This must be a keyword-only PythonAmi extension. Validate:

- integer type;
- positive value;
- alignment required by the backend;
- upper bound that prevents overflow;
- enough room for the native launch mechanism.

A global default may also be exposed:

```python
subprocess.DEFAULT_AMIGA_STACK_SIZE
```

### 9.2 Alignment

On M68000, misaligned word or long access can fault. Native structures, temporary buffers, command-line storage, and message payloads must have suitable alignment. Do not cast arbitrary byte-buffer positions to native structure pointers.

### 9.3 32-bit arithmetic

Length calculations must detect overflow before allocation:

```c
if (arg_len > UINT32_MAX - total_len - quote_overhead) {
    PyErr_SetString(PyExc_OverflowError, "command line is too long");
    return NULL;
}
```

Also enforce the smaller command-length limit imposed by the actual DOS API.

### 9.4 Memory pressure

Avoid constructing multiple full copies of:

- the argument vector;
- the quoted command line;
- captured output;
- decoded text.

Recommended sequence:

1. Compute quoted command length with overflow checks.
2. Allocate once.
3. Write directly into the final buffer.
4. Release the argument-conversion temporaries before launching.
5. Read captured output in chunks.
6. Decode incrementally when feasible.

### 9.5 No MMU assumption

A child process may corrupt shared memory if the OS does not isolate address spaces. PythonAmi must treat native handles and shared structures conservatively and should not expose backend pointers to Python code.

### 9.6 Cache assumptions

The base M68000 has no on-chip instruction or data cache. Optimize primarily for:

- fewer allocations;
- fewer library calls;
- sequential memory access;
- compact code;
- avoiding 32-bit division and multiplication in hot loops.

Do not make the quoting parser cryptic for tiny speed gains. Correctness and command safety are more important.

---

## 10. Current-directory handling

The Python parameter is:

```python
subprocess.run(args, cwd="RAM:work")
```

Preferred behavior:

- Lock the target directory.
- Associate that directory with the child process.
- Do not change the PythonAmi process's global current directory.
- Release the lock after the child finishes or launch fails.

If the selected OS backend cannot give the child a distinct current directory, version 1 may temporarily switch directories only in a single-threaded build:

1. Lock the requested directory.
2. Save the existing directory lock.
3. switch to the requested directory;
4. execute synchronously;
5. restore the previous directory in an unconditional cleanup block;
6. unlock the temporary directory.

This fallback must not be used once PythonAmi supports concurrent threads that can access the process-global current directory.

---

## 11. Environment handling

CPython accepts an `env` mapping. AmigaDOS environments do not necessarily map cleanly to a POSIX per-process dictionary.

Recommended staged support:

### Version 1

```python
subprocess.run(args, env=None)
```

- `env=None` inherits the normal DOS environment.
- Any mapping raises `NotImplementedError`.

### Later version

Support a mapping only if the backend can create child-local variables without mutating PythonAmi's own environment.

Never implement `env` by globally changing variables around an asynchronous launch. Even synchronously, global mutation is unsafe if callbacks, interrupts, or other tasks can observe it.

---

## 12. Timeout support

A reliable `timeout` needs:

- asynchronous child launch;
- a timer source;
- a completion message or polling mechanism;
- a defined cancellation strategy;
- cleanup when the child ignores cancellation;
- output-handle cleanup without use-after-free.

Therefore version 1 should reject it explicitly:

```python
subprocess.run(["wait", "10"], timeout=1)
# raises NotImplementedError
```

Do not accept `timeout` and then ignore it.

---

## 13. Return-code policy

Suggested interpretation helpers:

```python
import subprocess

result = subprocess.run(["command"])

if result.returncode == 0:
    print("success")
elif result.returncode < 0:
    print("PythonAmi-reserved termination status")
else:
    print("AmigaDOS/program return code", result.returncode)
```

Reserve negative values only if PythonAmi later needs to represent forced termination. Ordinary DOS command results should remain nonnegative integers.

`check=True` should raise for every nonzero result, matching Python's broad rule, even though AmigaDOS distinguishes warning, error, and failure levels.

PythonAmi may add an optional extension without changing the default:

```python
subprocess.run(args, ami_check_level=10)
```

Meaning: raise only when `returncode >= 10`. This should remain explicitly Amiga-specific and should not replace `check=True`.

---

## 14. Security considerations

### Prefer sequence arguments

```python
subprocess.run(["type", filename])
```

instead of:

```python
subprocess.run("type " + filename, shell=True)
```

### Additional checks

- Reject NUL characters.
- Apply a native command-length limit.
- Quote empty arguments correctly.
- Test quotes, spaces, tabs, colons, slashes, wildcard characters, redirection characters, and command separators.
- Do not search writable directories unexpectedly.
- Do not silently fall back from direct execution to shell execution.
- Never place sensitive values in exception text unless they were already part of the public command representation.

---

## 15. Cleanup and failure handling

Every native execution path should have one cleanup section. Track ownership explicitly for:

- command buffer;
- directory lock;
- input handle;
- output handle;
- error handle;
- temporary filenames;
- temporary-file locks;
- process structure;
- message port;
- child completion message;
- captured byte buffers;
- decoded Python strings.

Conceptual pattern:

```c
int ok = 0;
BPTR temp_out = 0;
BPTR old_dir = 0;
char *command = NULL;

command = build_command(args);
if (command == NULL)
    goto cleanup;

temp_out = open_capture_file();
if (temp_out == 0)
    goto cleanup;

/* Launch and collect result. */
ok = 1;

cleanup:
    if (old_dir != 0)
        restore_current_dir(old_dir);
    if (temp_out != 0)
        Close(temp_out);
    if (command != NULL)
        PyMem_Free(command);
    delete_owned_temp_files();
    return ok;
```

Save the native I/O error immediately after a failing DOS call. Cleanup calls may overwrite the thread/task's error value.

---

## 16. Feature matrix

### Phase 1: minimal and useful

- `os.system(str)`
- Direct AmigaDOS return code
- Inherited console I/O
- NUL and length validation
- Basic `OSError` mapping

### Phase 2: synchronous subprocess

- `subprocess.run()`
- `CompletedProcess`
- Sequence-to-command conversion
- `check=True`
- `call()`
- `check_call()`
- `check_output()`
- `cwd`

### Phase 3: capture

- `PIPE`
- `STDOUT`
- Temporary-file capture
- bytes/text modes
- encoding/error policies
- capture-size limit

### Phase 4: process object

- Restricted `Popen`
- `wait()`
- `communicate()`
- completion messages
- possibly `poll()`

### Phase 5: advanced features

- timeouts;
- stdin pipes;
- true direct launch for `shell=False`;
- controlled environment mappings;
- multiple concurrent children;
- pipe-based streaming.

---

## 17. Reference Python implementation skeleton

This pure-Python layer illustrates how public functions can share one native primitive.

```python
# subprocess.py for PythonAmi

PIPE = -1
STDOUT = -2
DEVNULL = -3


class SubprocessError(Exception):
    pass


class CalledProcessError(SubprocessError):
    def __init__(self, returncode, cmd, output=None, stderr=None):
        self.returncode = returncode
        self.cmd = cmd
        self.output = output
        self.stdout = output
        self.stderr = stderr
        super().__init__(returncode, cmd)

    def __str__(self):
        return "Command %r returned non-zero status %d" % (
            self.cmd,
            self.returncode,
        )


class CompletedProcess:
    def __init__(self, args, returncode, stdout=None, stderr=None):
        self.args = args
        self.returncode = returncode
        self.stdout = stdout
        self.stderr = stderr

    def check_returncode(self):
        if self.returncode:
            raise CalledProcessError(
                self.returncode,
                self.args,
                output=self.stdout,
                stderr=self.stderr,
            )


def run(args, *, stdin=None, stdout=None, stderr=None,
        capture_output=False, shell=False, cwd=None,
        env=None, text=False, encoding=None, errors=None,
        timeout=None, check=False):

    if capture_output:
        if stdout is not None or stderr is not None:
            raise ValueError(
                "stdout and stderr may not be used with capture_output"
            )
        stdout = PIPE
        stderr = PIPE

    if timeout is not None:
        raise NotImplementedError("timeout is not implemented")

    if env is not None:
        raise NotImplementedError("custom env is not implemented")

    command = _normalize_args(args, shell=shell)

    # Native primitive returns raw bytes for captured streams.
    rc, out, err = _amiga_execute(
        command,
        stdin=stdin,
        stdout=stdout,
        stderr=stderr,
        cwd=cwd,
        shell=shell,
    )

    if text:
        selected_encoding = encoding or _default_system_encoding()
        selected_errors = errors or "strict"

        if out is not None:
            out = out.decode(selected_encoding, selected_errors)
        if err is not None:
            err = err.decode(selected_encoding, selected_errors)

    result = CompletedProcess(args, rc, out, err)

    if check:
        result.check_returncode()

    return result


def call(args, **kwargs):
    return run(args, **kwargs).returncode


def check_call(args, **kwargs):
    run(args, check=True, **kwargs)
    return 0


def check_output(args, **kwargs):
    if "stdout" in kwargs:
        raise ValueError("stdout argument is not allowed")
    return run(args, stdout=PIPE, check=True, **kwargs).stdout
```

`_amiga_execute()`, `_normalize_args()`, and `_default_system_encoding()` are PythonAmi internals. They should be implemented once and shared with `os.system()` where possible.

---

## 18. Validation test plan

### Basic execution

```python
assert subprocess.run(["version"]).returncode == 0
assert isinstance(os.system("version"), int)
```

### Arguments

Test:

- no arguments;
- one argument;
- empty argument;
- argument containing one space;
- leading/trailing spaces;
- tabs;
- embedded quote;
- volume name such as `SYS:`;
- path containing spaces;
- wildcard characters;
- shell metacharacters;
- non-ASCII bytes representable in the selected encoding;
- unrepresentable characters;
- embedded NUL;
- maximum accepted command length;
- one byte beyond the limit;
- integer-overflow-sized synthetic lengths where the object model permits testing.

### Return codes

Use helper commands or a PythonAmi test executable that returns:

- 0;
- 5;
- 10;
- 20;
- another positive code;
- the largest representable backend code.

Verify `check=False`, `check=True`, and `CalledProcessError` fields.

### Capture

Test:

- empty stdout;
- empty stderr;
- stdout only;
- stderr only;
- interleaved output with `stderr=STDOUT`;
- output exactly at capture limit;
- output one byte over the limit;
- output larger than a single read buffer;
- binary output containing zero bytes;
- invalid text encoding sequences;
- `errors="strict"`, `replace`, and `ignore` if supported;
- temporary-file open failure;
- read failure;
- cleanup after decode failure.

### Current directory

Test:

- valid directory;
- missing directory;
- path naming a file;
- inaccessible directory;
- child observes requested directory;
- parent directory remains unchanged after success;
- parent directory remains unchanged after launch failure;
- parent directory remains unchanged after a Python exception.

### Resource cleanup

Under memory and handle instrumentation, inject failure after every allocation/open/lock step and verify:

- no leaked memory;
- no leaked DOS handles;
- no leaked locks;
- no remaining temporary files;
- current directory restored;
- original error preserved;
- no double-close or use-after-free.

### M68000-specific checks

- Run on a real or accurately emulated M68000 configuration.
- Use odd-sized command buffers and argument lengths.
- Exercise low-memory conditions.
- Test stack sizes near minimum and maximum accepted values.
- Verify no unaligned word/long accesses.
- Verify length calculation with 32-bit boundaries.
- Repeatedly execute commands to detect gradual memory loss.

---

## 19. Recommended initial contract

For the first production-ready PythonAmi release, implement this exact subset:

```python
os.system(command)

subprocess.run(
    args,
    *,
    stdout=None,
    stderr=None,
    capture_output=False,
    cwd=None,
    text=False,
    encoding=None,
    errors=None,
    check=False,
)

subprocess.call(args, **kwargs)
subprocess.check_call(args, **kwargs)
subprocess.check_output(args, **kwargs)
```

With these explicit restrictions:

- synchronous execution only;
- inherited stdin only;
- temporary-file output capture;
- no timeout;
- no custom environment;
- no `Popen` until asynchronous behavior is real;
- direct AmigaDOS return code;
- documented capture and command-size limits;
- sequence arguments passed through a single tested AmigaDOS quoting function.

This subset gives PythonAmi useful compatibility without forcing a Unix process model onto AmigaDOS or creating a fragile pseudo-`Popen` implementation.
