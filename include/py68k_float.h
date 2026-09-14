/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_FLOAT_H
#define PY68K_FLOAT_H

#include "py68k_types.h"

#define PY68_F32_ZERO 0x00000000u
#define PY68_F32_ONE 0x3F800000u
#define PY68_F32_TEN 0x41200000u
#define PY68_F32_MILLION 0x49742400u

int py68_f32_is_finite(Py68U32 bits);
int py68_f32_is_zero(Py68U32 bits);
Py68U32 py68_f32_negate(Py68U32 bits);
Py68U32 py68_f32_from_i32(Py68I32 value);
int py68_f32_to_i32_trunc(Py68U32 bits, Py68I32 *out);
Py68U32 py68_f32_add(Py68U32 left, Py68U32 right);
Py68U32 py68_f32_sub(Py68U32 left, Py68U32 right);
Py68U32 py68_f32_mul(Py68U32 left, Py68U32 right);
int py68_f32_div(Py68U32 left, Py68U32 right, Py68U32 *out);
int py68_f32_compare(Py68U32 left, Py68U32 right);
int py68_f32_equal(Py68U32 left, Py68U32 right);
int py68_f32_parse(const char *text, Py68U32 length, Py68U32 *out);
Py68U32 py68_f32_format(Py68U32 bits, char *buffer, Py68U32 capacity);

#endif
