[![Travis CI build status (Linux)](https://travis-ci.org/JoakimSoderberg/cargo.svg)](https://travis-ci.org/JoakimSoderberg/cargo)
[![Appveyor Build status (Windows)](https://ci.appveyor.com/api/projects/status/hia4q08852puktpf?svg=true)](https://ci.appveyor.com/project/JoakimSoderberg/cargo)
[![drone.io Build Status (verbose)](https://drone.io/github.com/JoakimSoderberg/cargo/status.png)](https://drone.io/github.com/JoakimSoderberg/cargo/latest)
[![Circle CI](https://circleci.com/gh/JoakimSoderberg/cargo.svg?style=svg)](https://circleci.com/gh/JoakimSoderberg/cargo)
[![Semaphore Build Status](https://semaphoreapp.com/api/v1/projects/22d61980-73d0-45bc-ba6c-3ed6c1ebadf5/366031/shields_badge.svg)](https://semaphoreapp.com/joakimsoderberg/cargo)
[![Codeship Status for JoakimSoderberg/cargo](https://codeship.com/projects/a953df40-a586-0132-a1eb-3aaa69fc7edf/status?branch=master)](https://codeship.com/projects/66740)
[![Shippable Build Status](https://api.shippable.com/projects/54f8944b5ab6cc1352934eed/badge?branchName=master)](https://app.shippable.com/projects/54f8944b5ab6cc1352934eed/builds/latest)
[![Coverage Status](https://coveralls.io/repos/JoakimSoderberg/cargo/badge.svg)](https://coveralls.io/r/JoakimSoderberg/cargo)
[![Coverity Statical Analysis Status](https://scan.coverity.com/projects/3566/badge.svg)](https://scan.coverity.com/projects/3566)

cargo
=====

C ARGument Obtainer. Alternative command line parser to getopt.

Designed to be small and easy to drop into any program without much configuration.

Compiling
---------

Simply drop into your project and compile, no configuration needed.

To compile the tests simply define `CARGO_TEST`. For example using `gcc`:

```bash
$ gcc -DCARGO_TEST=1 cargo.c
```

Or on Windows using the visual studio command line:

```bash
> cl.exe /DCARGO_TEST cargo.c
```

More advanced compiling
-----------------------

To enable running the tests easily on any platform there's also a CMake
project that can be used to generate `Make` files or a `Visual Studio`
project for instance.

Both for Windows and Linux
###########################

```bash
$ mkdir build && cd build
$ cmake ..        # Add -DCARGO_DEBUG=<level> for debugging output.
$ cmake --build . # You can use 'make' also, but not with Visual Studio.
$ ctest           # Run the tests.
```
