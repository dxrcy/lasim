# LC-3 Assember and Simulator

An attempt at implementing an LC-3 assembler and simulator in C++.

Only *nix systems are supported. Use WSL or something if you need.

# Usage

An [LC-3 assembler](https://github.com/chiragsakhuja/lc3tools) must be installed
if you wish to assemble object files.

Dependencies:

- `g++` to compile
- `lc3as` (from `lc3tools`) to assemble programs to object files

```sh
# Download
git clone https://github.com/dxrcy/lc3 lc3
cd lc3

# Compile
make

# Assemble and execute examples/hello_world.asm
make example

# Assemble and execute a specific file
./lc3 examples/checkerboard.asm
# Or assemble and execute in separate steps
./lc3 -a examples/checkerboard.asm -o examples/checkerboard.obj
./lc3 -x examples/checkerboard.obj
```

# Examples

- `checkerboard`: Prints an ASCII checkerboard
- `count_bits`: Counts the '1' bits in an inputted number 0-9
- `hello_world`: Take a wild guess
- `store_number`: Reads a 5-digit number and stores it in memory
- `string_array`: Prints an array of static-memory strings

Some of the examples are taken from
[here](https://github.com/Nguyen-Nhat-Tuan-Minh/LC_3-Assembly-Program).

# C++ Subset

This project only uses the following C++ features (to keep it simple):

- References
- `nullptr`
- `static_cast` / `reinterpret_cast`
- `std::vector`
- `enum class`

# TODO

- Add debugger mode

- Move all error-reporting to a function in `main.cpp`

