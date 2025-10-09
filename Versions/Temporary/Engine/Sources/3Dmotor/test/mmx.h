#pragma once

#include <cstdint>

namespace original {

    static uint64_t psllw_1(uint64_t val) {
        uint64_t result;
        __asm {
            movq mm1, val
            psllw mm1, 1
            movq result, mm1
            emms
        }
        return result;
    }

    static uint64_t psllw_3(uint64_t val) {
        uint64_t result;
        __asm {
            movq mm1, val
            psllw mm1, 3
            movq result, mm1
            emms
        }
        return result;
    }

    static uint64_t psllw_5(uint64_t val) {
        uint64_t result;
        __asm {
            movq mm1, val
            psllw mm1, 5
            movq result, mm1
            emms
        }
        return result;
    }

    static uint64_t psllq_16(uint64_t val) {
        uint64_t result;
        __asm {
            movq mm0, val
            psllq mm0, 16
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t psrlq_32(uint64_t val) {
        uint64_t result;
        __asm {
            movq mm0, val
            psrlq mm0, 32
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t psrlw_1(uint64_t val) {
        uint64_t result;
        __asm {
            movq mm0, val
            psrlw mm0, 1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t psrlw_2(uint64_t val) {
        uint64_t result;
        __asm {
            movq mm0, val
            psrlw mm0, 2
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t punpcklbw(uint64_t low1, uint64_t high1) {
        uint64_t result;
        __asm {
            movq mm0, low1
            movq mm1, high1
            punpcklbw mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t punpcklwd(uint64_t low1, uint64_t high1) {
        uint64_t result;
        __asm {
            movq mm0, low1
            movq mm1, high1
            punpcklwd mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t punpckhwd(uint64_t low1, uint64_t high1) {
        uint64_t result;
        __asm {
            movq mm0, low1
            movq mm1, high1
            punpckhwd mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t punpckldq(uint64_t low1, uint64_t high1) {
        uint64_t result;
        __asm {
            movq mm0, low1
            movq mm1, high1
            punpckldq mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t packssdw(uint64_t low1, uint64_t high1) {
        uint64_t result;
        __asm {
            movq mm0, low1
            movq mm1, high1
            packssdw mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t packuswb(uint64_t low1, uint64_t high1) {
        uint64_t result;
        __asm {
            movq mm0, low1
            movq mm1, high1
            packuswb mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t paddsw(uint64_t a, uint64_t b) {
        uint64_t result;
        __asm {
            movq mm0, a
            movq mm1, b
            paddsw mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t pmaddwd(uint64_t a, uint64_t b) {
        uint64_t result;
        __asm {
            movq mm0, a
            movq mm1, b
            pmaddwd mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t paddw(uint64_t a, uint64_t b) {
        uint64_t result;
        __asm {
            movq mm0, a
            movq mm1, b
            paddw mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t psubw(uint64_t a, uint64_t b) {
        uint64_t result;
        __asm {
            movq mm0, a
            movq mm1, b
            psubw mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t pmulhw(uint64_t a, uint64_t b) {
        uint64_t result;
        __asm {
            movq mm0, a
            movq mm1, b
            pmulhw mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t pmullw(uint64_t a, uint64_t b) {
        uint64_t result;
        __asm {
            movq mm0, a
            movq mm1, b
            pmullw mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t paddd(uint64_t a, uint64_t b) {
        uint64_t result;
        __asm {
            movq mm0, a
            movq mm1, b
            paddd mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t psrad_14(uint64_t val) {
        uint64_t result;
        __asm {
            movq mm0, val
            psrad mm0, 14
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t psrad_15(uint64_t val) {
        uint64_t result;
        __asm {
            movq mm0, val
            psrad mm0, 15
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t psraw_4(uint64_t val) {
        uint64_t result;
        __asm {
            movq mm0, val
            psraw mm0, 4
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t psraw_16(uint64_t val) {
        uint64_t result;
        __asm {
            movq mm0, val
            psraw mm0, 16
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t pand(uint64_t a, uint64_t b) {
        uint64_t result;
        __asm {
            movq mm0, a
            movq mm1, b
            pand mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t pandn(uint64_t a, uint64_t b) {
        uint64_t result;
        __asm {
            movq mm0, a
            movq mm1, b
            pandn mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t pxor(uint64_t a, uint64_t b) {
        uint64_t result;
        __asm {
            movq mm0, a
            movq mm1, b
            pxor mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t por(uint64_t a, uint64_t b) {
        uint64_t result;
        __asm {
            movq mm0, a
            movq mm1, b
            por mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t pcmpgtw(uint64_t a, uint64_t b) {
        uint64_t result;
        __asm {
            movq mm0, a
            movq mm1, b
            pcmpgtw mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }

    static uint64_t pcmpeqw(uint64_t a, uint64_t b) {
        uint64_t result;
        __asm {
            movq mm0, a
            movq mm1, b
            pcmpeqw mm0, mm1
            movq result, mm0
            emms
        }
        return result;
    }


}
