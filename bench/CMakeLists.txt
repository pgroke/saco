add_executable(bench
		bench.cpp
		nanobench.cpp
		)

target_link_libraries(bench
		PRIVATE
			saco
			${CMAKE_DL_LIBS}
		)

target_include_directories(bench
		PRIVATE ../third-party/nanobench
		)
