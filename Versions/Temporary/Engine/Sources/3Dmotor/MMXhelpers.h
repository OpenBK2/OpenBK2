#pragma once

#include <cstdint>
#include <algorithm>

namespace mmx {

	// Split 64-bit value into 4 signed 16-bit words (little-endian order)
	inline void split64(uint64_t val, short & z, short & y, short & x, short & w) {
		z = static_cast<short>( val        & 0xFFFF );
		y = static_cast<short>((val >> 16) & 0xFFFF );
		x = static_cast<short>((val >> 32) & 0xFFFF );
		w = static_cast<short>((val >> 48) & 0xFFFF );
	}

	// Split 64-bit value into 4 signed 16-bit words (little-endian order)
	inline void split64(uint64_t val, short w[4]) {
		split64(val, w[0], w[1], w[2], w[3]);
	}

	// Combine 4 signed 16-bit words into 64-bit value
	inline uint64_t combine64(short z, short y, short x, short w) {
		return  (static_cast<uint64_t>(static_cast<uint16_t>(z))) |
				(static_cast<uint64_t>(static_cast<uint16_t>(y)) << 16) |
				(static_cast<uint64_t>(static_cast<uint16_t>(x)) << 32) |
				(static_cast<uint64_t>(static_cast<uint16_t>(w)) << 48);
	}

	inline uint64_t combine64(const short w[4]) {
		return combine64(w[0], w[1], w[2], w[3]);
	}

	// Multiply Packed Signed Integers and Store High Result
	/*
	PMULHW (With 64-bit Operands)
	TEMP0[31:0] := DEST[15:0] ∗ SRC[15:0]; (* Signed multiplication *)
	TEMP1[31:0] := DEST[31:16] ∗ SRC[31:16];
	TEMP2[31:0] := DEST[47:32] ∗ SRC[47:32];
	TEMP3[31:0] := DEST[63:48] ∗ SRC[63:48];
	DEST[15:0] := TEMP0[31:16];
	DEST[31:16] := TEMP1[31:16];
	DEST[47:32] := TEMP2[31:16];
	DEST[63:48] := TEMP3[31:16];
	*/
	inline uint64_t pmulhw(uint64_t dest, uint64_t src) {
		short d[4], s[4], r[4];
		split64(dest, d);
		split64(src, s);
		for(int i=0; i<4; ++i) {
			int32_t tmp = static_cast<int32_t>(d[i]) * static_cast<int32_t>(s[i]);
			r[i] = static_cast<short>(tmp >> 16); // keep high 16 bits
		}
		return combine64(r);
	}

	// Multiply Packed Signed Integers and Store Low Result
	/*
	PMULLW (With 64-bit Operands)
	TEMP0[31:0] := DEST[15:0] ∗ SRC[15:0]; (* Signed multiplication *)
	TEMP1[31:0] := DEST[31:16] ∗ SRC[31:16];
	TEMP2[31:0] := DEST[47:32] ∗ SRC[47:32];
	TEMP3[31:0] := DEST[63:48] ∗ SRC[63:48];
	DEST[15:0] := TEMP0[15:0];
	DEST[31:16] := TEMP1[15:0];
	DEST[47:32] := TEMP2[15:0];
	DEST[63:48] := TEMP3[15:0];
	*/
	inline uint64_t pmullw(uint64_t dest, uint64_t src) {
		short d[4], s[4], r[4];
		split64(dest, d);
		split64(src, s);
		for (int i = 0; i < 4; ++i) {
			int32_t tmp = static_cast<int32_t>(d[i]) * static_cast<int32_t>(s[i]);
			r[i] = static_cast<short>(tmp & 0xFFFF);  // keep low 16 bits
		}
		return combine64(r);
	}
	// Multiply and Add Packed Integers
	/*
	PMADDWD (With 64-bit Operands)
	DEST[31:0] := (DEST[15:0] ∗ SRC[15:0]) + (DEST[31:16] ∗ SRC[31:16]);
	DEST[63:32] := (DEST[47:32] ∗ SRC[47:32]) + (DEST[63:48] ∗ SRC[63:48]);
	*/
	inline uint64_t pmaddwd(uint64_t dest, uint64_t src) {
		short d[4], s[4];
		split64(dest, d);
		split64(src, s);
		uint32_t r0 = static_cast<int32_t>(d[0]) * s[0] + static_cast<int32_t>(d[1]) * s[1];
		uint32_t r1 = static_cast<int32_t>(d[2]) * s[2] + static_cast<int32_t>(d[3]) * s[3];
		return (static_cast<uint64_t>(r0) & 0xFFFFFFFFULL) | (static_cast<uint64_t>(r1) << 32);
	}

