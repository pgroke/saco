#pragma once

#include <saco/xcore.h>
#include <saco/xutility.h>

#include <memory>
#include <type_traits>
#include <utility>

namespace saco {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
struct saco_delete {
	void operator()(T* p) const {
		static_assert(sizeof(T) > 0, "type must be complete");
		p->~T();
		detail::free_raw(p);
	}
};

template <class T>
using unique_ptr = std::unique_ptr<T, saco_delete<T>>;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace detail {

template <std::size_t ALIGN, class Offset>
SACO_ALWAYS_INLINE Offset align(Offset ref_offset) {
	static_assert(is_power_of_two(ALIGN));
	static constexpr std::size_t MASK = ALIGN - 1;
	return (ref_offset + MASK) & ~MASK;
}

template <std::size_t ALIGN, class Offset>
SACO_ALWAYS_INLINE Offset align_and_add(Offset& ref_offset, std::size_t size) {
	Offset const aligned = align<ALIGN>(ref_offset);
	ref_offset = aligned + size;
	return aligned;
}

} // namespace detail

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class measure_context final {
public:
	static constexpr bool is_construct_context = false;

	measure_context(measure_context&&) = delete;
	SACO_ALWAYS_INLINE measure_context() = default;

	template <class T>
	SACO_ALWAYS_INLINE void* allocate_space() {
		static_assert(sizeof(T) % alignof(T) == 0);
		return allocate_space_0<alignof(T)>(sizeof(T));
	}

	template <class T>
	SACO_ALWAYS_INLINE void* allocate_space(std::size_t count) {
		static_assert(sizeof(T) % alignof(T) == 0);
		return allocate_space_0<alignof(T)>(sizeof(T) * count);
	}

	std::size_t required_size() const {
		return this->m_offset + m_extra_padding;
	}

private:
	template <std::size_t ALIGN>
	SACO_ALWAYS_INLINE void* allocate_space_0(std::size_t size) {
		if SACO_IF_CONSTEXPR (ALIGN <= detail::MAX_NEW_ALIGNMENT)
			detail::align_and_add<ALIGN>(m_offset, size);
		else
			allocate_space_1<ALIGN>(size);
		return nullptr;
	}

	template <std::size_t ALIGN>
	void allocate_space_1(std::size_t size) {
		if (ALIGN <= m_free_alignment)
			detail::align_and_add<ALIGN>(m_offset, size);
		else
			allocate_over_aligned<ALIGN>(size);
	}

	template <std::size_t ALIGN>
	void allocate_over_aligned(std::size_t size) {
		SACO_ASSERT_MSG(m_offset != 0, "head must not be over-aligned");

		auto const fa_mask = m_free_alignment - 1u;
		m_offset = (m_offset + fa_mask) & ~fa_mask;

		std::size_t const old_offset = m_offset;
		std::size_t const offset = detail::align_and_add<ALIGN>(m_offset, size);

		// amount of padding already accounted for my increased m_offset
		std::size_t const padding = offset - old_offset;
		// amount of extra padding that may be required to achieve ALIGN
		std::size_t const extra_padding = ALIGN - padding - m_free_alignment;

		m_extra_padding += extra_padding;
		m_free_alignment = ALIGN;
	}

	static_assert(detail::is_power_of_two(detail::MAX_NEW_ALIGNMENT));

	std::size_t m_offset{0};
	std::size_t m_free_alignment{detail::MAX_NEW_ALIGNMENT};
	std::size_t m_extra_padding{0};
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class construct_context final {
public:
	static constexpr bool is_construct_context = true;

	construct_context(construct_context&&) = delete;

	SACO_ALWAYS_INLINE construct_context(void* mem, std::size_t size) :
			m_current{reinterpret_cast<std::uintptr_t>(mem)},
			m_end{m_current + size} {
	}

	SACO_ALWAYS_INLINE void* current() {
		return reinterpret_cast<void*>(m_current);
	}

	template <class T>
	SACO_ALWAYS_INLINE void* allocate_space() {
		static_assert(sizeof(T) % alignof(T) == 0);
		return allocate_space_0<alignof(T)>(sizeof(T));
	}

	template <class T>
	SACO_ALWAYS_INLINE void* allocate_space(std::size_t count) {
		static_assert(sizeof(T) % alignof(T) == 0);
		return allocate_space_0<alignof(T)>(sizeof(T) * count);
	}

private:
	template <std::size_t ALIGN>
	void* allocate_space_0(std::size_t size) {
		auto const address = detail::align_and_add<ALIGN>(m_current, size);
		SACO_ASSERT(m_current <= m_end);
		return reinterpret_cast<void*>(address);
	}

	std::uintptr_t m_current;
	std::uintptr_t m_end;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
struct builder;

template <class T, std::size_t N>
struct builder<T[N]> {};
template <class T>
struct builder<T[]> {};

template <class T>
struct builder {
	template <class Context, class... Args>
	static SACO_ALWAYS_INLINE T* build(void* memory, Context& ctx, Args&&... args) {
		if SACO_IF_CONSTRUCT_CONTEXT (Context)
			return ::new (memory) T(std::forward<Args>(args)...);
		else
			return nullptr;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class Context, class... Args, SACO_REQUIRES_NON_ARRAY(T)>
T* place(Context& ctx, Args&&... args) {
	auto const memory = ctx.template allocate_space<T>();
	return builder<T>::build(memory, ctx, std::forward<Args>(args)...);
}

template <class T, class Context, SACO_REQUIRES_NON_ARRAY(T)>
T* place_for_overwrite(Context& ctx) {
	[[maybe_unused]] auto const memory = ctx.template allocate_space<T>();
	if SACO_IF_CONSTRUCT_CONTEXT (Context)
		return ::new (memory) T;
	else
		return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class A, class Context, class... Args, SACO_REQUIRES_UB_ARRAY(A)>
std::remove_extent_t<A>* place(std::size_t count, Context& ctx, Args&&... args) {
	using T = std::remove_extent_t<A>;

	if (count == 0)
		return nullptr;

	[[maybe_unused]] auto const memory = ctx.template allocate_space<T>(count);

	if SACO_IF_CONSTRUCT_CONTEXT (Context) {
		auto const ts = static_cast<T*>(memory); // bless
		for (std::size_t i = 0; i < count; i++)
			builder<T>::build(&ts[i], ctx, std::forward<Args>(args)...);
		return ts;
	} else
		return nullptr;
}

template <class A, class Context, SACO_REQUIRES_UB_ARRAY(A)>
std::remove_extent_t<A>* place_for_overwrite(std::size_t count, Context& ctx) {
	using T = std::remove_extent_t<A>;

	if (count == 0)
		return nullptr;

	[[maybe_unused]] auto const memory = ctx.template allocate_space<T>(count);

	if SACO_IF_CONSTRUCT_CONTEXT (Context) {
		auto const ts = static_cast<T*>(memory); // bless
		for (std::size_t i = 0; i < count; i++)
			::new (&ts[i]) T;
		return ts;
	} else
		return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class... Args>
unique_ptr<T> build_unique(Args&&... args) {
	static_assert(alignof(T) <= detail::MAX_NEW_ALIGNMENT);
	// measure
	measure_context mctx;
	saco::place<T>(mctx, std::as_const(args)...);
	std::size_t const required_size = mctx.required_size();

	// allocate raw memory
	std::unique_ptr<void, detail::raw_delete> raw_memory(detail::alloc_raw(required_size));

	// construct
	construct_context cctx{raw_memory.get(), required_size};
	unique_ptr<T> obj(saco::place<T>(cctx, std::forward<Args>(args)...));
	[[maybe_unused]] auto const rmem = raw_memory.release();
	SACO_ASSERT(obj.get() == static_cast<void*>(rmem));
	return obj;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace saco
