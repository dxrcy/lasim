# LC-3 Simulator

An attempt at implementing an LC-3 simulator in C++.

Only *nix systems are supported. Use WSL or something if you need.

# Usage

An [LC-3 assembler](https://github.com/chiragsakhuja/lc3tools) must be installed
if you wish to assemble object files.

Dependencies:

- `g++` to compile
- `lc3as` (from `lc3tools`) to assemble programs to object files

```sh
# Download
git clone https://github.com/dxrcy/lc3sim lc3sim
cd lc3sim

# Compile
make

# Assemble and execute examples/hello_world.asm
make example

# Assemble and execute a specific file
lc3as examples/checkerboard.asm
./lc3sim examples/checkerboard.obj
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
- `new` / `delete`
- `static_cast` / `reinterpret_cast`

# TODO

- Create assembler
- Add debugger mode

- Move all error-reporting to a function in `main.cpp`