	// Shift Packed Data Left Logical
	/*
	PSLLQ (With 64-bit Operand)
	IF (COUNT > 63)
	THEN
	    DEST[64:0] := 0000000000000000H;
	ELSE
	    DEST := ZeroExtend(DEST << COUNT);
	FI;
	*/
	inline uint64_t psllq(uint64_t val, int shift) {
		return val << shift;
	}

	// Shift Packed Data Right Logical
	/*
	PSRLQ (With 64-bit Operand)
	IF (COUNT > 63)
	THEN
	    DEST[64:0] := 0000000000000000H
	ELSE
	    DEST := ZeroExtend(DEST >> COUNT);
	FI;
	*/
	inline uint64_t psrlq(uint64_t val, int shift) {
		if(shift > 63) return 0;
		return val >> shift;
	}

	// Shift Packed Data Left Logical
	/*
	PSLLW (With 64-bit Operand)
	IF (COUNT > 15)
	THEN
	    DEST[64:0] := 0000000000000000H;
	ELSE
	    DEST[15:0] := ZeroExtend(DEST[15:0] << COUNT);
	    (* Repeat shift operation for 2nd and 3rd words *)
	    DEST[63:48] := ZeroExtend(DEST[63:48] << COUNT);
	FI;
	*/
	inline uint64_t psllw(uint64_t val, int shift) {
		return
		(((val & 0xFFFF) << shift) & 0xFFFF) |
		(((val & 0xFFFF0000) << shift) & 0xFFFF0000) |
		(((val & 0xFFFF00000000) << shift) & 0xFFFF00000000) |
		(((val & 0xFFFF000000000000) << shift) & 0xFFFF000000000000);
	}

	// Add Packed Signed Integers with Signed Saturation
	/*
	PADDSW (with 64-bit operands)
	DEST[15:0] := SaturateToSignedWord(DEST[15:0] + SRC[15:0] );
	(* Repeat add operation for 2nd and 7th words *)
	DEST[63:48] := SaturateToSignedWord(DEST[63:48] + SRC[63:48] );
	SaturateToSignedWord — Represents the result of an operation as a signed 16-bit value.
	If the result is less than –32768, it is represented by the saturated value –32768 (8000H);
	if it is greater than 32767, it is represented by the saturated value 32767 (7FFFH).
	*/
	inline uint64_t paddsw(uint64_t a, uint64_t b) {
		short wa[4], wb[4], wr[4];
		split64(a, wa);
		split64(b, wb);
		for(int i = 0; i < 4; ++i) {
			int32_t sum = static_cast<int32_t>(wa[i]) + static_cast<int32_t>(wb[i]);
			if(sum > 32767) sum = 32767;
			if(sum < -32768) sum = -32768;
			wr[i] = static_cast<short>(sum);
		}
		return combine64(wr);
	}

