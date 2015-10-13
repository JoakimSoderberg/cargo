Compiling
=========
cargo was designed for **portability** and is meant to be easily added to any project, so it consists of only [cargo.c][cargoc] and [cargo.h][cargoh].

In these files you can find the library itself, as well as unit tests, an example and a small helper program.

There are multiple ways you can decide to compile and use cargo in your project:

Copy the files
--------------
Simply copy [cargo.c][cargoc] and [cargo.h][cargoh] into your project dir and build it as a part of your project. This is as simple as it gets.

CMake project (Recommended)
---------------------------
cargo comes with a [CMake][cmake] project that you can use to build everything.

It is setup to build `cargo_tests`, `cargo_helper` as well as a static and dynamic library on all platforms.

To see all available cmake options you can use `cmake -LH ..` for turning off memory checks and such things.

### Unix

```bash
$ cd <cargo dir>
$ mkdir build
$ cd build
$ cmake ..
$ make
```

### Windows

Using git bash. Assuming you have Visual Studio installed (See [CMake][cmake] documentation for using generators for other IDEs)

```bash
$ cd <cargo dir>
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build . # To build from command line
$ start cargo.sln # Open project in Visual Studio.
```

Build unit test / helper applications manually
----------------------------------------------

### Unix

```bash
$ gcc -DCARGO_TEST=1 -o cargo_tests cargo.c
$ gcc -DCARGO_HELPER=1 -o cargo_helper cargo.c
```

### Windows

Using the Visual Studio commandline:

```bash
> cl.exe /DCARGO_TEST /Fecargo_tests cargo.c
> cl.exe /DCARGO_HELPER /Fecargo_helper cargo.c
```

Unit tests
==========
The benefit of using the [CMake][cmake] project to build everything is that it also sets up the unit tests to automatically run each separate test in its own process, as well as running them through [Valgrind][valgrind] (Linux) or [Dr. Memory][drmemory] (Windows) to check for any memory leaks or corruption.

However if you want you can compile these manually as well (see below).

Running unit tests using CMake
------------------------------
Start by following the build instructions [above](#cmake-project-recommended).

To run the tests simply do this:

```bash
$ cd <cargo dir>
$ cd build
$ ctest --output-on-failure # Only show full output if a test fails.
Test project /Users/js/dev/cargo/build
      Start  1: TEST_no_args_bool_option
 1/72 Test  #1: TEST_no_args_bool_option ..........................   Passed    0.01 sec
      Start  2: TEST_add_integer_option
 2/72 Test  #2: TEST_add_integer_option ...........................   Passed    0.00 sec
      Start  3: TEST_add_uinteger_option
 ...
 100% tests passed, 0 tests failed out of 72
```

The above is when running the tests through [CMakes][cmake] ctest program that takes care of such things as running each of the tests in its own process and redirects the output to only show a summary as above. Only if the test fails will it output what went wrong.

It also takes care of running each test through the memory checkers.

You can also run the test executable manually with full output, which is nice when troubleshooting:

### Unix

```bash
$ bin/cargo_tests    # Show help and list of tests.
$ bin/cargo_tests -1 # Run all tests. 
```

### Windows

On Windows if you use Visual Studio, it supports building both Debug and Release builds in the same build tree. By default it is set to Debug when running `cmake --build .`. You can change this to Release `cmake --build . --config Release`. For more information regarding this see the [CMake][cmake] documentation.

```bash
> bin/Debug/cargo_tests # The path is different on Windows.
```

Running unit tests manually
---------------------------

### Unix

```bash
$ gcc -DCARGO_TEST=1 -o cargo_tests cargo.c
$ ./cargo_tests -1 # Runs all tests
$ ./cargo_tests    # Show help and list of tests available.
```

### Windows

```bash
> cl.exe /DCARGO_TEST /Fecargo_tests cargo.c
> cargo_tests.exe -1 # Run all tests.
> cargo_tests.exe    # Show help and list available tests.
```

Debugging cargo
===============
When using cargo or modifying it, things might not work as expected. For instance if a unit test fails, it might be beneficial to get some more verbose output of what is happening.

To make this easier cargo can be told to output some extra debug information. This is done by setting the `CARGO_DEBUG` macro. `CARGO_DEBUG=1` will print error messages for internal errors occuring. So for instance if you're geting a non-zero return value from `cargo_add_option` or some other API function you can get the related error message by setting `CARGO_DEBUG` to `1`.

The higher you set `CARGO_DEBUG`, the more verbose it will be. Valid values are between `1-6`. To not get totally flooded using `4` in most cases is recommended.

Compile using CMake with debug output
-------------------------------------

```bash
$ cmake -DCARGO_DEBUG=4 ..
$ make
```

Compile manually with debug output
----------------------------------
Example of compiling the unit tests with debug output:

### Unix

```bash
$ gcc -DCARGO_TEST=1 -DCARGO_DEBUG=4 -o cargo_tests cargo.c
```

### Windows

```bash
> cl.exe /DCARGO_TEST /DCARGO_DEBUG=4 /Fecargo_tests cargo.c
```



[cargoc]: https://github.com/JoakimSoderberg/cargo/blob/master/cargo.c
[cargoh]: https://github.com/JoakimSoderberg/cargo/blob/master/cargo.h
[cmake]: http://www.cmake.org/
[valgrind]: http://valgrind.org/
[drmemory]: http://drmemory.org/
