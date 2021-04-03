#include "_common.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>

#include "_poison_std_types_in_global_namespace.h"

#include <saco/saco.h>

namespace {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <std::size_t N>
struct char_array_struct {
	char arr[N];
};

template <std::size_t N>
struct alignas(N) aligned_struct {
	char ch;
};

static_assert(alignof(aligned_struct<1>) == 1);
static_assert(alignof(aligned_struct<2>) == 2);
static_assert(alignof(aligned_struct<4096>) == 4096);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("allocate_space-1") {
	{
		saco::measure_context mctx;
		mctx.allocate_space<char>();
		CHECK(mctx.required_size() == sizeof(char));
	}

	{
		saco::measure_context mctx;
		mctx.allocate_space<short>();
		CHECK(mctx.required_size() == sizeof(short));
	}

	{
		saco::measure_context mctx;
		mctx.allocate_space<int>();
		CHECK(mctx.required_size() == sizeof(int));
	}

	{
		saco::measure_context mctx;
		mctx.allocate_space<long>();
		CHECK(mctx.required_size() == sizeof(long));
	}

	{
		saco::measure_context mctx;
		mctx.allocate_space<long long>();
		CHECK(mctx.required_size() == sizeof(long long));
	}

	{
		saco::measure_context mctx;
		mctx.allocate_space<char_array_struct<123>>();
		CHECK(mctx.required_size() == sizeof(char_array_struct<123>));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("allocate_space-2") {
	{
		saco::measure_context mctx;
		mctx.allocate_space<char>();
		mctx.allocate_space<short>();
		CHECK(mctx.required_size() == alignof(short) + sizeof(short));
	}

	{
		saco::measure_context mctx;
		mctx.allocate_space<char>();
		mctx.allocate_space<int>();
		CHECK(mctx.required_size() == alignof(int) + sizeof(int));
	}

	{
		saco::measure_context mctx;
		mctx.allocate_space<char>();
		mctx.allocate_space<long>();
		CHECK(mctx.required_size() == alignof(long) + sizeof(long));
	}

	{
		saco::measure_context mctx;
		mctx.allocate_space<char>();
		mctx.allocate_space<char_array_struct<123>>();
		CHECK(mctx.required_size() == alignof(char_array_struct<123>) + sizeof(char_array_struct<123>));
	}

	{
		saco::measure_context mctx;
		mctx.allocate_space<char>();
		mctx.allocate_space<aligned_struct<1024>>();
		CHECK(mctx.required_size() == alignof(aligned_struct<1024>) + sizeof(aligned_struct<1024>));
	}

	{
		saco::measure_context mctx;
		mctx.allocate_space<char>();
		mctx.allocate_space<char>(1023);
		CHECK(mctx.required_size() == 1024);

		mctx.allocate_space<aligned_struct<1024>>();
		CHECK(mctx.required_size()
			  == 1024 + 1 * sizeof(aligned_struct<1024>) + alignof(aligned_struct<1024>)
						 - saco::detail::MAX_NEW_ALIGNMENT);

		mctx.allocate_space<aligned_struct<1024>>();
		CHECK(mctx.required_size()
			  == 1024 + 2 * sizeof(aligned_struct<1024>) + alignof(aligned_struct<1024>)
						 - saco::detail::MAX_NEW_ALIGNMENT);

		mctx.allocate_space<char>();
		mctx.allocate_space<aligned_struct<1024>>();
		CHECK(mctx.required_size()
			  == 1024 + 4 * sizeof(aligned_struct<1024>) + alignof(aligned_struct<1024>)
						 - saco::detail::MAX_NEW_ALIGNMENT);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct measure_op {
	saco::measure_context* const mctx;

	template <class T>
	void alloc(std::size_t count) {
		mctx->allocate_space<T>(count);
	}

	template <class T>
	void alloc() {
		mctx->allocate_space<T>();
	}
};

template <class S>
std::size_t measured_size(S sequence) {
	saco::measure_context mctx;
	sequence(measure_op{&mctx});
	return mctx.required_size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <std::size_t ALIGN, class Offset>
Offset reference_align(Offset ref_offset) {
	Offset o = ref_offset;
	o /= ALIGN;
	o *= ALIGN;
	if (o < ref_offset)
		o += ALIGN;

	REQUIRE(o >= ref_offset);
	REQUIRE(o % ALIGN == 0);
	return o;
}

template <std::size_t ALIGN, class Offset>
Offset reference_align_and_add(Offset& ref_offset, std::size_t size) {
	Offset o = reference_align<ALIGN>(ref_offset);
	ref_offset = o + size;
	return o;
}

struct ref_size_op {
	std::uintptr_t* const address;

	template <class T>
	void alloc(std::size_t count) {
		reference_align_and_add<alignof(T)>(*address, sizeof(T) * count);
	}

	template <class T>
	void alloc() {
		alloc<T>(1);
	}
};

template <class S>
std::size_t ref_size_at(S sequence, std::uintptr_t start_address) {
	INFO("ref size at address ", start_address);
	std::uintptr_t address = start_address;
	sequence(ref_size_op{&address});
	REQUIRE(address - start_address <= std::numeric_limits<std::size_t>::max());
	return static_cast<std::size_t>(address - start_address);
}

template <class S>
std::size_t ref_size(S sequence, std::size_t max_align) {
	std::size_t max_size = 0;
	for (std::size_t offset = 0; offset <= max_align; offset += saco::detail::MAX_NEW_ALIGNMENT)
		max_size = std::max(max_size, ref_size_at(sequence, offset + max_align));
	return max_size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct cctx_size_op {
	saco::construct_context* const cctx;

	template <class T>
	void alloc(std::size_t count) {
		cctx->allocate_space<T>(count);
	}

	template <class T>
	void alloc() {
		cctx->allocate_space<T>();
	}
};

template <class S>
std::size_t cctx_size_at(S sequence, std::size_t start_offset) {
	INFO("cctx size at address ", start_offset);
	static saco::byte memory[1024 * 1024];
	REQUIRE(start_offset < sizeof(memory) / 2);
	saco::construct_context cctx{&memory[start_offset], sizeof(memory) - start_offset};
	sequence(cctx_size_op{&cctx});
	return static_cast<saco::byte*>(cctx.current()) - &memory[start_offset];
}

template <class S>
std::size_t cctx_size(S sequence, std::size_t max_align) {
	std::size_t max_size = 0;
	for (std::size_t offset = 0; offset <= max_align; offset += saco::detail::MAX_NEW_ALIGNMENT)
		max_size = std::max(max_size, cctx_size_at(sequence, offset));
	return max_size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class S>
void check_sequence(int line, std::size_t max_align, S sequence) {
	CAPTURE(line);
	auto const ref = ref_size(sequence, max_align);
	auto const measured = measured_size(sequence);
	auto const required = cctx_size(sequence, max_align);
	CHECK(measured == ref);
	CHECK(required == ref);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class F0, class F1, class F2, class F3, class F4>
void check_sequence_permutations(int line, std::size_t max_align, F0 f0, F1 f1, F2 f2, F3 f3, F4 f4) {
	int order[]{1, 2, 3, 4};

	do {
		INFO("order := ", order[0], order[1], order[2], order[3]);
		check_sequence(line, max_align, [&f0, &f1, &f2, &f3, &f4, &order](auto op) {
			f0(op);
			for (int o : order) {
				switch (o) {
				case 1: f1(op); break;
				case 2: f2(op); break;
				case 3: f3(op); break;
				case 4: f4(op); break;
				}
			}
		});
	} while (std::next_permutation(std::begin(order), std::end(order)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("allocate_space-3") {
	static constexpr std::size_t max_align = 512;

#define ALLOC op.template alloc
#define CHECK_SEQUENCE(...) check_sequence(__LINE__, max_align, __VA_ARGS__)

	CHECK_SEQUENCE([](auto op) { ALLOC<char>(); });
	CHECK_SEQUENCE([](auto op) { ALLOC<int>(); });
	CHECK_SEQUENCE([](auto op) { ALLOC<long long>(); });
	CHECK_SEQUENCE([](auto op) { ALLOC<long double>(); });
	CHECK_SEQUENCE([](auto op) { ALLOC<std::max_align_t>(); });

	for (std::size_t array_size = 0; array_size < max_align; array_size += 7) {
		CAPTURE(array_size);
		check_sequence(__LINE__, max_align, [array_size](auto op) {
			ALLOC<char>();
			ALLOC<char>(array_size);
			ALLOC<char>();
		});
		check_sequence(__LINE__, max_align, [array_size](auto op) {
			ALLOC<char>();
			ALLOC<char>(array_size);
			ALLOC<int>();
		});
		check_sequence(__LINE__, max_align, [array_size](auto op) {
			ALLOC<char>();
			ALLOC<char>(array_size);
			ALLOC<std::max_align_t>();
		});
		//
		check_sequence(__LINE__, max_align, [array_size](auto op) {
			ALLOC<char>();
			ALLOC<char>(array_size);
			ALLOC<aligned_struct<32>>();
		});
		check_sequence(__LINE__, max_align, [array_size](auto op) {
			ALLOC<char>();
			ALLOC<char>(array_size);
			ALLOC<aligned_struct<64>>();
		});
		check_sequence(__LINE__, max_align, [array_size](auto op) {
			ALLOC<char>();
			ALLOC<char>(array_size);
			ALLOC<aligned_struct<128>>();
		});
		check_sequence(__LINE__, max_align, [array_size](auto op) {
			ALLOC<char>();
			ALLOC<char>(array_size);
			ALLOC<aligned_struct<256>>();
		});
		check_sequence(__LINE__, max_align, [array_size](auto op) {
			ALLOC<char>();
			ALLOC<char>(array_size);
			ALLOC<aligned_struct<512>>();
		});
		//
		check_sequence_permutations(
				__LINE__,
				max_align,
				[array_size](auto op) { ALLOC<char>(); },
				[array_size](auto op) { ALLOC<char>(array_size); },
				[array_size](auto op) { ALLOC<aligned_struct<32>>(); },
				[array_size](auto op) { ALLOC<aligned_struct<128>>(); },
				[array_size](auto op) { ALLOC<aligned_struct<512>>(); });
	}

#undef ALLOC
#undef CHECK_SEQUENCE
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace
