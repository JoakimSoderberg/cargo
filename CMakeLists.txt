cmake_minimum_required (VERSION 2.8)
project(cargo C)

option(CARGO_DEBUG "Set cargo debug level" 0)

source_group("Headers" FILES cargo.h)
source_group("Sources" FILES cargo.c)

add_definitions(-DCARGO_TEST -DCARGO_DEBUG=${CARGO_DEBUG})

if (MSVC)
    # Turn off Microsofts "security" warnings.
   add_definitions("/W3 /D_CRT_SECURE_NO_WARNINGS /wd4005 /wd4996 /nologo")
endif()

add_executable(cargo cargo.c cargo.h)
