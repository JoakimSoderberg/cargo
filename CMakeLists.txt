#
# The MIT License (MIT)
#
# Copyright (c) 2015 Joakim Soderberg <joakim.soderberg@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
cmake_minimum_required (VERSION 2.8)
project(cargo C)


option(CARGO_DEBUG "Set cargo debug level" 0)
option(CARGO_TEST "Build unit tests" ON)
option(CARGO_EXAMPLE "Build example application that comes in cargo.c" ON)
option(CARGO_HELPER "Build cargo formatting helper" ON)
option(CARGO_COVERALLS "Generate coveralls data (CARGO_TEST must be turned on, and CMAKE_BUILD_MODE must be Debug)" OFF)
option(CARGO_EXTRA_EXAMPLES "Builds the extra examples under the examples/ directory" ON)
option(CARGO_WITH_MEMCHECK "Run unit tests in valgrind or dr.memory" ON)
option(CARGO_BUILD_SHARED_LIB "Build a shared library" ON)
option(CARGO_BUILD_STATIC_LIB "Build a static library" ON)
option(CARGO_SHUTUP "Don't output adding of tests and stuff" OFF)
option(CARGO_EXAMPLES_LINK_SHARED "Link the example programs with the shared library instead of static" OFF)

# Add project cmake modules to path.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

if (NOT CARGO_BUILD_SHARED_LIB AND NOT CARGO_BUILD_STATIC_LIB)
	message(FATAL_ERROR "You must build either shared or static lib, or both")
endif()

if (CARGO_TEST AND CARGO_COVERALLS)
    include(Coveralls)
    coveralls_turn_on_coverage()
endif()

# Set some nicer output dirs.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

# Get cargo version from header.
file(READ cargo.h HEADER_CONTENTS)
foreach(VER MAJOR MINOR PATCH)
	string(REGEX MATCH "define[ \t]+CARGO_${VER}_VERSION[ \t]+([0-9])" BLA ${HEADER_CONTENTS})
	set(CARGO_${VER}_VERSION ${CMAKE_MATCH_1})
endforeach()

set(CARGO_VERSION ${CARGO_MAJOR_VERSION}.${CARGO_MINOR_VERSION}.${CARGO_PATCH_VERSION})
message("Cargo version: ${CARGO_VERSION}")

# TODO: Add so version!

source_group("Headers" FILES cargo.h)
source_group("Sources" FILES cargo.c)
include_directories(${PROJECT_SOURCE_DIR})

message("Setting debug level to: ${CARGO_DEBUG}")

if (NOT CARGO_DEBUG)
	remove_definitions(-DCARGO_DEBUG)
else()
	add_definitions(-DCARGO_DEBUG=${CARGO_DEBUG})
endif()

if (MSVC)
    # Turn off Microsofts "security" warnings.
   add_definitions("/W3 /D_CRT_SECURE_NO_WARNINGS /wd4005 /wd4996 /nologo")
endif()

if (CARGO_BUILD_STATIC_LIB)
	add_library(cargo STATIC cargo.c cargo.h)
	install(TARGETS cargo
			DESTINATION "lib")
endif()

if (CARGO_BUILD_SHARED_LIB)
	if (POLICY CMP0042)
		cmake_policy(PUSH)
		cmake_policy(SET CMP0042 OLD)
	endif()

	add_library(cargo_shared SHARED cargo.c cargo.h)
	install(TARGETS cargo_shared
			DESTINATION "lib")

	if (UNIX)
		set_target_properties(cargo_shared PROPERTIES OUTPUT_NAME cargo)
	endif()

	if (POLICY CMP0042)
		cmake_policy(POP)
	endif()
endif()

set(CARGO_EXE_LIST )

# The internal example in cargo.c
if (CARGO_EXAMPLE)
	add_executable(cargo_example cargo.c cargo.h)
	set_target_properties(cargo_example PROPERTIES COMPILE_DEFINITIONS "CARGO_EXAMPLE=1 CARGO_NOLIB=1")
	list(APPEND CARGO_EXE_LIST cargo_example)
endif()

if (CARGO_EXTRA_EXAMPLES)
	file(GLOB EXTRA_EXAMPLE_FILES "${PROJECT_SOURCE_DIR}/examples/*.c")

	foreach(EXAMPLE_FILE ${EXTRA_EXAMPLE_FILES})
		get_filename_component(EXAMPLE_NAME "${EXAMPLE_FILE}" NAME_WE)
		add_executable(${EXAMPLE_NAME} "${EXAMPLE_FILE}" cargo.c)
		list(APPEND CARGO_EXE_LIST ${EXAMPLE_NAME})
	endforeach()
endif()

if (CARGO_HELPER)
	add_executable(cargo_helper cargo.c cargo.h)
	set_target_properties(cargo_helper PROPERTIES COMPILE_DEFINITIONS "CARGO_HELPER=1 CARGO_NOLIB=1")
	list(APPEND CARGO_EXE_LIST cargo_helper)
endif()

