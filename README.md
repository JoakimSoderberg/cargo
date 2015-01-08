[![Travis CI build status (Linux)](https://travis-ci.org/JoakimSoderberg/cargo.svg)](https://travis-ci.org/JoakimSoderberg/cargo)
[![Appveyor Build status (Windows)](https://ci.appveyor.com/api/projects/status/hia4q08852puktpf?svg=true)](https://ci.appveyor.com/project/JoakimSoderberg/cargo)


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
