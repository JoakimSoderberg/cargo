cmake_minimum_required (VERSION 2.8)
project(cargo C)

option(CARGO_DEBUG "Set cargo debug level" 0)
option(CARGO_TEST "Build unit tests" ON)
option(CARGO_COVERALLS "Generate coveralls data (CARGO_TEST must be turned on, and CMAKE_BUILD_MODE must be Debug)" OFF)

if (UNIX)
	option(CARGO_WITH_MEMCHECK "Run unit tests in valgrind" ON)
endif()

if (NOT CARGO_DEBUG)
	set(CARGO_DEBUG 0)
endif()

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

message("Setting debug level to: ${CARGO_DEBUG}")
add_definitions(-DCARGO_DEBUG=${CARGO_DEBUG})

if (MSVC)
    # Turn off Microsofts "security" warnings.
   add_definitions("/W3 /D_CRT_SECURE_NO_WARNINGS /wd4005 /wd4996 /nologo")
endif()

add_executable(cargo cargo.c cargo.h)
set_target_properties(cargo PROPERTIES COMPILE_DEFINITIONS "CARGO_EXAMPLE=ON")

if (CARGO_TEST)
	ENABLE_TESTING()

	if (CARGO_WITH_MEMCHECK)
		find_program(VALGRIND_PROGRAM NAMES valgrind)

		if (NOT EXISTS ${VALGRIND_PROGRAM})
			message(WARNING "Valgrind not found! Tests will be run without memory check. Turn this warning off by installing valgrind or settings -DCARGO_WITH_MEMCHECK=OFF")
			set(CARGO_WITH_MEMCHECK OFF)
		else()
			message("Found valgrind: ${VALGRIND_PROGRAM}")
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
				"CARGO_ADD_TEST\\(TEST_[a-z_]+"
				TEST_LINES
				"${CARGO_LINES}")

	foreach (CARGO_LINE ${TEST_LINES})
		string(REGEX MATCH
				"TEST_[a-z_]+"
				TEST_NAME
				"${CARGO_LINE}")

		if (TEST_NAME)
			message("Adding test: '${TEST_NAME}'")

			if (CARGO_WITH_MEMCHECK)
				add_test("${TEST_NAME}_memchk" ${VALGRIND_PROGRAM} --error-exitcode=5 --leak-check=full ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cargo_tests ${TEST_NAME})
			else()
				add_test("${TEST_NAME}" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cargo_tests ${TEST_NAME})
			endif()
		endif()
	endforeach()
endif()
