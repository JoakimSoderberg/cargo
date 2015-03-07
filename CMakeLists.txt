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

# Add project cmake modules to path.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

if (CARGO_TEST AND CARGO_COVERALLS)
    include(Coveralls)
    coveralls_turn_on_coverage()
endif()

# Set some nicer output dirs.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

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

# The internal example in cargo.c
if (CARGO_EXAMPLE)
	add_executable(cargo cargo.c cargo.h)
	set_target_properties(cargo PROPERTIES COMPILE_DEFINITIONS "CARGO_EXAMPLE=1")
endif()

if (CARGO_EXTRA_EXAMPLES)
	file(GLOB EXTRA_EXAMPLE_FILES "${PROJECT_SOURCE_DIR}/examples/*.c")

	foreach(EXAMPLE_FILE ${EXTRA_EXAMPLE_FILES})
		get_filename_component(EXAMPLE_NAME "${EXAMPLE_FILE}" NAME_WE)
		add_executable(${EXAMPLE_NAME} "${EXAMPLE_FILE}" cargo.c)
	endforeach()
endif()

if (CARGO_HELPER)
	add_executable(cargo_helper cargo.c cargo.h)
	set_target_properties(cargo_helper PROPERTIES COMPILE_DEFINITIONS "CARGO_HELPER=1")
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
	set_target_properties(cargo_tests PROPERTIES COMPILE_DEFINITIONS "CARGO_TEST=ON")

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
			message("Adding test: '${TEST_NAME}'")

			if (CARGO_WITH_MEMCHECK)
				if (WIN32)
					# TODO: This is hard coded to bin/Debug for now... Possible fix:
					# http://stackoverflow.com/questions/9467510/how-to-pass-ctest-configuration-type-using-add-test-in-cmake
					add_test("${TEST_NAME}_memchk" ${DRMEMORY_PROGRAM} -batch -show_reachable -exit_code_if_errors 5 -- ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/cargo_tests.exe --shortlist ${TEST_NAME})
				else()
					add_test("${TEST_NAME}_memchk" ${VALGRIND_PROGRAM} --error-exitcode=5 --leak-check=full ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cargo_tests --shortlist ${TEST_NAME})
				endif()
			else()
				add_test("${TEST_NAME}" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cargo_tests --shortlist ${TEST_NAME})
			endif()
		endif()
	endforeach()
endif()
