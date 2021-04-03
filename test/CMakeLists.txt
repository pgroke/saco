include(../third-party/doctest/doctest.cmake)

set(common_test_headers
	"_common.h"
	"_poison_std_types_in_global_namespace.h"
	)

function(add_saco_test name)
	add_executable(${name} "${name}.cpp" ${common_test_headers} ${ARGN})
	target_link_libraries(${name} PRIVATE saco)
	target_include_directories(${name} PRIVATE ../third-party/doctest)
	doctest_discover_tests(${name} TEST_PREFIX "saco.${name}.")
endfunction()

add_saco_test(test_align)
add_saco_test(test_mctx)
add_saco_test(test_general)
add_saco_test(test_size_dispatcher)

add_saco_test(compile_test_saco_h)
add_saco_test(compile_test_shared_ptr_h)
