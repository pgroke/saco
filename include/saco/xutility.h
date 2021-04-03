#pragma once

#include <saco/xcore.h>

#include <type_traits>

#define SACO_REQUIRES(...) std::enable_if_t<(__VA_ARGS__), int> = 0
#define SACO_REQUIRES_NON_ARRAY(...) SACO_REQUIRES(!::std::is_array_v<__VA_ARGS__>)
#define SACO_REQUIRES_UB_ARRAY(...) SACO_REQUIRES(::saco::detail::is_unbounded_array_v<__VA_ARGS__>)

namespace saco::detail {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
struct is_unbounded_array : std::false_type {};

template <class T>
struct is_unbounded_array<T[]> : std::true_type {};

template <class T>
inline constexpr bool is_unbounded_array_v = is_unbounded_array<T>::value;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SACO_ALWAYS_INLINE void* alloc_raw(std::size_t size) {
	auto const p = ::operator new(size);

#if !defined(NDEBUG)
	if (size >= MAX_NEW_ALIGNMENT) {
		SACO_ASSERT(reinterpret_cast<std::uintptr_t>(p) % MAX_NEW_ALIGNMENT == 0);
	} else {
		std::size_t bits = 0;
		for (std::size_t s = 2; s <= size; s *= 2)
			bits++;
		auto const addr = reinterpret_cast<std::uintptr_t>(p);
		SACO_ASSERT((addr >> bits << bits) == addr);
	}
#endif

	return p;
}

SACO_ALWAYS_INLINE void free_raw(void* mem) {
	::operator delete(mem);
}

struct raw_delete {
	void operator()(void* p) const {
		free_raw(p);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void call_dtor(void* p) {
	static_cast<T*>(p)->~T();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline constexpr bool is_power_of_two(T value) {
	return value != 0 && (value & (value - 1)) == 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace saco::detail
