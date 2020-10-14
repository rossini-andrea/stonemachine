# The Stone Machine

## Preamble

In the late evening of 04/10/2020, I was surfing around the Internet when
suddenly a temporary perturbation in the connection allowed me to download a
costruction order coming directly from the Cult of the Bound Variable!

I could not believe my eyes, but I could not afford to lose such a rarity, and
decided to save the original document in `um-spec.txt`.

The document describes the so called "Universal Machine", a complex contraption
which can read and execute operational instructions directly from carved
sandstone platters and carve data itself.

I read the document, a mistery unfolding in front of me, and accepted the
challenge; can a 21st century man recreate a glimpse of pre industrial
revolution technology?

## Implementation

Since everyone knows that only ancient Egyptians and Mayans could carve stone
because they could get aid from aliens, I must solely rely on electronics and
software to create an emulator for the "Universal Machine": I called this
project "The Stone Machine".

The project comprehends:

* **stonemachine**: The implementation of the actual emulator.
* **carver**: The software simulacrum of a machinery that can read assembly code
    from paper scrolls and carve binary code on stone platters.

## Building

To build the software, copy or clone this repository on your PC. Check if you
have access to:

* cmake >= 3.16.
* boost - any modern version should work, please provide feedback if you find
    yours to be too old.
* A compiler supporting at least C++17.

To build navigate to the local repo folder, open the command line and issue the
commands:

```sh
mkdir build
cd build
cmake ..
make
cd ..
```

Running is simple, first of all, compile one of the example scrolls with carver:

```sh
./build/carver scrolls/hello.scroll scrolls/hello.um
```

And pass the new stone carved program to the machine:

```sh
./build/stonemachine scrolls/hello.um
```

If everything is ok, you should get as output:

```
Hello, World!

```

Press CTRL+C to stop the process.

## Assembly language scrolls quick reference

To code for the stone machine, you don't need to manually carve binary digits on
sandstone platters. You can write text on paper scrolls, and the carver will do
the hard work for you.

The scrolls, saved as `.scroll` files are composed of multiple lines of single
instructions obeying the syntax:

```
Operation Param, Param, ...
```

A Param can be either a register name ranging from A to H, a numerical unsigned
expression, or a single-quoted character.

The available operations are:

```
CondMove    register-name, register-name, register-name
Index       register-name, register-name, register-name
Amend       register-name, register-name, register-name
Add         register-name, register-name, register-name
Mult        register-name, register-name, register-name
Div         register-name, register-name, register-name
Nand        register-name, register-name, register-name
Halt
Alloc       register-name, register-name
Abandon     register-name
Output      register-name
Input       register-name
Load        register-name, register-name
Orthography register-name, expression
Data        expression
```

## Notes

This project has been a warm up for more interesting retro CPU emulation
projects. While I where at it, I decided to learn Boost::Spirit from scratch.

If you wish, you can try the benchmarks available at
[http://boundvariable.org/task.shtml](http://boundvariable.org/task.shtml). They work!

Thanks for reading.
