#pragma once

#include <saco/saco.h>
#include <saco/xsize_dispatcher.h>

namespace saco::detail {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct shared_buffer_header {
	explicit shared_buffer_header(void* object) : object{object}, dtor_fn{nullptr} {
	}

	~shared_buffer_header() {
		if (dtor_fn)
			dtor_fn(object);
	}

	void (*dtor_fn)(void*);
	void* object;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <bool HAS_TRIVIAL_DTOR>
struct set_dtor_fn_impl;

template <>
struct set_dtor_fn_impl<true> {
	template <class T>
	static void set_dtor_fn(shared_buffer_header&) {
		// no need to call trivial dtor
	}
};

template <>
struct set_dtor_fn_impl<false> {
	template <class T>
	static void set_dtor_fn(shared_buffer_header& sbh) {
		sbh.dtor_fn = &detail::call_dtor<T>;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <std::size_t N>
struct shared_buffer : shared_buffer_header {
	shared_buffer() : shared_buffer_header(&storage) {
	}

	alignas(MAX_NEW_ALIGNMENT) byte storage[N];
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct shared_alloc_impl {
	template <std::size_t OBJECT_SIZE>
	struct shared_buffer_factory {
		std::shared_ptr<shared_buffer_header> operator()() const {
			if SACO_IF_CONSTEXPR (OBJECT_SIZE > 0)
				return std::make_shared<shared_buffer<OBJECT_SIZE>>();
			else
				return std::make_shared<shared_buffer<1>>();
		}
	};

	using dispatcher = size_dispatcher_nested_if;
	static constexpr std::size_t MAX_SIZE = dispatcher::MAX_SIZE;

	static SACO_NOINLINE std::shared_ptr<shared_buffer_header> alloc(std::size_t s) {
		return dispatcher::dispatch<shared_buffer_factory>(s);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace saco::detail

namespace saco {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class... Args>
std::shared_ptr<T> build_shared(Args&&... args) {
	static_assert(alignof(T) <= detail::MAX_NEW_ALIGNMENT);
	// measure
	measure_context mctx;
	saco::place<T>(mctx, std::as_const(args)...);

	// check size limit & allocate memory
	std::size_t const alloc_size = mctx.required_size();
	if (alloc_size > detail::shared_alloc_impl::MAX_SIZE)
		return build_unique<T>(std::forward<Args>(args)...);
	std::shared_ptr<detail::shared_buffer_header> sp = detail::shared_alloc_impl::alloc(alloc_size);

	// construct
	construct_context cctx{sp->object, alloc_size};
	[[maybe_unused]] auto const original_address = sp->object;
	sp->object = saco::place<T>(cctx, std::forward<Args>(args)...);
	detail::set_dtor_fn_impl<std::has_virtual_destructor_v<T>>::template set_dtor_fn<T>(*sp);
	SACO_ASSERT(sp->object == original_address);
	return std::shared_ptr<T>(std::move(sp), static_cast<T*>(sp->object));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace saco
