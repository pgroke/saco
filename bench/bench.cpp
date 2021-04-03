#include <string.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <nanobench.h>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#if defined(__linux__) || defined(__unix__)
#include <dlfcn.h>
#endif

namespace force_ambiguity {
struct dummy {};
typedef dummy size_t;
typedef dummy ptrdiff_t;
typedef dummy int8_t;
typedef dummy int16_t;
typedef dummy int32_t;
typedef dummy int64_t;
typedef dummy uint8_t;
typedef dummy uint16_t;
typedef dummy uint32_t;
typedef dummy uint64_t;
} // namespace force_ambiguity

using namespace force_ambiguity;

#include <saco/saco.h>
#include <saco/shared_ptr.h>

struct saco_foo {
	std::size_t n;
	int* p;
	std::string_view sv1;
	std::string_view sv2;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class Char, class Traits, class Context>
std::basic_string_view<Char, Traits> place_string_view(Context& ctx, std::basic_string_view<Char, Traits> sv) {
	[[maybe_unused]] Char* const mem = saco::place_for_overwrite<Char[]>(sv.size() + 1, ctx);

	if SACO_IF_CONSTRUCT_CONTEXT (Context) {
		Traits::copy(mem, sv.data(), sv.size());
		mem[sv.size()] = 0;
		return {mem, sv.size()};
	} else {
		return {};
	}
}

template <class Char, class Traits, class Context>
std::basic_string_view<Char, Traits> place_string_view(Context& ctx, std::basic_string<Char, Traits> const& str) {
	return place_string_view(ctx, std::basic_string_view<Char, Traits>(str));
}

template <class Char, class Context>
std::basic_string_view<Char> place_string_view(Context& ctx, Char const* str) {
	return place_string_view(ctx, str ? std::basic_string_view<Char>{str} : std::basic_string_view<Char>());
}

template <class Char, class Context>
std::basic_string_view<Char> place_string_view(Context& ctx, Char const* data, std::size_t count) {
	return place_string_view(ctx, std::basic_string_view<Char>{data, count});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <>
struct saco::builder<saco_foo> {
	template <class Context, class... Args>
	static saco_foo* build(void* memory, Context& ctx, std::size_t n, std::string_view sv1, std::string_view sv2) {
		[[maybe_unused]] auto const s1 = place_string_view(ctx, sv1);
		[[maybe_unused]] auto const s2 = place_string_view(ctx, sv2);
		[[maybe_unused]] int* const p = saco::place<int[]>(n, ctx);

		if SACO_IF_CONSTRUCT_CONTEXT (Context)
			return ::new (memory) saco_foo{n, p, s1, s2};
		else
			return nullptr;
	}
};

#if 1
struct classic_foo {
	classic_foo(std::size_t n, std::string_view sv1, std::string_view sv2) :
			n(n),
			p(n ? std::make_unique<int[]>(n) : nullptr),
			s1(sv1),
			s2(sv2) {
	}

	std::size_t n;
	std::unique_ptr<int[]> p;
	std::string s1;
	std::string s2;
};
#else
struct classic_foo {
	classic_foo(std::size_t n, std::string_view sv1, std::string_view sv2) : p(n), s1(sv1), s2(sv2) {
	}

	std::vector<int> p;
	std::string s1;
	std::string s2;
};
#endif

#if 0
size_t const array_size = 10;
size_t const count = 1000;
std::string const s1 = "My test string 111-222-333-444-555-666-777-888 111-222-333-444-555-666-777-888.";
std::string const s2 = "My other, slightly longer test string 111-222-333-444-555-666-777-888 111-222-333-444-555-666-777-888 111-222-333-444-555-666-777-888.";
#else
std::size_t const array_size = 10;
std::size_t const count = 10;
std::string const s1 = "s1xxxxxxxxxxxxxxxxxxxxxxxxxxx";
std::string const s2 = "s2xxxxxxxxxxxxxxxxxxxxxxxxxxx";
#endif

auto Bench(std::uint32_t minIterations) {
	return ankerl::nanobench::Bench()
			.batch(count)
			.minEpochIterations(minIterations)
			.warmup(minIterations * 10)
			.relative(true);
}

SACO_NOINLINE void classic_unique(ankerl::nanobench::Bench& bench) {
	std::vector<std::unique_ptr<classic_foo>> buf;
	buf.reserve(count);
	bench.run("classic unique", [&] {
		for (std::size_t i = 0; i < count; i++)
			buf.push_back(std::make_unique<classic_foo>(array_size, s1, s2));
		ankerl::nanobench::doNotOptimizeAway(buf.data());
		buf.clear();
	});
}

SACO_NOINLINE void saco_unique(ankerl::nanobench::Bench& bench) {
	std::vector<saco::unique_ptr<saco_foo>> buf;
	buf.reserve(count);
	bench.run("saco unique", [&] {
		for (std::size_t i = 0; i < count; i++)
			buf.push_back(saco::build_unique<saco_foo>(array_size, s1, s2));
		ankerl::nanobench::doNotOptimizeAway(buf.data());
		buf.clear();
	});
}

SACO_NOINLINE void classic_shared(ankerl::nanobench::Bench& bench) {
	std::vector<std::shared_ptr<classic_foo>> buf;
	buf.reserve(count);
	bench.run("classic shared", [&] {
		for (std::size_t i = 0; i < count; i++)
			buf.push_back(std::make_shared<classic_foo>(array_size, s1, s2));
		ankerl::nanobench::doNotOptimizeAway(buf.data());
		buf.clear();
	});
}

SACO_NOINLINE void saco_shared_from_unique(ankerl::nanobench::Bench& bench) {
	std::vector<std::shared_ptr<saco_foo>> buf;
	buf.reserve(count);
	bench.run("saco shared-from-unique", [&] {
		for (std::size_t i = 0; i < count; i++)
			buf.push_back(saco::build_unique<saco_foo>(array_size, s1, s2));
		ankerl::nanobench::doNotOptimizeAway(buf.data());
		buf.clear();
	});
}

SACO_NOINLINE void saco_shared(ankerl::nanobench::Bench& bench) {
	std::vector<std::shared_ptr<saco_foo>> buf;
	buf.reserve(count);
	bench.run("saco shared", [&] {
		for (std::size_t i = 0; i < count; i++)
			buf.push_back(saco::build_shared<saco_foo>(array_size, s1, s2));
		ankerl::nanobench::doNotOptimizeAway(buf.data());
		buf.clear();
	});
}

static constexpr std::size_t vec_min_length = 0;
static constexpr std::size_t vec_random_length = 0;

#if 0
static constexpr std::size_t s1_min_length = 50;
static constexpr std::size_t s2_min_length = 50;
static constexpr std::size_t s1_random_length = 150;
static constexpr std::size_t s2_random_length = 150;
#else
static constexpr std::size_t s1_min_length = 100;
static constexpr std::size_t s2_min_length = 100;
static constexpr std::size_t s1_random_length = 100;
static constexpr std::size_t s2_random_length = 100;
#endif

std::string long_str1(s1_min_length + s1_random_length, 'a');
std::string long_str2(s2_min_length + s2_random_length, 'b');
#define RAND_ARGS                                                                                                     \
	vec_min_length + rng.bounded(vec_random_length + 1),                                                              \
			std::string_view{long_str1.data(), s1_min_length + rng.bounded(s1_random_length + 1)}, std::string_view { \
		long_str2.data(), s2_min_length + rng.bounded(s2_random_length + 1)                                           \
	}

SACO_NOINLINE void randlen_classic_unique(ankerl::nanobench::Bench& bench) {
	std::vector<std::unique_ptr<classic_foo>> buf;
	buf.reserve(count);
	ankerl::nanobench::Rng rng{12345};
	bench.run("randlen classic unique", [&] {
		for (std::size_t i = 0; i < count; i++)
			buf.push_back(std::make_unique<classic_foo>(RAND_ARGS));
		ankerl::nanobench::doNotOptimizeAway(buf.data());
		buf.clear();
	});
}

SACO_NOINLINE void randlen_saco_unique(ankerl::nanobench::Bench& bench) {
	std::vector<saco::unique_ptr<saco_foo>> buf;
	buf.reserve(count);
	ankerl::nanobench::Rng rng{12345};
	bench.run("randlen saco unique", [&] {
		for (std::size_t i = 0; i < count; i++)
			buf.push_back(saco::build_unique<saco_foo>(RAND_ARGS));
		ankerl::nanobench::doNotOptimizeAway(buf.data());
		buf.clear();
	});
}

SACO_NOINLINE void randlen_classic_shared(ankerl::nanobench::Bench& bench) {
	std::vector<std::shared_ptr<classic_foo>> buf;
	buf.reserve(count);
	ankerl::nanobench::Rng rng{12345};
	bench.run("randlen classic shared", [&] {
		for (std::size_t i = 0; i < count; i++)
			buf.push_back(std::make_shared<classic_foo>(RAND_ARGS));
		ankerl::nanobench::doNotOptimizeAway(buf.data());
		buf.clear();
	});
}

SACO_NOINLINE void randlen_saco_shared_from_unique(ankerl::nanobench::Bench& bench) {
	std::vector<std::shared_ptr<saco_foo>> buf;
	buf.reserve(count);
	ankerl::nanobench::Rng rng{12345};
	bench.run("randlen saco shared-from-unique", [&] {
		for (std::size_t i = 0; i < count; i++)
			buf.push_back(saco::build_unique<saco_foo>(RAND_ARGS));
		ankerl::nanobench::doNotOptimizeAway(buf.data());
		buf.clear();
	});
}

SACO_NOINLINE void randlen_saco_shared(ankerl::nanobench::Bench& bench) {
	std::vector<std::shared_ptr<saco_foo>> buf;
	buf.reserve(count);
	ankerl::nanobench::Rng rng{12345};
	bench.run("randlen saco shared", [&] {
		for (std::size_t i = 0; i < count; i++)
			buf.push_back(saco::build_shared<saco_foo>(RAND_ARGS));
		ankerl::nanobench::doNotOptimizeAway(buf.data());
		buf.clear();
	});
}

SACO_NOINLINE void bench_malloc(ankerl::nanobench::Bench& bench, std::size_t size) {
	void* volatile vpv;
	bench.run("malloc(" + std::to_string(size) + ")", [&] {
		for (std::size_t i = 0; i < count; i++) {
			auto p = ::operator new(size);
			vpv = p;
			ankerl::nanobench::doNotOptimizeAway(p);
			::operator delete(p);
		}
	});
}

template <std::size_t S>
struct return_size {
	static volatile std::size_t s;
	SACO_NOINLINE std::size_t operator()() const {
		return s;
	}
};

template <std::size_t S>
volatile std::size_t return_size<S>::s = S;

std::size_t g_malloc_count = 0;

int main() {
#if 0
	int bucket = 0;
	for (size_t s = 0; s < 2200; s++) {
		int next_bucket = compute_bucket(s);
		if (next_bucket != bucket) {
			std::cout << "case " << bucket << ": return Fn<" << s - 1 << ">{}();" << std::endl;
			bucket = next_bucket;
		}
	}
#endif

	{
		for (std::size_t s = 20; s < 1000;) {
			std::string my_str(s, 'a');
			g_malloc_count = 0;
			auto sp = saco::build_shared<saco_foo>(20u, "s1", my_str);
			auto allocs = g_malloc_count;
			std::cout << "build_shared<saco_foo(" << s << ")> allocations: " << allocs << std::endl;

			if (s < 300)
				s += 20;
			else
				s += 100;
		}
	}

	{
		for (std::size_t s = 20; s < 1000;) {
			std::string my_str(s, 'a');
			g_malloc_count = 0;
			auto sp = std::make_shared<classic_foo>(20u, "s1", my_str);
			auto allocs = g_malloc_count;
			std::cout << "make_shared<classic_foo(" << s << ")> allocations: " << allocs << std::endl;

			if (s < 300)
				s += 20;
			else
				s += 100;
		}
	}
#if 1
	static constexpr std::size_t num_allocs = 1'000'000;
	static constexpr std::size_t num_frees = 100'000;
	static constexpr std::uint32_t alloc_range = 8192;

	std::vector<void*> allocs;
	allocs.reserve(num_allocs);
	ankerl::nanobench::Rng rng{42};
	for (std::size_t i = 0; i < num_allocs; i++) {
		std::uint32_t range = 8;
		while (range * 2 <= alloc_range) {
			if (rng.bounded(1024) < 512)
				break;
			range *= 2;
		}

		allocs.push_back(::operator new(rng.bounded(range)));
	}

	for (std::size_t i = 0; i < num_allocs; i++) {
		auto const n = rng.bounded(num_allocs);
		if (allocs[n])
			::operator delete(allocs[n]);
		allocs[n] = 0;
	}

	void* volatile pv;
	pv = allocs.data();
#endif


#if 0
	{
		auto b = Bench(10000);
		for (std::size_t size = 32; size <= 1 << 16;) {
			bench_malloc(b, size);
			std::size_t n;
			n = size;
			n |= n >> 1;
			n |= n >> 2;
			n |= n >> 4;
			n |= n >> 8;
			n |= n >> 16;
			n++;
			n /= 8;
			if (n < 8)
				size += 8;
			else
				size += n;
		}
	}
#endif

	{
		auto b = Bench(10000);
		classic_unique(b);
		saco_unique(b);
	}

	{
		auto b = Bench(10000);
		classic_shared(b);
		saco_shared_from_unique(b);
		saco_shared(b);
	}
	{
		auto b = Bench(10000);
		randlen_classic_unique(b);
		randlen_saco_unique(b);
	}

	{
		auto b = Bench(10000);
		randlen_classic_shared(b);
		randlen_saco_shared_from_unique(b);
		randlen_saco_shared(b);
	}
}

#if 0
void* operator new(std::size_t count) {
	g_malloc_count++;
	void* p = malloc(count);
	if (SACO_UNLIKELY(!p))
		throw std::bad_alloc();
#if 0
	volatile int a = 1234567;
	volatile int b = 33;
	volatile int c;
	for (std::size_t i = 0; i < 500; i++)
		c = a / b;
#endif
	return p;
}
#endif

#if 0 && (defined(__linux__) || defined(__unix__))

#include <dlfcn.h>

extern "C" {

	typedef void* (*malloc_type)(std::size_t);
	static malloc_type real_malloc = NULL;

	extern void* malloc(std::size_t size)
	{
		if (size == 0)
			return NULL;

		malloc_type m = real_malloc;

		if (!m) {
			m = (malloc_type)dlsym(RTLD_NEXT, "malloc");
			if (!m) {
				exit(EXIT_FAILURE);
			}
		}

		g_malloc_count++;
		return (*m)(size);
	}

	static __attribute__((constructor)) void init(void)
	{
		real_malloc = (malloc_type)dlsym(RTLD_NEXT, "malloc");
		if (!real_malloc) {
			exit(EXIT_FAILURE);
		}
	}

}

#endif
