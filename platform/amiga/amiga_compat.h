/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_AMIGA_COMPAT_H
#define PY68K_AMIGA_COMPAT_H

/* Minimal ABI declarations; final linking requires an AmigaOS NDK. */
typedef signed long LONG;
typedef unsigned long ULONG;
typedef void *APTR;
typedef unsigned long BPTR;
typedef const void *CONST_APTR;

extern BPTR Output(void);
/* ErrorOutput() is dos.library V47 only; platform_amiga.c uses pr_CES/Output
   fallback on older Kickstarts instead of calling this unconditionally. */
extern BPTR ErrorOutput(void);
extern LONG Write(BPTR file_handle, CONST_APTR buffer, LONG length);

#endif
