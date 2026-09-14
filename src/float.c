/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_float.h"

#include <stddef.h>

#define PY68_F32_SIGN 0x80000000u
#define PY68_F32_EXP 0x7F800000u
#define PY68_F32_FRAC 0x007FFFFFu
#define PY68_F32_HIDDEN 0x00800000u

int py68_f32_is_finite(Py68U32 bits)
{
    return ((bits & PY68_F32_EXP) != PY68_F32_EXP);
}

int py68_f32_is_zero(Py68U32 bits)
{
    return (bits & 0x7FFFFFFFu) == 0;
}

Py68U32 py68_f32_negate(Py68U32 bits)
{
    return bits ^ PY68_F32_SIGN;
}

static Py68U32 py68_f32_clz32(Py68U32 value)
{
    Py68U32 count = 0;
    if (value == 0) return 32;
    if ((value & 0xFFFF0000u) == 0) { count += 16; value <<= 16; }
    if ((value & 0xFF000000u) == 0) { count += 8; value <<= 8; }
    if ((value & 0xF0000000u) == 0) { count += 4; value <<= 4; }
    if ((value & 0xC0000000u) == 0) { count += 2; value <<= 2; }
    if ((value & 0x80000000u) == 0) { count += 1; }
    return count;
}

static Py68U32 py68_f32_pack(Py68U32 sign, Py68I32 exp, Py68U32 sig)
{
    if (exp >= 255) return sign | PY68_F32_EXP;
    if (exp <= 0) {
        if (exp < -24) return sign;
        sig >>= (Py68U32)(1 - exp);
        return sign | (sig & PY68_F32_FRAC);
    }
    return sign | ((Py68U32)exp << 23) | (sig & PY68_F32_FRAC);
}

static void py68_f32_unpack(Py68U32 bits, Py68U32 *sign, Py68I32 *exp,
                            Py68U32 *sig)
{
    *sign = bits & PY68_F32_SIGN;
    *exp = (Py68I32)((bits & PY68_F32_EXP) >> 23);
    *sig = bits & PY68_F32_FRAC;
    if (*exp == 0) {
        if (*sig != 0) {
            Py68U32 shift = py68_f32_clz32(*sig) - 8;
            *sig <<= shift;
            *exp = 1 - (Py68I32)shift;
        }
    } else {
        *sig |= PY68_F32_HIDDEN;
    }
}

Py68U32 py68_f32_from_i32(Py68I32 value)
{
    Py68U32 sign = 0;
    Py68U32 mag;
    Py68U32 shift;
    Py68I32 exp;
    if (value == 0) return 0;
    if (value < 0) {
        sign = PY68_F32_SIGN;
        mag = (Py68U32)(-(value + 1)) + 1u;
    } else {
        mag = (Py68U32)value;
    }
    shift = 31u - py68_f32_clz32(mag);
    exp = (Py68I32)shift + 127;
    if (shift >= 23)
        mag >>= (shift - 23);
    else
        mag <<= (23 - shift);
    return py68_f32_pack(sign, exp, mag);
}

int py68_f32_to_i32_trunc(Py68U32 bits, Py68I32 *out)
{
    Py68U32 sign;
    Py68I32 exp;
    Py68U32 sig;
    Py68U32 mag;
    py68_f32_unpack(bits, &sign, &exp, &sig);
    if (py68_f32_is_zero(bits)) {
        *out = 0;
        return 1;
    }
    if (!py68_f32_is_finite(bits)) return 0;
    exp = exp - 127;
    if (exp < 0) {
        *out = 0;
        return 1;
    }
    if (exp >= 31) return 0;
    if (exp >= 23)
        mag = sig << (Py68U32)(exp - 23);
    else
        mag = sig >> (Py68U32)(23 - exp);
    if (sign) {
        if (mag > 2147483648u) return 0;
        *out = mag == 2147483648u ? (Py68I32)-2147483647 - 1
                                  : -(Py68I32)mag;
    } else {
        if (mag > 2147483647u) return 0;
        *out = (Py68I32)mag;
    }
    return 1;
}

static void py68_umul32(Py68U32 a, Py68U32 b, Py68U32 *hi, Py68U32 *lo)
{
    Py68U32 a0 = a & 0xFFFFu;
    Py68U32 a1 = a >> 16;
    Py68U32 b0 = b & 0xFFFFu;
    Py68U32 b1 = b >> 16;
    Py68U32 p0 = a0 * b0;
    Py68U32 p1 = a0 * b1;
    Py68U32 p2 = a1 * b0;
    Py68U32 p3 = a1 * b1;
    Py68U32 mid = (p0 >> 16) + (p1 & 0xFFFFu) + (p2 & 0xFFFFu);
    *lo = (p0 & 0xFFFFu) | (mid << 16);
    *hi = p3 + (p1 >> 16) + (p2 >> 16) + (mid >> 16);
}

