# scheme

A small interpreter for the Scheme language written in C++. The main objectives are:

1. Implement enough of the Scheme standard to be useful
2. Optimize for a small source code base
3. Only at the last point comes performance

The interpreter can read the source from standard input.
Alternatively all the files to read can be passed as command line arguments.
If `-` is passed as a command line  argument, standard input will we parsed again.

For better Unix-Support, a source file can start with a single `#` comment line.
This can be used for a `#!` line, so the Scheme script can be made executable.
