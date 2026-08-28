; The original MMX inner loop of NGScene::CalcDirectionalLighting, lifted verbatim
; out of the __asm block that GLightPerVertex.cpp carried before the C++ port.
; MSVC accepts __asm on x86 only, so keeping the reference as inline assembly made
; the bit-exactness tests unbuildable on x64. As a .asm file it assembles for both.
;
; The three arguments all arrive in registers and the two results come back packed
; into one 64-bit return value, so there is no stack argument and no callee-saved
; register to preserve in either ABI.
;
;   uint64_t CalcDirectionalLightingMMX( const void *pDirData,
;                                        const NGfx::SMMXWord *pTranslucentShade,
;                                        uint32_t dwNormal );
;
;   returns ( dwShadowColor << 32 ) | dwColor
;
; pDirData points at SPerVertexLightState::ambient; the six SMMXWord members that
; follow it are indexed as [DIRPTR + n * 8]. GLightPerVertex.h static_asserts those
; offsets.

IFDEF RAX
    ; Win64: rcx, rdx, r8d. r10/r11 are volatile, so nothing needs saving.
    DIRPTR   TEXTEQU <r10>
    TRANSPTR TEXTEQU <r11>
ELSE
    ; cdecl: arguments on the stack, eax/ecx are caller-saved.
    DIRPTR   TEXTEQU <eax>
    TRANSPTR TEXTEQU <ecx>
ENDIF

.code

IFDEF RAX
CalcDirectionalLightingMMX PROC
    mov DIRPTR, rcx
    mov TRANSPTR, rdx
    movd mm7, r8d
ELSE
CalcDirectionalLightingMMX PROC \
    pDirData:DWORD, \
    pTranslucentShade:DWORD, \
    dwNormal:DWORD

    mov DIRPTR, pDirData
    mov TRANSPTR, pTranslucentShade
    movd mm7, dwNormal
ENDIF

    punpcklbw mm7, mm7
    psubw mm7, [DIRPTR + 5 * 8]   ; shift
    pmaddwd mm7, [DIRPTR + 4 * 8] ; dirLight
    movq mm6, mm7
    psrlq mm6, 32
    paddd mm7, mm6
    psrad mm7, 15
    punpcklwd mm7, mm7
    punpckldq mm7, mm7

    movq mm6, mm7
    movq mm5, mm7
    psraw mm6, 16                 ; sign mask
    pand mm7, mm6                 ; shifted & sign
    pandn mm6, mm5                ; mm6 = f, range [0, 0x4000]
    pcmpeqw mm0, mm0
    pxor mm7, mm0                 ; mm7 = -f

    movq mm0, [DIRPTR + 0 * 8]    ; ambient, becomes vRes
    movq mm1, mm0                 ; vResShadow
    movq mm2, [DIRPTR + 1 * 8]    ; lightColor
    movq mm3, [DIRPTR + 2 * 8]    ; incidentShadowColor
    movq mm4, [DIRPTR + 3 * 8]    ; shadeColor
    movq mm5, [TRANSPTR]          ; translucentShade

    pmulhw mm2, mm6
    pmulhw mm3, mm6
    pmulhw mm5, mm7
    pmulhw mm4, mm7
    paddw mm0, mm2
    paddw mm1, mm3
    paddw mm0, mm5
    paddw mm1, mm4
    psraw mm0, 4
    psraw mm1, 4
    packuswb mm0, mm0
    packuswb mm1, mm1

    movd eax, mm0                 ; colour, zero extends into rax on x64
    movd edx, mm1                 ; shadow colour
IFDEF RAX
    shl rdx, 32
    or rax, rdx
ENDIF
    emms
    ret

CalcDirectionalLightingMMX ENDP
END
