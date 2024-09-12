# LASIM : LC-3 Assembler & Simulator

An attempt at implementing an LC-3 assembler and simulator in C++.

Only *nix systems are supported. Use WSL or something if you need.

# Usage

Dependencies:

- `g++` to compile
- [`laser`](https://github.com/PaperFanz/laser) for test script

```sh
# Download
git clone https://github.com/dxrcy/lasim lasim
cd lasim

# Compile and install
make
sudo make install

# Assemble and execute a specific file
lasim examples/checkerboard.asm
# Or assemble and execute in separate steps
lasim -a examples/checkerboard.asm -o examples/checkerboard.obj
lasim -x examples/checkerboard.obj
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
- Use static error value
- Move all error-reporting to a function in `main.cpp`

