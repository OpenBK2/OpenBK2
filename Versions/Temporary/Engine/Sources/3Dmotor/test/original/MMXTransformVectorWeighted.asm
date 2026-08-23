; The original MMX bodies of MMXTransformVector2Impl and MMXTransformVector3Impl,
; lifted out of the __asm blocks in test/original.h so they assemble for x64 too.
;
; These need eight and eleven inputs respectively, well past what either ABI passes
; in registers, so rather than reach past the shadow space for stack arguments they
; take a pointer to SMMXTransformArgs. Every member of that struct is pointer sized,
; so a single SLOT equate covers both architectures and the two bodies below are
; identical apart from the register aliases.
;
;   uint32_t MMXTransformVector2MMX( uint32_t nSrc, const SMMXTransformArgs *pArgs );
;   uint32_t MMXTransformVector3MMX( uint32_t nSrc, const SMMXTransformArgs *pArgs );
;
; Both merge the untouched w byte of nSrc back into the packed result and return it.
; MMXTransformVector.h static_asserts the struct layout the offsets below assume.
;
; As in MMXTransformVector.asm, the two things the inline form got for free are
; handled here: x86 saves the callee-saved registers it uses, and the normalize and
; weight tables arrive through the struct instead of being named as C++ globals.

IFDEF RAX
    SLOT     EQU 8
    ARGS     TEXTEQU <rdx>
    SCR      TEXTEQU <r8>
    SCR2     TEXTEQU <r9>
    WREG     TEXTEQU <r10d>
    IDX      TEXTEQU <rcx>
    IDXD     TEXTEQU <ecx>
ELSE
    SLOT     EQU 4
    ARGS     TEXTEQU <esi>
    SCR      TEXTEQU <edi>
    SCR2     TEXTEQU <ebx>
    WREG     TEXTEQU <edx>
    IDX      TEXTEQU <ecx>
    IDXD     TEXTEQU <ecx>
ENDIF

TRANS0    EQU 0 * SLOT
TRANS1    EQU 1 * SLOT
TRANS2    EQU 2 * SLOT
FIXUPS    EQU 3 * SLOT
WEIGHTTAB EQU 4 * SLOT
NORMTAB   EQU 5 * SLOT
WEIGHT0   EQU 6 * SLOT
WEIGHT1   EQU 7 * SLOT
WEIGHT2   EQU 8 * SLOT

.code

; Unpack nSrc into mm0 with the normal fixup applied, keeping w in WREG.
PREPARE_SOURCE MACRO
    mov WREG, eax
    and eax, 0FFFFFFh
    and WREG, 0FF000000h
    movd mm7, eax
    mov SCR, [ARGS + FIXUPS]
    pxor mm0, mm0
    punpcklbw mm0, mm7
    psubw mm0, [SCR]
ENDM

; mm1 = z y x, mm2 = x z y, mm3 = y x z, all derived from mm0.
BUILD_SHUFFLES MACRO
    movq mm1, mm0
    movq mm2, mm0
    movq mm3, mm0
    psllq mm2, 16
    psrlq mm3, 32
    paddw mm2, mm3
    movq mm3, mm0
    movq mm4, mm0
    psllq mm3, 32
    psrlq mm4, 16
    paddw mm3, mm4
ENDM

; Normalize, pack, merge w, and return. Expects the accumulated vector in mm1.
FINISH MACRO
    psllw mm1, 3
    movq mm2, mm1
    pmaddwd mm2, mm2
    movq mm3, mm2
    psrlq mm3, 32
    paddd mm2, mm3
    movd IDXD, mm2
    shr IDXD, 18
    mov SCR, [ARGS + NORMTAB]
    movzx eax, word ptr [SCR + IDX * 2]
    movd mm2, eax
    punpcklwd mm2, mm2
    punpckldq mm2, mm2
    pmulhw mm1, mm2
    psllw mm1, 5
    mov SCR, [ARGS + FIXUPS]
    paddw mm1, [SCR + 8]
    psrlw mm1, 8
    packuswb mm1, mm1
    movd eax, mm1
    or eax, WREG
    emms
