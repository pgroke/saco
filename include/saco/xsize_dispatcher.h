#pragma once

#include <saco/xcore.h>

#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace saco::detail {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)

#define SACO_HAVE_BSR_INTRIN
SACO_ALWAYS_INLINE unsigned bsr_intrin(unsigned s) {
	unsigned long index;
	_BitScanReverse(&index, s);
	return static_cast<unsigned>(index);
}

#elif defined(__GNUC__) || defined(__clang__)

#define SACO_HAVE_BSR_INTRIN
SACO_ALWAYS_INLINE unsigned bsr_intrin(unsigned s) {
	return 31u - static_cast<unsigned int>(__builtin_clz(s));
}

#endif

SACO_ALWAYS_INLINE unsigned bsr_de_bruijn_32(unsigned v) {
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;

	static constexpr std::uint_fast8_t TABLE[32] = {0, 9,  1,  10, 13, 21, 2,  29, 11, 14, 16, 18, 22, 25, 3, 30,
													8, 12, 20, 28, 15, 17, 24, 7,  19, 27, 23, 6,  26, 5,  4, 31};

	return TABLE[static_cast<std::uint32_t>(v * 0x07C4ACDDu) >> 27];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct size_dispatcher_switch {
	static constexpr std::size_t MAX_SIZE = 2048;

	static std::size_t compute_bucket(std::size_t s) {
		s = (s < 32) ? 31 : s - 1;
#if defined(SACO_HAVE_BSR_INTRIN)
		std::size_t const w = bsr_intrin(static_cast<unsigned>(s));
		SACO_ASSERT(w == bsr_de_bruijn_32(static_cast<unsigned>(s)));
#else
		std::size_t const w = bsr_de_bruijn_32(static_cast<unsigned>(s));
#endif
		std::size_t const a = w - 4;
		std::size_t const b = (s >> (w - 2)) & 3;
		return (a << 2 | b) - 3;
	}

	template <template <std::size_t> class Fn>
	static auto dispatch(std::size_t s) {
		SACO_ASSERT(s > 0);
		SACO_ASSERT(s <= MAX_SIZE);
		auto const bucket = compute_bucket(s);

		switch (bucket) {
		case 0: return Fn<32>{}();
		case 1: return Fn<40>{}();
		case 2: return Fn<48>{}();
		case 3: return Fn<56>{}();
		case 4: return Fn<64>{}();
		case 5: return Fn<80>{}();
		case 6: return Fn<96>{}();
		case 7: return Fn<112>{}();
		case 8: return Fn<128>{}();
		case 9: return Fn<160>{}();
		case 10: return Fn<192>{}();
		case 11: return Fn<224>{}();
		case 12: return Fn<256>{}();
		case 13: return Fn<320>{}();
		case 14: return Fn<384>{}();
		case 15: return Fn<448>{}();
		case 16: return Fn<512>{}();
		case 17: return Fn<640>{}();
		case 18: return Fn<768>{}();
		case 19: return Fn<896>{}();
		case 20: return Fn<1024>{}();
		case 21: return Fn<1280>{}();
		case 22: return Fn<1536>{}();
		case 23: return Fn<1792>{}();
		case 24: return Fn<2048>{}();
		default: SACO_ASSERT_MSG(0, "internal error in size dispatcher"); std::abort();
		}
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct size_dispatcher_nested_if {
	// clang-format off
	static constexpr std::size_t SIZE_BUCKETS[] = {
		32,
		40,
		48,
		56,
		64,
		80,
		96,
		112,
		128,
		160,
		192,
		224,
		256,
		320,
		384,
		448,
		512,
		640,
		768,
		896,
		1024,
		1280,
		1536,
		1792,
		//2048,
	};
	// clang-format on

	static constexpr int BUCKET_COUNT = sizeof(SIZE_BUCKETS) / sizeof(SIZE_BUCKETS[0]);
	static constexpr std::size_t MAX_SIZE = SIZE_BUCKETS[BUCKET_COUNT - 1];

	template <template <std::size_t> class Fn, int FIRST, int LAST, int MIDDLE = (FIRST + LAST + 1) / 2>
	struct impl {
		static auto step(std::size_t s) {
			static_assert(FIRST < LAST);
			static_assert(FIRST < MIDDLE);
			static_assert(MIDDLE <= LAST);
			if (s <= SIZE_BUCKETS[MIDDLE - 1])
				return impl<Fn, FIRST, MIDDLE - 1>::step(s);
			else
				return impl<Fn, MIDDLE, LAST>::step(s);
		}
	};

	template <template <std::size_t> class Fn, int FIRST>
	struct impl<Fn, FIRST, FIRST, FIRST> {
		static auto step(std::size_t s) {
			return Fn<SIZE_BUCKETS[FIRST]>{}();
		}
	};

	template <template <std::size_t> class Fn>
	static auto dispatch(std::size_t s) {
		SACO_ASSERT(s > 0);
		SACO_ASSERT(s <= MAX_SIZE);
		return impl<Fn, 0, BUCKET_COUNT - 1, 16>::step(s);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace saco::detail