	// Add Packed Integers
	/*
	PADDW (With 64-bit Operands)
	DEST[15:0] := DEST[15:0] + SRC[15:0];
	(* Repeat add operation for 2nd and 3th word *)
	DEST[63:48] := DEST[63:48] + SRC[63:48];
	*/
	inline uint64_t paddw(uint64_t a, uint64_t b) {
		short wa[4], wb[4], wr[4];
		split64(a, wa);
		split64(b, wb);
		for (int i = 0; i < 4; ++i) {
			// Wrap-around addition, like MMX paddw (non-saturating)
			wr[i] = static_cast<short>(wa[i] + wb[i]);
		}
		return combine64(wr);
	}

	// Subtract Packed Integers
	/*
	PSUBW (With 64-bit Operands)
	DEST[15:0] := DEST[15:0] − SRC[15:0];
	(* Repeat subtract operation for 2nd and 3rd word *)
	DEST[63:48] := DEST[63:48] − SRC[63:48];
	*/
	inline uint64_t psubw(uint64_t a, uint64_t b) {
		short wa[4], wb[4], wr[4];
		split64(a, wa);
		split64(b, wb);
		for (int i = 0; i < 4; ++i) {
			wr[i] = static_cast<short>(wa[i] - wb[i]);
		}
		return combine64(wr);
	}

	inline uint64_t shuffleTransform(uint64_t mm0, int leftShift, int rightShift) {
		uint64_t tmp1 = psllq(mm0, leftShift);
		uint64_t tmp2 = psrlq(mm0, rightShift);
		return paddw(tmp1, tmp2);
	}

	// Unpack Low Data
	/*
	PUNPCKLBW Instruction With 64-bit Operands:
	DEST[63:56] := SRC[31:24];
	DEST[55:48] := DEST[31:24];
	DEST[47:40] := SRC[23:16];
	DEST[39:32] := DEST[23:16];
	DEST[31:24] := SRC[15:8];
	DEST[23:16] := DEST[15:8];
	DEST[15:8] := SRC[7:0];
	DEST[7:0] := DEST[7:0];
	*/
	inline uint64_t punpcklbw(uint64_t low, uint64_t high)
	{
		uint64_t result =
			((low & 0xFF)) |
			((high & 0xFF) << 8) |
			((low & 0xFF00) << 8) |
			((high & 0xFF00) << 16) |
			((low & 0xFF0000) << 16) |
			((high & 0xFF0000) << 24) |
			((low & 0xFF000000) << 24) |
			((high & 0xFF000000) << 32);
		return result;
	}

	// Unpack Low Data
	/*
	PUNPCKLWD Instruction With 64-bit Operands:
	DEST[63:48] := SRC[31:16];
	DEST[47:32] := DEST[31:16];
	DEST[31:16] := SRC[15:0];
	DEST[15:0] := DEST[15:0];
	*/
	inline uint64_t punpcklwd(uint64_t low, uint64_t high)
	{
		uint64_t result =
			((low & 0xFFFF)) |
			((high & 0xFFFF) << 16) |
			((low & 0xFFFF0000) << 16) |
			((high & 0xFFFF0000) << 32);
		return result;
	}

	// Unpack High Data
	/*
	PUNPCKHWD Instruction With 64-bit Operands:
	DEST[15:0] := DEST[47:32];
	DEST[31:16] := SRC[47:32];
	DEST[47:32] := DEST[63:48];
	DEST[63:48] := SRC[63:48];
	*/
	inline uint64_t punpckhwd(uint64_t low, uint64_t high)
	{
		return
			((low  >> 32) & 0xFFFFULL) |
			(((high >> 32) & 0xFFFFULL) << 16) |
			(((low  >> 48) & 0xFFFFULL) << 32) |
			(((high >> 48) & 0xFFFFULL) << 48);
	}

	// Unpack Low Data
	/*
	PUNPCKLDQ Instruction With 64-bit Operands:
	DEST[63:32] := SRC[31:0];
	DEST[31:0] := DEST[31:0];
	*/
	inline uint64_t punpckldq(uint64_t low, uint64_t high)
	{
		uint64_t result =
			((low & 0xFFFFFFFF)) |
			((high & 0xFFFFFFFF) << 32);
		return result;
	}