if (CARGO_TEST)
	ENABLE_TESTING()

	if (CARGO_WITH_MEMCHECK)
		if (WIN32)
			cmake_policy(PUSH)
			#cmake_policy(SET CMP0053 OLD) # Allow () in the var
			find_program(DRMEMORY_PROGRAM NAMES drmemory.exe PATHS "$ENV{ProgramFiles(x86)}/Dr. Memory/bin")
			cmake_policy(POP)

			if (NOT EXISTS ${DRMEMORY_PROGRAM})
				message(WARNING "Dr. Memory not found! Tests will be run without memory check. Turn this warning off by installing Dr. Memory or settings -DCARGO_WITH_MEMCHECK=OFF")
				set(CARGO_WITH_MEMCHECK OFF)
			else()
				message("Found valgrind: ${VALGRIND_PROGRAM}")
			endif()
		else()
			find_program(VALGRIND_PROGRAM NAMES valgrind)

			if (NOT EXISTS ${VALGRIND_PROGRAM})
				message(WARNING "Valgrind not found! Tests will be run without memory check. Turn this warning off by installing valgrind or settings -DCARGO_WITH_MEMCHECK=OFF")
				set(CARGO_WITH_MEMCHECK OFF)
			else()
				message("Found valgrind: ${VALGRIND_PROGRAM}")
			endif()
		endif()
	endif()

	if (CARGO_COVERALLS)
		# Create the coveralls target.
		coveralls_setup(
			${PROJECT_SOURCE_DIR}/cargo.c	# The source files.
			ON								# If we should upload.
			"${PROJECT_SOURCE_DIR}/cmake/") # (Optional) Alternate project cmake module path.
	endif()

	add_executable(cargo_tests cargo.c cargo.h)
	set_target_properties(cargo_tests PROPERTIES COMPILE_DEFINITIONS "CARGO_TEST=1 CARGO_NOLIB=1")

	# Look for all TEST_ functions in cargo.c, and add a test for each!
	# (We need to do this in two steps because CMake regexp sucks)
	file(STRINGS cargo.c CARGO_LINES)
	string(REGEX MATCHALL
				"CARGO_ADD_TEST\\(TEST_[a-zA-Z0-9_]+"
				TEST_LINES
				"${CARGO_LINES}")

	foreach (CARGO_LINE ${TEST_LINES})
		string(REGEX MATCH
				"TEST_[a-z0-9_]+"
				TEST_NAME
				"${CARGO_LINE}")

		if (TEST_NAME)
			if (NOT CARGO_SHUTUP)
				message("Adding test: '${TEST_NAME}'")
			endif()

			if (CARGO_WITH_MEMCHECK)
				if (WIN32)
					# TODO: This is hard coded to bin/Debug for now... Possible fix:
					# http://stackoverflow.com/questions/9467510/how-to-pass-ctest-configuration-type-using-add-test-in-cmake
					add_test("${TEST_NAME}_memchk" ${DRMEMORY_PROGRAM} -batch -show_reachable -exit_code_if_errors 5 -- ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/cargo_tests.exe --shortlist ${TEST_NAME})
				else()
					add_test("${TEST_NAME}_memchk" ${VALGRIND_PROGRAM} --gen-suppressions=all --suppressions=${PROJECT_SOURCE_DIR}/valgrind-suppress.txt --error-exitcode=5 --leak-check=full ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cargo_tests --shortlist ${TEST_NAME})
				endif()
			else()
				add_test("${TEST_NAME}" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cargo_tests --shortlist ${TEST_NAME})
			endif()
		endif()
	endforeach()
endif()

foreach (CARGO_EXE ${CARGO_EXE_LIST})
	if (CARGO_EXAMPLES_LINK_SHARED)
		target_link_libraries(${CARGO_EXE} cargo_shared)
	else()
		target_link_libraries(${CARGO_EXE} cargo)
	endif()
endforeach()

# Warn if stuff is not in the documentation.
if (UNIX AND NOT CARGO_SHUTUP)

	function(search_undocumented regexp name)
		execute_process(COMMAND grep -o -E ${regexp} ${PROJECT_SOURCE_DIR}/cargo.h
						OUTPUT_VARIABLE FUNCTION_NAMES
						OUTPUT_STRIP_TRAILING_WHITESPACE)

		string(REPLACE "(" ";" FUNCTION_NAMES "${FUNCTION_NAMES}")
		string(REPLACE "\n" ";" FUNCTION_NAMES "${FUNCTION_NAMES}")

		set(errmsg "")
		foreach (FUNC ${FUNCTION_NAMES})
			string(REPLACE ")" "" FUNC "${FUNC}")
			string(REGEX REPLACE "(CARGO_[_A-Z0-9]+)[ \t]+[0-9]+[ \t]*" "\\1" FUNC "${FUNC}")
			string(STRIP "${FUNC}" FUNC)

			execute_process(COMMAND grep -o -E "${FUNC}" ${PROJECT_SOURCE_DIR}/docs/api.md
						OUTPUT_VARIABLE API
						RESULT_VARIABLE GREPRES)
			if (GREPRES)
				set(errmsg "${errmsg}${FUNC}\n")
			endif()
		endforeach()
		message(WARNING "Missing ${name}(s) from API documentation:\n ${errmsg}")
	endfunction()

	# Functions.
	search_undocumented("cargo_[_a-zA-Z0-9]*?\\(" "function")

	# Enums and constants.
	# TODO: Fix more for enum values
	search_undocumented("[ \t]*CARGO_[_A-Z0-9]+[ \t]+[a-zA-Z0-9]+" "enums")

	# Types.
	# TODO: Add for structs and other typedefs...
endif()

# For pkg-config file
set(includedir "\${prefix}/include")
set(libdir "\${prefix}/lib")
set(PKG_CONFIG_LIBDIR "\${libdir}")
set(PKG_CONFIG_INCLUDEDIR "\${includedir}")

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/pkg-config.pc.cmake"
	"${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/pkgconfig/cargo.pc"
)

install(FILES cargo.h
		DESTINATION "include")

install(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/pkgconfig/cargo.pc"
		DESTINATION "lib/pkgconfig")
