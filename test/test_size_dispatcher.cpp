#include "_common.h"

#include <cstddef>
#include <cstdint>

#include "_poison_std_types_in_global_namespace.h"

#include <saco/xsize_dispatcher.h>

namespace {

struct TestObject {
	std::size_t bucketSize;
};

template <std::size_t SIZE>
struct TestFactoryFn {
	TestObject operator()() const {
		return {SIZE};
	}
};

template <class Dispatcher>
void test_size_dispatcher() {
	static constexpr std::size_t MAX_SIZE = Dispatcher::MAX_SIZE;
	for (std::size_t size = 1; size < MAX_SIZE; size++) {
		TestObject const obj = Dispatcher::template dispatch<TestFactoryFn>(size);
		CHECK(obj.bucketSize >= size);
		CHECK(obj.bucketSize <= std::max<std::size_t>(64, 2 * size));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("size_dispatcher_switch") {
	test_size_dispatcher<saco::detail::size_dispatcher_switch>();
}

TEST_CASE("size_dispatcher_nested_if") {
	test_size_dispatcher<saco::detail::size_dispatcher_nested_if>();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace
