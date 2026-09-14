/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_runtime.h"
#include "py68k_value.h"

#include <stdio.h>

int main(void)
{
    Py68Value a;
    Py68Value b;
    int passed = 1;

    a = py68_value_float_bits(0x3FC00000u); /* 1.5 */
    b = py68_value_int(2);
    passed &= py68_value_is_number(a);
    passed &= py68_value_float_bits_get(a) == 0x3FC00000u;
    passed &= py68_value_equal(NULL, a, py68_value_float_bits(0x3FC00000u));
    passed &= py68_value_equal(NULL, b, py68_value_float_bits(0x40000000u));
    passed &= py68_value_truthy(a);
    passed &= !py68_value_truthy(py68_value_float_bits(0));
    if (passed) { puts("PASS: float tests"); return 0; }
    return 1;
}
