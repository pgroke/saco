cmake_minimum_required(VERSION 3.12)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# TODO: add DESCRIPTION
# TODO: add HOMEPAGE_URL
# TODO: figure out how to best avoid duplicating the VERSION
project(
		saco
		VERSION 0.1
		)

#
# if (CMAKE_BUILD_TYPE MATCHES Debug)
# 	set(_is_debug ON)
# else ()
# 	set(_is_debug OFF)
# endif ()
# 
# option(INTERNAL_ASSERTIONS "enable internal assertions" ${_is_debug})
#

set(_headers
		${saco_SOURCE_DIR}/include/saco/saco.h
		${saco_SOURCE_DIR}/include/saco/shared_ptr.h
		)

set(_detail_headers
		${saco_SOURCE_DIR}/include/saco/xcore.h
		${saco_SOURCE_DIR}/include/saco/xsize_dispatcher.h
		${saco_SOURCE_DIR}/include/saco/xutility.h
		)

include(GNUInstallDirs)

add_library(saco INTERFACE)

cmake_policy(PUSH)
cmake_policy(SET CMP0076 OLD)
target_sources(
		saco
		INTERFACE
			$<BUILD_INTERFACE:${_headers}>
			$<BUILD_INTERFACE:${_detail_headers}>
		)
cmake_policy(POP)

target_include_directories(
		saco
		INTERFACE
			$<BUILD_INTERFACE:${saco_SOURCE_DIR}/include>
			$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
		)

target_compile_features(
	saco
	INTERFACE
		cxx_std_17
)

#
# tests
#

option(build_tests "build tests and benchmarks" OFF)

if (${build_tests} OR ("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}"))
    enable_testing()
	add_subdirectory(bench)
    add_subdirectory(test)
endif()

#
# install rules
#

set(_config_install_dir ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

set(_generated_dir ${CMAKE_CURRENT_BINARY_DIR}/generated)

set(_config_in_path		 ${saco_SOURCE_DIR}/${PROJECT_NAME}-config.cmake.in)
set(_config_path         ${_generated_dir}/${PROJECT_NAME}-config.cmake)
set(_config_version_path ${_generated_dir}/${PROJECT_NAME}-config-version.cmake)

set(_targets_export_name ${PROJECT_NAME}-targets)

set(_namespace ${PROJECT_NAME}::)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
		${_config_version_path}
		VERSION ${PROJECT_VERSION}
		COMPATIBILITY SameMajorVersion
		ARCH_INDEPENDENT
		)

configure_package_config_file(
		${_config_in_path}
		${_config_path}
		INSTALL_DESTINATION ${_config_install_dir}
		)

# define target
install(
		TARGETS saco
		EXPORT ${_targets_export_name}
		RUNTIME        DESTINATION ${CMAKE_INSTALL_BINDIR}
		ARCHIVE        DESTINATION ${CMAKE_INSTALL_LIBDIR}
		LIBRARY        DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PUBLIC_HEADER  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		)

# install include directory
install(
		DIRECTORY ${saco_SOURCE_DIR}/include/saco
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		)

# install cmake targets file
install(
		EXPORT ${_targets_export_name}
		NAMESPACE ${_namespace}
		DESTINATION ${_config_install_dir}
		)
