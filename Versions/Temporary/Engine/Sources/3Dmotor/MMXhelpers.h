#pragma once

#include <cstdint>

namespace mmx {
	// Split 64-bit value into 4 signed 16-bit words (little-endian order)
	inline void split64(uint64_t val, short w[4]) {
		w[0] = static_cast<short>( val        & 0xFFFF );
		w[1] = static_cast<short>((val >> 16) & 0xFFFF );
		w[2] = static_cast<short>((val >> 32) & 0xFFFF );
		w[3] = static_cast<short>((val >> 48) & 0xFFFF );
	}

	// Combine 4 signed 16-bit words into 64-bit value
	inline uint64_t combine64(const short w[4]) {
		return  (static_cast<uint64_t>(static_cast<uint16_t>(w[0]))      ) |
				(static_cast<uint64_t>(static_cast<uint16_t>(w[1])) << 16) |
				(static_cast<uint64_t>(static_cast<uint16_t>(w[2])) << 32) |
				(static_cast<uint64_t>(static_cast<uint16_t>(w[3])) << 48);
	}

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

	inline uint64_t pmaddwd(uint64_t dest, uint64_t src) {
		short d[4], s[4];
		split64(dest, d);
		split64(src, s);
		uint32_t r0 = static_cast<int32_t>(d[0])*s[0] + static_cast<int32_t>(d[1])*s[1];
		uint32_t r1 = static_cast<int32_t>(d[2])*s[2] + static_cast<int32_t>(d[3])*s[3];
		return (static_cast<uint64_t>(r0) & 0xFFFFFFFFull) | (static_cast<uint64_t>(r1) << 32);
	}

	// Isolated psllq (64-bit) emulation
	inline uint64_t psllq(uint64_t val, int shift) {
		return val << shift;
	}

	inline uint64_t psrlq(uint64_t val, int shift) {
		if(shift > 63) return 0;
		return val >> shift;
	}
	inline uint64_t psllw(uint64_t val, int shift) {
		return
		(((val & 0xFFFF) << shift) & 0xFFFF) |
		(((val & 0xFFFF0000) << shift) & 0xFFFF0000) |
		(((val & 0xFFFF00000000) << shift) & 0xFFFF00000000) |
		(((val & 0xFFFF000000000000) << shift) & 0xFFFF000000000000);
	}

	// 64-bit paddsw emulation: signed 16-bit saturating addition
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

	// 64-bit paddw emulation: per-word non-saturating addition (16-bit)
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

	inline uint64_t shuffleTransform(uint64_t mm0, int leftShift, int rightShift) {
		uint64_t tmp1 = psllq(mm0, leftShift);
		uint64_t tmp2 = psrlq(mm0, rightShift);
		return pmaddwd(tmp1, tmp2);
	}

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
}