	inline uint8_t saturate_signed_word_to_unsigned_byte(int16_t w) {
		return static_cast<uint8_t>(std::clamp<int16_t>(w, 0, 255));
	}

	inline int16_t saturate_signed_double_word_to_signed_word(int32_t v) {
		return static_cast<int16_t>(std::clamp<int32_t>(v, -32768, 32767));
	};

	// Pack With Unsigned Saturation
	/*
	PACKUSWB (With 64-bit Operands)
	DEST[7:0] := SaturateSignedWordToUnsignedByte DEST[15:0];
	DEST[15:8] := SaturateSignedWordToUnsignedByte DEST[31:16];
	DEST[23:16] := SaturateSignedWordToUnsignedByte DEST[47:32];
	DEST[31:24] := SaturateSignedWordToUnsignedByte DEST[63:48];
	DEST[39:32] := SaturateSignedWordToUnsignedByte SRC[15:0];
	DEST[47:40] := SaturateSignedWordToUnsignedByte SRC[31:16];
	DEST[55:48] := SaturateSignedWordToUnsignedByte SRC[47:32];
	DEST[63:56] := SaturateSignedWordToUnsignedByte SRC[63:48];
	SaturateSignedWordToUnsignedByte — Converts a signed 16-bit value to an unsigned 8-bit value.
    If the signed 16-bit value is less than zero, it is represented by the saturated value zero (00H);
    if it is greater than 255, it is represented by the saturated value 255 (FFH).
	*/
	inline uint64_t packuswb(uint64_t low, uint64_t high) {
		uint64_t result = 0;
		// Process 4 words from low (each 16 bits)
		for (int i = 0; i < 4; i++) {
			int16_t w = static_cast<int16_t>((low >> (i * 16)) & 0xFFFF);
			uint8_t b = saturate_signed_word_to_unsigned_byte(w);
			result |= (static_cast<uint64_t>(b) << (i * 8));
		}
		// Process 4 words from high (each 16 bits)
		for (int i = 0; i < 4; i++) {
			int16_t w = static_cast<int16_t>((high >> (i * 16)) & 0xFFFF);
			uint8_t b = saturate_signed_word_to_unsigned_byte(w);
			result |= (static_cast<uint64_t>(b) << ((i + 4) * 8));
		}
		return result;
	}

	// Pack With Signed Saturation
	inline uint64_t packssdw(uint64_t low, uint64_t high) {

		int32_t l0 = static_cast<int32_t>(low & 0xFFFFFFFFULL);
		int32_t l1 = static_cast<int32_t>((low >> 32) & 0xFFFFFFFFULL);
		int32_t h0 = static_cast<int32_t>(high & 0xFFFFFFFFULL);
		int32_t h1 = static_cast<int32_t>((high >> 32) & 0xFFFFFFFFULL);

		uint64_t r0 = static_cast<uint16_t>(saturate_signed_double_word_to_signed_word(l0));
		uint64_t r1 = static_cast<uint16_t>(saturate_signed_double_word_to_signed_word(l1));
		uint64_t r2 = static_cast<uint16_t>(saturate_signed_double_word_to_signed_word(h0));
		uint64_t r3 = static_cast<uint16_t>(saturate_signed_double_word_to_signed_word(h1));

		return (r3 << 48) | (r2 << 32) | (r1 << 16) | r0;
	}

	inline uint32_t low_u32(uint64_t val) {
		return val & 0xFFFFFFFFUL;
	}

	inline uint32_t high_u32(uint64_t val) {
		return val >> 32;
	}

	inline int32_t low_i32(uint64_t val) {
		return static_cast<int32_t>(val & 0xFFFFFFFFUL);
	}

	inline int32_t high_i32(uint64_t val) {
		return static_cast<int32_t>(val >> 32);
	}