ENDM

IFDEF RAX
MMXTransformVector2MMX PROC
    mov eax, ecx
ELSE
MMXTransformVector2MMX PROC nSrc:DWORD, pArgs:DWORD
    push ebx
    push esi
    push edi
    mov ARGS, pArgs
    mov eax, nSrc
ENDIF

    PREPARE_SOURCE
    BUILD_SHUFFLES

    ; mm1 accumulates transform 0, mm5 accumulates transform 1.
    movq mm5, mm1
    movq mm6, mm2
    movq mm7, mm3
    mov SCR, [ARGS + TRANS0]
    mov SCR2, [ARGS + TRANS1]
    pmulhw mm1, [SCR]
    pmulhw mm5, [SCR2]
    pmulhw mm2, [SCR + 8]
    pmulhw mm6, [SCR2 + 8]
    paddsw mm1, mm2
    paddsw mm5, mm6
    pmulhw mm3, [SCR + 16]
    pmulhw mm7, [SCR2 + 16]
    paddsw mm1, mm3
    paddsw mm5, mm7

    ; Weight each by mmxWeights[w] and sum.
    mov SCR, [ARGS + WEIGHTTAB]
    psllw mm1, 4
    psllw mm5, 4
    mov IDX, [ARGS + WEIGHT0]
    pmulhw mm1, qword ptr [SCR + IDX * 8]
    mov IDX, [ARGS + WEIGHT1]
    pmulhw mm5, qword ptr [SCR + IDX * 8]
    paddsw mm1, mm5

    FINISH

IFNDEF RAX
    pop edi
    pop esi
    pop ebx
ENDIF
    ret
MMXTransformVector2MMX ENDP

IFDEF RAX
MMXTransformVector3MMX PROC
    mov eax, ecx
ELSE
MMXTransformVector3MMX PROC nSrc:DWORD, pArgs:DWORD
    push ebx
    push esi
    push edi
    mov ARGS, pArgs
    mov eax, nSrc
ENDIF

    PREPARE_SOURCE
    BUILD_SHUFFLES

    ; mm1, mm5 and mm7 accumulate transforms 0, 1 and 2. Only three shuffles exist,
    ; so each is multiplied by the matching row of each transform in turn.
    movq mm5, mm1
    movq mm7, mm1
    mov SCR, [ARGS + TRANS0]
    mov SCR2, [ARGS + TRANS1]
    pmulhw mm1, [SCR]
    pmulhw mm5, [SCR2]
    movq mm6, mm2
    movq mm4, mm2
    pmulhw mm2, [SCR + 8]
    pmulhw mm6, [SCR2 + 8]
    paddsw mm1, mm2
    paddsw mm5, mm6
    movq mm2, mm3
    movq mm6, mm3
    pmulhw mm3, [SCR + 16]
    pmulhw mm6, [SCR2 + 16]
    paddsw mm1, mm3
    paddsw mm5, mm6

    ; mm7 still holds the z y x shuffle, mm4 the x z y one and mm2 the y x z one.
    mov SCR, [ARGS + TRANS2]
    pmulhw mm7, [SCR]
    pmulhw mm4, [SCR + 8]
    paddsw mm7, mm4
    pmulhw mm2, [SCR + 16]
    paddsw mm7, mm2

    mov SCR, [ARGS + WEIGHTTAB]
    psllw mm1, 4
    psllw mm5, 4
    psllw mm7, 4
    mov IDX, [ARGS + WEIGHT0]
    pmulhw mm1, qword ptr [SCR + IDX * 8]
    mov IDX, [ARGS + WEIGHT1]
    pmulhw mm5, qword ptr [SCR + IDX * 8]
    mov IDX, [ARGS + WEIGHT2]
    pmulhw mm7, qword ptr [SCR + IDX * 8]
    paddsw mm1, mm5
    paddsw mm1, mm7

    FINISH

IFNDEF RAX
    pop edi
    pop esi
    pop ebx
ENDIF
    ret
MMXTransformVector3MMX ENDP
END
