#include "_common.h"

#include <cstddef>
#include <cstdint>

#include "_poison_std_types_in_global_namespace.h"

#include <saco/saco.h>

namespace {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("align") {
	CHECK(saco::detail::align<1>(0) == 0);
	CHECK(saco::detail::align<2>(0) == 0);
	CHECK(saco::detail::align<4>(0) == 0);
	CHECK(saco::detail::align<8>(0) == 0);
	CHECK(saco::detail::align<16>(0) == 0);
	CHECK(saco::detail::align<32>(0) == 0);
	CHECK(saco::detail::align<4096>(0) == 0);

	CHECK(saco::detail::align<1>(1) == 1);
	CHECK(saco::detail::align<2>(1) == 2);
	CHECK(saco::detail::align<4>(1) == 4);
	CHECK(saco::detail::align<8>(1) == 8);
	CHECK(saco::detail::align<16>(1) == 16);
	CHECK(saco::detail::align<32>(1) == 32);
	CHECK(saco::detail::align<4096>(1) == 4096);

	CHECK(saco::detail::align<1>(8) == 8);
	CHECK(saco::detail::align<2>(8) == 8);
	CHECK(saco::detail::align<4>(8) == 8);
	CHECK(saco::detail::align<8>(8) == 8);
	CHECK(saco::detail::align<16>(8) == 16);
	CHECK(saco::detail::align<32>(8) == 32);
	CHECK(saco::detail::align<4096>(8) == 4096);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("align_and_add") {
	std::size_t offset;

	CHECK(saco::detail::align_and_add<1>(offset = 0, 1) == 0);
	CHECK(offset == 1);
	CHECK(saco::detail::align_and_add<1>(offset = 0, 1234) == 0);
	CHECK(offset == 1234);

	CHECK(saco::detail::align_and_add<2>(offset = 0, 1) == 0);
	CHECK(offset == 1);
	CHECK(saco::detail::align_and_add<2>(offset = 0, 1234) == 0);
	CHECK(offset == 1234);

	CHECK(saco::detail::align_and_add<4096>(offset = 0, 1) == 0);
	CHECK(offset == 1);
	CHECK(saco::detail::align_and_add<4096>(offset = 0, 1234) == 0);
	CHECK(offset == 1234);

	CHECK(saco::detail::align_and_add<1>(offset = 1, 1) == 1);
	CHECK(offset == 1 + 1);
	CHECK(saco::detail::align_and_add<1>(offset = 1, 1234) == 1);
	CHECK(offset == 1 + 1234);

	CHECK(saco::detail::align_and_add<2>(offset = 1, 1) == 2);
	CHECK(offset == 2 + 1);
	CHECK(saco::detail::align_and_add<2>(offset = 1, 1234) == 2);
	CHECK(offset == 2 + 1234);

	CHECK(saco::detail::align_and_add<4096>(offset = 1, 1) == 4096);
	CHECK(offset == 4096 + 1);
	CHECK(saco::detail::align_and_add<4096>(offset = 1, 1234) == 4096);
	CHECK(offset == 4096 + 1234);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace
