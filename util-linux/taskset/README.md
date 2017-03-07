# Minimal `taskset` Implementation

This is a minimal implementation of the `taskset` Linux user-space command.  It is intended for use
on systems that do not provide this command as a built-in.

## Building

To build the application for the current system, run

    make

You can also cross-compile it for another system:

    make GCC=aarch64-linux-gnu-gcc

## Installing

To install the application in the current system, run

    make install

You can change the installation target by specifying `PREFIX`:

    make install PREFIX=/usr/local

The default `PREFIX` is `/usr`.

## Testing

    make test

There is currently no automated checking if tests pass.  You have to check manually whether the
output is sane.
