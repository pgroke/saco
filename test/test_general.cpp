#include "_common.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "_poison_std_types_in_global_namespace.h"

#include <saco/saco.h>

namespace {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SACO_NOINLINE void throw_out_of_range() {
	throw std::runtime_error("meh");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class Offset>
class offset_ptr_base {
	static_assert(std::is_integral_v<Offset>);
	static_assert(std::is_signed_v<Offset>);

public:
	offset_ptr_base(offset_ptr_base&&) = delete;

	offset_ptr_base() : m_offset{0} {
	}

	explicit offset_ptr_base(T* p) : m_offset{encode(p)} {
	}

	explicit operator bool() const {
		return m_offset != 0;
	}

	bool operator!() const {
		return m_offset == 0;
	}

	T* get() const {
		return decode();
	}

	T* operator->() const {
		return decode();
	}

	T& operator*() const {
		return *decode();
	}

private:
	T* decode() const {
		auto const my_addr = reinterpret_cast<std::uintptr_t>(&m_offset);
		auto const p_addr = my_addr + m_offset;
		return m_offset ? reinterpret_cast<T*>(p_addr) : nullptr;
	}

	Offset encode(T* p) {
		auto const p_addr = reinterpret_cast<std::uintptr_t>(p);
		auto const my_addr = reinterpret_cast<std::uintptr_t>(&m_offset);
		auto const offset = p ? static_cast<std::ptrdiff_t>(p_addr - my_addr) : 0;
		if (SACO_UNLIKELY(offset < std::numeric_limits<Offset>::min() || offset > std::numeric_limits<Offset>::max()))
			throw_out_of_range();
		return static_cast<Offset>(offset);
	}

	Offset m_offset;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class Base, bool IS_TRIVIALLY_DESTRUCTIBLE>
class inner_ptr_dtor_impl;

template <class T>
using target_type = std::decay_t<decltype(*std::declval<T>())>;

template <class Base>
class inner_ptr_dtor_impl<Base, true> : public Base {
	static_assert(std::is_trivially_destructible_v<target_type<Base>>);
	using Base::Base;
};

template <class Base>
class inner_ptr_dtor_impl<Base, false> : public Base {
public:
	using Base::Base;
	~inner_ptr_dtor_impl() {
		if (*this) {
			using T = target_type<Base>;
			this->get()->~T();
		}
	}
};

template <class T, class Offset>
class unique_offset_ptr : public inner_ptr_dtor_impl<offset_ptr_base<T, Offset>, std::is_trivially_destructible_v<T>> {
	using _base = inner_ptr_dtor_impl<offset_ptr_base<T, Offset>, std::is_trivially_destructible_v<T>>;
	using _base::_base;
};

template <class T>
using unique_o8_ptr = unique_offset_ptr<T, std::int8_t>;
template <class T>
using unique_o16_ptr = unique_offset_ptr<T, std::int16_t>;
template <class T>
using unique_o32_ptr = unique_offset_ptr<T, std::int32_t>;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct bar {
	static thread_local std::uint32_t tls_instance_count;

	bar() {
		signature[0] = '^';
		signature[1] = 'b';
		signature[2] = 'a';
		signature[3] = 'r';
		signature[4] = 'B';
		signature[5] = 'A';
		signature[6] = 'R';
		signature[7] = '$';
		tls_instance_count++;
	}

	~bar() {
		tls_instance_count--;
	}

	char signature[8];
};

thread_local std::uint32_t bar::tls_instance_count{0};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct foo {
	static thread_local std::uint32_t tls_instance_count;

	foo(bar* bars, std::size_t bar_count, bar* bar2) : bars{bars}, bar_count{bar_count}, bar2{bar2} {
		tls_instance_count++;
	}

	~foo() {
		tls_instance_count--;
		for (std::size_t i = 0; i < bar_count; i++)
			bars[i].~bar();
	}

	char s0 = '0';
	bar* bars;
	std::size_t bar_count;
	char s1 = '1';
	unique_o16_ptr<bar> bar2;
};

thread_local std::uint32_t foo::tls_instance_count{0};

#define CHECK_BAR_SIGNATURE(bar)          \
	do {                                  \
		CHECK((bar).signature[0] == '^'); \
		CHECK((bar).signature[1] == 'b'); \
		CHECK((bar).signature[2] == 'a'); \
		CHECK((bar).signature[3] == 'r'); \
		CHECK((bar).signature[4] == 'B'); \
		CHECK((bar).signature[5] == 'A'); \
		CHECK((bar).signature[6] == 'R'); \
		CHECK((bar).signature[7] == '$'); \
	} while (false)

} // namespace

template <>
struct saco::builder<foo> {
	template <class Context, class... Args>
	static foo* build(void* memory, Context& ctx, std::size_t bar_count) {
		[[maybe_unused]] bar* bars = saco::place<bar[]>(bar_count, ctx);
		[[maybe_unused]] bar* bar2 = saco::place<bar>(ctx);

		if SACO_IF_CONSTRUCT_CONTEXT (Context)
			return ::new (memory) foo{bars, bar_count, bar2};
		else
			return nullptr;
	}
};

namespace {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("general-1") {
	foo::tls_instance_count = 0;
	bar::tls_instance_count = 0;

	{
		auto const f = saco::build_unique<foo>(3);
		CHECK(foo::tls_instance_count == 1);
		CHECK(bar::tls_instance_count == 4);
		CHECK(f->s0 == '0');
		CHECK(f->s1 == '1');
		REQUIRE(f->bars != nullptr);
		REQUIRE(f->bar_count == 3);

		CHECK_BAR_SIGNATURE(f->bars[0]);
		CHECK_BAR_SIGNATURE(f->bars[1]);
		CHECK_BAR_SIGNATURE(f->bars[2]);
		CHECK_BAR_SIGNATURE(*f->bar2);
	}

	CHECK(foo::tls_instance_count == 0);
	CHECK(bar::tls_instance_count == 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("alloc_raw") {
	for (std::size_t s = 0; s < 512; s++)
		for (std::size_t i = 0; i < 1000; i++)
			saco::detail::free_raw(saco::detail::alloc_raw(s));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace
