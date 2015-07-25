[![Travis CI build status (Linux)][travis_img]][travis]
[![Appveyor Build status (Windows)][appveyor_img]][appveyor]
[![drone.io Build Status (verbose)][droneio_img]][droneio]
[![Circle CI][circleci_img]][circleci]
[![Semaphore Build Status][semaphore_img]][semaphore]
[![Shippable Build Status][shippable_img]][shippable]
[![Solano build status][solano_img]][solano]
[![Coveralls Status][coveralls_img]][coveralls]
[![Codecov Status][codecov_img]][codecov]
[![Coverity Statical Analysis Status][coverity_img]][coverity]
[![Codeship Status for JoakimSoderberg/cargo][codeship_img]][codeship]
[![Documentation Status][docs_img]][docs]

cargo
=====

C ARGument Obtainer. Alternative command line parser to getopt.

Designed to be small and easy to drop into any program without much configuration.

cargo is license under the [MIT License](http://opensource.org/licenses/mit-license.php)

Documentation
-------------

Documentation is available at http://cargo.readthedocs.org/en/latest

It was generated using [mkdocs][mkdocs] and can be found under the
[docs/](https://github.com/JoakimSoderberg/cargo/tree/master/docs) directory.

If you'd rather learn by example see
[examples/](https://github.com/JoakimSoderberg/cargo/tree/master/examples)

Compiling
---------

The recommended way to compile and run the tests is to use the CMake project on all
platforms. This will also build libraries and the examples.

Both for Windows and Linux
###########################

```bash
$ mkdir build && cd build
$ cmake ..        # Add -DCARGO_DEBUG=<level> for debugging output.
$ cmake --build . # You can use 'make' also, but not with Visual Studio.
$ ctest           # Run the tests.
```

Compiling manually
------------------

If you'd rather compile manually without using CMake, either simply add it to your
own build system, or follow the steps below.

Simply drop into your project and compile, no configuration needed.

To compile the tests simply define `CARGO_TEST`. For example using `gcc`:

```bash
$ gcc -DCARGO_TEST=1 cargo.c
```

Or on Windows using the visual studio command line:

```bash
> cl.exe /DCARGO_TEST cargo.c
```

[travis]: https://travis-ci.org/JoakimSoderberg/cargo
[travis_img]: https://travis-ci.org/JoakimSoderberg/cargo.svg
[appveyor]: https://ci.appveyor.com/project/JoakimSoderberg/cargo
[appveyor_img]: https://ci.appveyor.com/api/projects/status/hia4q08852puktpf?svg=true
[droneio]: https://drone.io/github.com/JoakimSoderberg/cargo/latest
[droneio_img]: https://drone.io/github.com/JoakimSoderberg/cargo/status.png
[circleci]: https://circleci.com/gh/JoakimSoderberg/cargo
[circleci_img]: https://circleci.com/gh/JoakimSoderberg/cargo.svg?style=svg 
[semaphore]: https://semaphoreapp.com/joakimsoderberg/cargo
[semaphore_img]: https://semaphoreapp.com/api/v1/projects/22d61980-73d0-45bc-ba6c-3ed6c1ebadf5/366031/shields_badge.svg
[shippable]: https://app.shippable.com/projects/54f8944b5ab6cc1352934eed/builds/latest
[shippable_img]: https://api.shippable.com/projects/54f8944b5ab6cc1352934eed/badge?branchName=master
[solano]: https://ci.solanolabs.com:443/JoakimSoderberg/cargo/suites/193238
[solano_img]: https://ci.solanolabs.com:443/JoakimSoderberg/cargo/badges/193238.png
[coveralls]: https://coveralls.io/r/JoakimSoderberg/cargo
[coveralls_img]: https://coveralls.io/repos/JoakimSoderberg/cargo/badge.svg
[codecov]: http://codecov.io/github/JoakimSoderberg/cargo?branch=master
[codecov_img]: http://codecov.io/github/JoakimSoderberg/cargo/coverage.svg?branch=master
[coverity]: https://scan.coverity.com/projects/3566
[coverity_img]: https://scan.coverity.com/projects/3566/badge.svg
[codeship]: https://codeship.com/projects/66740
[codeship_img]: https://codeship.com/projects/a953df40-a586-0132-a1eb-3aaa69fc7edf/status?branch=master
[docs]: http://cargo.readthedocs.org/en/latest/
[docs_img]: https://readthedocs.org/projects/cargo/badge/?version=latest
[mkdocs]: http://www.mkdocs.org/
