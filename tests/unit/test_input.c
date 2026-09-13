/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_builtin.h"
#include "py68k_runtime.h"
#include "py68k_string.h"

#include <stdio.h>
#include <string.h>

static int expect(int condition, const char *label, int *passed)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", label);
        *passed = 0;
        return 0;
    }
    return 1;
}

int main(void)
{
    Py68Runtime runtime;
    Py68Value result;
    Py68Value args[1];
    Py68String *prompt;
    FILE *saved;
    int passed = 1;
    const char *stdin_path = "build/host/test_input_stdin.txt";
    FILE *in;

    expect(py68_runtime_initialize(&runtime) == PY68_STATUS_OK, "init",
           &passed);

    in = fopen(stdin_path, "wb");
    expect(in != NULL, "create stdin file", &passed);
    if (in != NULL) {
        fputs("Alice\n", in);
        fclose(in);
    }

    saved = freopen(stdin_path, "rb", stdin);
    expect(saved != NULL, "freopen stdin", &passed);

    expect(py68_string_new_copy(&runtime, "Name: ", 6, &prompt) ==
               PY68_STATUS_OK,
           "prompt", &passed);
    args[0] = py68_value_from_object(&prompt->base);
    expect(py68_builtin_input(&runtime, 1, args, &result) == PY68_STATUS_OK,
           "input", &passed);
    expect(result.type == PY68_VALUE_OBJECT &&
               ((Py68String *)result.as.object)->length == 5 &&
               memcmp(((Py68String *)result.as.object)->data, "Alice", 5) == 0,
           "input value", &passed);
    py68_value_release(&runtime, result);
    py68_object_release(&runtime, &prompt->base);

    /* EOF on next input */
    expect(py68_builtin_input(&runtime, 0, NULL, &result) != PY68_STATUS_OK,
           "input EOF", &passed);

    freopen("/dev/null", "r", stdin);
    remove(stdin_path);
    py68_runtime_shutdown(&runtime);
    expect(runtime.allocator.stats.current_bytes == 0, "no leak", &passed);

    if (passed) {
        printf("PASS: input builtin tests\n");
        return 0;
    }
    return 1;
}
