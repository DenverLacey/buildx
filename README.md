# buildx

A stupid build system for C and C++ projects that specifically sets up projects the way I like them.

(This is not meant to be a complete or standalone build system that can build any type of C/C++ project.)

## Dependencies

* premake5
* libc that conforms to POSIX (linux, macOS)
* C11
* make

## Getting Started

This project (as well as the build system itself) relies on premake5 so to build this project you will need that.

```console
$ premake5 gmake2
$ make config=release
$ ./install
```

These commands will build `buildx` and then install it at `usr/local/bin/bx`. If this is not where you want to install `buildx` simply copy the resulting executable (found at `bin/release/bx`) wherever you want it to be.