Py68U32 py68_f32_add(Py68U32 left, Py68U32 right)
{
    Py68U32 sign_a, sign_b, sig_a, sig_b, sig;
    Py68I32 exp_a, exp_b, exp, shift;
    Py68U32 sign;
    if (py68_f32_is_zero(left)) return py68_f32_is_zero(right) ? (left & right)
                                                               : right;
    if (py68_f32_is_zero(right)) return left;
    if (!py68_f32_is_finite(left) || !py68_f32_is_finite(right))
        return PY68_F32_EXP;
    py68_f32_unpack(left, &sign_a, &exp_a, &sig_a);
    py68_f32_unpack(right, &sign_b, &exp_b, &sig_b);
    if (exp_a < exp_b) {
        Py68U32 ts = sign_a; sign_a = sign_b; sign_b = ts;
        ts = sig_a; sig_a = sig_b; sig_b = ts;
        shift = exp_a; exp_a = exp_b; exp_b = shift;
    }
    shift = exp_a - exp_b;
    exp = exp_a;
    sign = sign_a;
    if (shift > 24) return left;
    sig_a <<= 3;
    sig_b <<= 3;
    if (shift != 0) sig_b >>= (Py68U32)shift;
    if (sign_a == sign_b) {
        sig = sig_a + sig_b;
        if (sig >= (PY68_F32_HIDDEN << 4)) {
            sig >>= 1;
            ++exp;
        }
        sig >>= 3;
    } else {
        if (sig_a >= sig_b) {
            sig = sig_a - sig_b;
        } else {
            sig = sig_b - sig_a;
            sign = sign_b;
        }
        if (sig == 0) return 0;
        while ((sig & (PY68_F32_HIDDEN << 3)) == 0) {
            sig <<= 1;
            --exp;
        }
        sig >>= 3;
    }
    return py68_f32_pack(sign, exp, sig);
}

Py68U32 py68_f32_sub(Py68U32 left, Py68U32 right)
{
    return py68_f32_add(left, py68_f32_negate(right));
}

Py68U32 py68_f32_mul(Py68U32 left, Py68U32 right)
{
    Py68U32 sign_a, sign_b, sig_a, sig_b, hi, lo, sig;
    Py68I32 exp_a, exp_b, exp;
    if (py68_f32_is_zero(left) || py68_f32_is_zero(right))
        return (left ^ right) & PY68_F32_SIGN;
    if (!py68_f32_is_finite(left) || !py68_f32_is_finite(right))
        return PY68_F32_EXP;
    py68_f32_unpack(left, &sign_a, &exp_a, &sig_a);
    py68_f32_unpack(right, &sign_b, &exp_b, &sig_b);
    exp = exp_a + exp_b - 127;
    py68_umul32(sig_a, sig_b, &hi, &lo);
    /* 24x24 product sits in bits 47..0; hidden*hidden is bit 46. */
    if (hi & 0x8000u) {
        sig = (hi << 8) | (lo >> 24);
        ++exp;
    } else {
        sig = (hi << 9) | (lo >> 23);
    }
    return py68_f32_pack(sign_a ^ sign_b, exp, sig);
}

int py68_f32_div(Py68U32 left, Py68U32 right, Py68U32 *out)
{
    Py68U32 sign_a, sign_b, sig_a, sig_b, quot, rem;
    Py68I32 exp_a, exp_b, exp;
    Py68U32 bit;
    if (py68_f32_is_zero(right)) return 1;
    if (py68_f32_is_zero(left)) {
        *out = (left ^ right) & PY68_F32_SIGN;
        return 0;
    }
    if (!py68_f32_is_finite(left) || !py68_f32_is_finite(right)) {
        *out = PY68_F32_EXP;
        return 2;
    }
    py68_f32_unpack(left, &sign_a, &exp_a, &sig_a);
    py68_f32_unpack(right, &sign_b, &exp_b, &sig_b);
    exp = exp_a - exp_b + 127;
    rem = sig_a;
    quot = 0;
    for (bit = 0; bit < 25; ++bit) {
        quot <<= 1;
        if (rem >= sig_b) {
            rem -= sig_b;
            quot |= 1;
        }
        rem <<= 1;
    }
    if ((quot & 0x1000000u) != 0) {
        quot >>= 1;
    } else {
        --exp;
        quot &= 0xFFFFFFu;
    }
    *out = py68_f32_pack(sign_a ^ sign_b, exp, quot);
    return 2 - (py68_f32_is_finite(*out) ? 2 : 0);
}

int py68_f32_compare(Py68U32 left, Py68U32 right)
{
    Py68U32 a = left;
    Py68U32 b = right;
    if (!py68_f32_is_finite(left) || !py68_f32_is_finite(right)) return 2;
    if (py68_f32_is_zero(left) && py68_f32_is_zero(right)) return 0;
    if ((a & PY68_F32_SIGN) != (b & PY68_F32_SIGN))
        return (a & PY68_F32_SIGN) ? -1 : 1;
    if (a == b) return 0;
    if (a & PY68_F32_SIGN)
        return a > b ? -1 : 1;
    return a < b ? -1 : 1;
}

