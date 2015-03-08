Cargo Documentation
===================

**C** **ARG**ument **O**btainer. An easy and powerful command line
parser written in C.

Introduction
------------
cargo is a C library for parsing command line arguments. A superiour alternative
to getopt.

The main design principles:

 - **Portability**:
   Written in C89 for maximum portability to all platforms. Verified to work on
   Linux, OSX and Windows. The goal is to support as many platforms as possible.

 - **No dependencies**:
   cargo requires no external dependencies except the standard C library.

 - **Drop in**: 
   cargo consists of *cargo.c* and *cargo.h* which contains everything needed,
   including an example program, unit tests and the lib itself. This is to make
   it as easy as possible to include in any project.

 - **Simple API**:
   The API has been heavily inspired by how Python 
   [argparse][arparse] and other dynamic languages.

 - **Test suite**:
   The goal is keep a [90% test coverage][coveralls].

[argparse]: https://docs.python.org/3/library/argparse.html
[coveralls]: http://coveralls.io/r/JoakimSoderberg/cargo
