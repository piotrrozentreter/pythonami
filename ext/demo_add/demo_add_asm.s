; 2026 by Piotr Rozentreter (Rozsoft)
;
; Motorola 68000 assembler export for demo_add.py68k.
; Implements mul(a, b) with the Py68ExtCallback / vbcc +aos68k C ABI:
;   4(sp)  = runtime (pointer)
;   8(sp)  = argument_count (32-bit slot)
;  12(sp)  = arguments (Py68Value *)
;  16(sp)  = result (Py68Value *)
;
; Py68Value layout: type.u16 @0, reserved.u16 @2, payload.i32 @4 (8 bytes).
; PY68_VALUE_BOOL=1, PY68_VALUE_INT=2
; PY68_STATUS_OK=0, PY68_STATUS_RUNTIME_ERROR=11

        section "CODE",code

        xdef    _demo_mul

_demo_mul:
        move.l  12(sp),a0           ; arguments
        move.l  16(sp),a1           ; result

        move.w  (a0),d0             ; arg0.type
        cmp.w   #1,d0
        beq.s   .arg0_ok
        cmp.w   #2,d0
        bne.s   .type_error
.arg0_ok:
        move.w  8(a0),d0            ; arg1.type
        cmp.w   #1,d0
        beq.s   .arg1_ok
        cmp.w   #2,d0
        bne.s   .type_error
.arg1_ok:
        move.l  4(a0),d0            ; left
        move.l  12(a0),d1           ; right
        bsr.s   .mul32
        move.w  #2,(a1)             ; PY68_VALUE_INT
        clr.w   2(a1)
        move.l  d0,4(a1)
        moveq   #0,d0               ; PY68_STATUS_OK
        rts

.type_error:
        moveq   #11,d0              ; PY68_STATUS_RUNTIME_ERROR
        rts

; d0*d1 -> d0 (32x32 truncating product via shift-add)
.mul32:
        movem.l d2-d4,-(sp)
        move.l  d0,d2
        move.l  d1,d3
        moveq   #0,d0
        moveq   #31,d4
.mul_loop:
        lsr.l   #1,d3
        bcc.s   .mul_skip
        add.l   d2,d0
.mul_skip:
        add.l   d2,d2
        dbf     d4,.mul_loop
        movem.l (sp)+,d2-d4
        rts