int py68_f32_equal(Py68U32 left, Py68U32 right)
{
    if (py68_f32_is_zero(left) && py68_f32_is_zero(right)) return 1;
    return left == right;
}

int py68_f32_parse(const char *text, Py68U32 length, Py68U32 *out)
{
    Py68U32 index = 0;
    Py68U32 sign = 0;
    Py68U32 digits = 0;
    Py68U32 digit_count = 0;
    Py68I32 frac_digits = 0;
    Py68I32 exp = 0;
    int saw_dot = 0;
    int saw_digit = 0;
    Py68U32 bits;
    Py68I32 scale;
    if (text == NULL || length == 0) return 0;
    if (text[0] == '+' || text[0] == '-') {
        if (text[0] == '-') sign = PY68_F32_SIGN;
        ++index;
    }
    while (index < length) {
        char character = text[index];
        if (character == '.') {
            if (saw_dot) return 0;
            saw_dot = 1;
            ++index;
            continue;
        }
        if (character == 'e' || character == 'E') break;
        if (character < '0' || character > '9') return 0;
        saw_digit = 1;
        if (digit_count < 9)
            digits = digits * 10u + (Py68U32)(character - '0');
        ++digit_count;
        if (saw_dot) ++frac_digits;
        ++index;
    }
    if (!saw_digit) return 0;
    if (index < length && (text[index] == 'e' || text[index] == 'E')) {
        int exp_sign = 1;
        int exp_digits = 0;
        ++index;
        if (index < length && (text[index] == '+' || text[index] == '-')) {
            if (text[index] == '-') exp_sign = -1;
            ++index;
        }
        while (index < length) {
            if (text[index] < '0' || text[index] > '9') return 0;
            exp = exp * 10 + (text[index] - '0');
            ++exp_digits;
            ++index;
        }
        if (exp_digits == 0) return 0;
        exp *= exp_sign;
    }
    if (index != length) return 0;
    bits = py68_f32_from_i32((Py68I32)digits);
    scale = exp - frac_digits;
    while (scale > 0) {
        bits = py68_f32_mul(bits, PY68_F32_TEN);
        --scale;
        if (!py68_f32_is_finite(bits)) return 0;
    }
    while (scale < 0) {
        if (py68_f32_div(bits, PY68_F32_TEN, &bits) != 0) return 0;
        ++scale;
    }
    *out = bits | sign;
    return py68_f32_is_finite(*out);
}

Py68U32 py68_f32_format(Py68U32 bits, char *buffer, Py68U32 capacity)
{
    Py68U32 length = 0;
    Py68U32 scaled;
    Py68I32 whole;
    Py68U32 frac;
    Py68U32 digits[12];
    Py68U32 count = 0;
    Py68U32 index;
    if (buffer == NULL || capacity == 0) return 0;
    if (!py68_f32_is_finite(bits)) {
        buffer[0] = '\0';
        return 0;
    }
    if (bits & PY68_F32_SIGN) {
        if (length + 1 >= capacity) return 0;
        buffer[length++] = '-';
        bits ^= PY68_F32_SIGN;
    }
    if (py68_f32_is_zero(bits)) {
        if (length + 1 >= capacity) return 0;
        buffer[length++] = '0';
        buffer[length] = '\0';
        return length;
    }
    scaled = py68_f32_mul(bits, PY68_F32_MILLION);
    if (!py68_f32_to_i32_trunc(scaled, &whole)) {
        buffer[0] = '\0';
        return 0;
    }
    if (whole < 0) whole = -whole;
    frac = (Py68U32)whole % 1000000u;
    whole = whole / 1000000;
    if (whole == 0) {
        if (length + 1 >= capacity) return 0;
        buffer[length++] = '0';
    } else {
        Py68I32 n = whole;
        while (n != 0 && count < 12) {
            digits[count++] = (Py68U32)(n % 10);
            n /= 10;
        }
        while (count != 0) {
            if (length + 1 >= capacity) return 0;
            buffer[length++] = (char)('0' + digits[--count]);
        }
    }
    if (frac != 0) {
        char frac_digits[6];
        Py68U32 frac_len = 6;
        Py68U32 rest = frac;
        if (length + 1 >= capacity) return 0;
        buffer[length++] = '.';
        for (index = 6; index > 0; --index) {
            frac_digits[index - 1] = (char)('0' + (rest % 10));
            rest /= 10;
        }
        while (frac_len > 1 && frac_digits[frac_len - 1] == '0')
            --frac_len;
        for (index = 0; index < frac_len; ++index) {
            if (length + 1 >= capacity) return 0;
            buffer[length++] = frac_digits[index];
        }
    }
    buffer[length] = '\0';
    return length;
}
