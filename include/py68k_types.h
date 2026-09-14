/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_TYPES_H
#define PY68K_TYPES_H

#if defined(PY68K_AMIGA)
typedef signed long Py68I32;
typedef unsigned long Py68U32;
#else
typedef signed int Py68I32;
typedef unsigned int Py68U32;
#endif
typedef signed short Py68I16;
typedef unsigned short Py68U16;
typedef signed char Py68I8;
typedef unsigned char Py68U8;

typedef char Py68AssertChar8[(sizeof(char) == 1) ? 1 : -1];
typedef char Py68AssertShort16[(sizeof(short) == 2) ? 1 : -1];
typedef char Py68AssertI32[(sizeof(Py68I32) == 4) ? 1 : -1];
typedef char Py68AssertU32[(sizeof(Py68U32) == 4) ? 1 : -1];
typedef char Py68AssertFloat32[(sizeof(float) == 4) ? 1 : -1];

#endif