	inline uint64_t pack(uint32_t high, uint32_t low) {
		return (static_cast<uint64_t>(high) << 32) | low;
	}

	// Add Packed Integers
	/*
	PADDD (With 64-bit Operands)
	DEST[31:0] := DEST[31:0] + SRC[31:0];
	DEST[63:32] := DEST[63:32] + SRC[63:32];
	*/
	inline uint64_t paddd(uint64_t a, uint64_t b) {
		uint32_t low = low_u32(a) + low_u32(b);
		uint32_t high = high_u32(a) + high_u32(b);
		return pack(high, low);
	}

	// Shift Packed Data Right Arithmetic
	/*
	PSRAD (with 64-bit operand)
	IF (COUNT > 31)
	    THEN COUNT := 32;
	FI;
	DEST[31:0] := SignExtend(DEST[31:0] >> COUNT);
	DEST[63:32] := SignExtend(DEST[63:32] >> COUNT);
	*/
	inline uint64_t psrad(uint64_t val, uint64_t shift) {

		int32_t low = low_i32(val) >> shift;
		int32_t high = high_i32(val) >> shift;
		return pack(high, low);
	}

	// Shift Packed Data Right Arithmetic
	/*
	PSRAW (With 64-bit Operand)
	IF (COUNT > 15)
	THEN COUNT := 16;
	FI;
	DEST[15:0] := SignExtend(DEST[15:0] >> COUNT);
	(* Repeat shift operation for 2nd and 3rd words *)
	DEST[63:48] := SignExtend(DEST[63:48] >> COUNT);
	*/
	inline uint64_t psraw(uint64_t val, uint64_t shift) {

		int16_t v0 = static_cast<int16_t>(val & 0xFFFFULL);
		int16_t v1 = static_cast<int16_t>((val >> 16) & 0xFFFFULL);
		int16_t v2 = static_cast<int16_t>((val >> 32) & 0xFFFFULL);
		int16_t v3 = static_cast<int16_t>((val >> 48) & 0xFFFFULL);

		v0 >>= shift;
		v1 >>= shift;
		v2 >>= shift;
		v3 >>= shift;

		return combine64(v0, v1, v2, v3);
	}

	// Logical AND
	/*
	PAND (64-bit Operand)
	DEST := DEST AND SRC
	*/
	inline uint64_t pand(uint64_t a, uint64_t b) {
		return a & b;
	}

	// Logical AND NOT
	/*
	PANDN (64-bit Operand)
	DEST := NOT(DEST) AND SRC
	*/
	inline uint64_t pandn(uint64_t a, uint64_t b) {
		return ~a & b;
	}

	// Logical Exclusive OR
	/*
	PXOR (64-bit Operand)
	DEST := DEST XOR SRC
	*/
	inline uint64_t pxor(uint64_t a, uint64_t b) {
		return a ^ b;
	}

	// Compare Packed Signed Integers for Greater Than
	/*
	PCMPGTW (With 64-bit Operands)
	IF DEST[15:0] > SRC[15:0]
	THEN DEST[15:0] := FFFFH;
	ELSE DEST[15:0] := 0; FI;
	(* Continue comparison of 2nd and 3rd words in DEST and SRC *)
	IF DEST[63:48] > SRC[63:48]
	THEN DEST[63:48] := FFFFH;
	ELSE DEST[63:48] := 0; FI;
	*/
	inline uint64_t pcmpgtw(uint64_t a, uint64_t b) {
		uint64_t result = 0;
		for (int i = 0; i < 4; ++i) {
			// extract 16-bit word from each operand
			int16_t wa = static_cast<int16_t>((a >> (i * 16)) & 0xFFFF);
			int16_t wb = static_cast<int16_t>((b >> (i * 16)) & 0xFFFF);

			// compare signed
			uint16_t out = (wa > wb) ? 0xFFFF : 0x0000;

			result |= (uint64_t)out << (i * 16);
		}
		return result;
	}

}
