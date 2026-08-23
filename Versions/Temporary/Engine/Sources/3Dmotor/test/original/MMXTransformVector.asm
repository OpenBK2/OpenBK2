; The original MMX body of MMXTransformVectorImpl, lifted out of the __asm block in
; test/original.h so it assembles for x64 as well as x86.
;
; Two things in the inline original do not survive a move to a .asm file untouched:
;
;   ebx  MSVC's inline assembler notices an __asm block clobbering a callee-saved
;        register and emits the save/restore itself. A standalone procedure gets no
;        such help, so x86 pushes ebx/esi/edi explicitly and x64 avoids the problem
;        by using only volatile registers.
;
;   nNormalizeTable  the inline form names the C++ global directly and lets the
;        compiler resolve it. MASM would need the decorated name, which differs
;        between architectures and is not worth pinning down; the table is passed in
;        as an argument instead.
;
;   uint32_t MMXTransformVectorMMX( uint32_t nSrc,
;                                   const SMMXFixups *pFixups,
;                                   const NGfx::SCompactTransformer *pTrans,
;                                   const short *pNormalizeTable );
;
; Four arguments, all register-passed on Win64, and the packed SCompactVector comes
; back as the return value rather than through an output pointer, so there is no
; stack argument in either ABI. pFixups is indexed as [.. + 0] normalFixup and
; [.. + 8] shiftedFixup, pTrans as [.. + 0/8/16] for a, b, c; GSSETransform.h
; static_asserts both layouts.

IFDEF RAX
    WREG     TEXTEQU <r10d>   ; the untouched w byte of the source
    FIXPTR   TEXTEQU <rdx>
    TRANSPTR TEXTEQU <r8>
    NORMTAB  TEXTEQU <r9>
    IDX      TEXTEQU <rcx>
    IDXD     TEXTEQU <ecx>
ELSE
    WREG     TEXTEQU <edx>
    FIXPTR   TEXTEQU <edi>
    TRANSPTR TEXTEQU <esi>
    NORMTAB  TEXTEQU <ebx>
    IDX      TEXTEQU <ecx>
    IDXD     TEXTEQU <ecx>
ENDIF

.code

IFDEF RAX
MMXTransformVectorMMX PROC
    mov eax, ecx                  ; nSrc
    mov WREG, eax
ELSE
MMXTransformVectorMMX PROC \
    nSrc:DWORD, \
    pFixups:DWORD, \
    pTrans:DWORD, \
    pNormalizeTable:DWORD

    push ebx
    push esi
    push edi

    mov eax, nSrc
    mov WREG, eax
    mov FIXPTR, pFixups
    mov TRANSPTR, pTrans
    mov NORMTAB, pNormalizeTable
ENDIF

    and eax, 0FFFFFFh             ; drop w
    and WREG, 0FF000000h          ; keep only w
    movd mm7, eax

    pxor mm0, mm0
    punpcklbw mm0, mm7
    psubw mm0, [FIXPTR]           ; normalFixup

    movq mm1, mm0                 ; z y x
    pmulhw mm1, [TRANSPTR]
    movq mm2, mm0
    movq mm3, mm0
    psllq mm2, 16
    psrlq mm3, 32
    paddw mm2, mm3                ; x z y
    pmulhw mm2, [TRANSPTR + 8]
    movq mm3, mm0
    movq mm4, mm0
    paddsw mm1, mm2
    psllq mm3, 32
    psrlq mm4, 16
    paddw mm3, mm4                ; y x z
    pmulhw mm3, [TRANSPTR + 16]
    paddsw mm1, mm3               ; packed result

    ; normalize
    psllw mm1, 3
    movq mm2, mm1
    pmaddwd mm2, mm2
    movq mm3, mm2
    psrlq mm3, 32
    paddd mm2, mm3
    movd IDXD, mm2
    shr IDXD, 18
    movzx eax, word ptr [NORMTAB + IDX * 2]
    movd mm2, eax
    punpcklwd mm2, mm2
    punpckldq mm2, mm2
    pmulhw mm1, mm2
    psllw mm1, 5

    ; pack and merge the original w back in
    paddw mm1, [FIXPTR + 8]       ; shiftedFixup
    psrlw mm1, 8
    packuswb mm1, mm1
    movd eax, mm1
    or eax, WREG
    emms

IFNDEF RAX
    pop edi
    pop esi
    pop ebx
ENDIF
    ret

MMXTransformVectorMMX ENDP
END
