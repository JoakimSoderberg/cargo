cmake_minimum_required (VERSION 2.8)
project(cargo C)

option(CARGO_DEBUG "Set cargo debug level" 0)
option(CARGO_TEST "Build unit tests" ON)

if (NOT CARGO_DEBUG)
	set(CARGO_DEBUG 0)
endif()

source_group("Headers" FILES cargo.h)
source_group("Sources" FILES cargo.c)

add_definitions(-DCARGO_DEBUG=${CARGO_DEBUG})

if (MSVC)
    # Turn off Microsofts "security" warnings.
   add_definitions("/W3 /D_CRT_SECURE_NO_WARNINGS /wd4005 /wd4996 /nologo")
endif()

add_executable(cargo cargo.c cargo.h)
set_target_properties(cargo PROPERTIES COMPILE_DEFINITIONS "CARGO_EXAMPLE=ON")

if (CARGO_TEST)
	ENABLE_TESTING()

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
			message("test: '${TEST_NAME}'")
			add_test("${TEST_NAME}" cargo_tests 1)
		endif()
	endforeach()
endif()
