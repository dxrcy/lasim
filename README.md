# **LASIM** : LC-3 Assembler & Simulator

A simple [LC-3](https://en.wikipedia.org/wiki/Little_Computer_3) assembler and
simulator in C++.

# Usage

> Only *nix systems are supported currently.

- `g++` to compile
- [`lc3as` from `lc3tools`](https://github.com/chiragsakhuja/lc3tools) only for
    running `make test`

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

- `char_count`: Counts the amount of times each letter was inputted
- `checkerboard`: Prints an ASCII checkerboard
- `count_bits`: Counts the '1' bits of an inputted number from 0-9
- `hello_world`: Take a wild guess...
- `store_number`: Reads a 5-digit number and stores it in memory (no output)
- `string_array`: Prints an array of static-memory strings

Some of the examples are taken from
[here](https://github.com/Nguyen-Nhat-Tuan-Minh/LC_3-Assembly-Program) and
[here](https://github.com/dideler/LC-3-Programs).

# C++ Subset

This project only uses the following C++ features (to keep it simple):

- References
- `nullptr`
- `static_cast`/`reinterpret_cast`
- `std::vector`
- `enum class`

# Features to Implement

- [ ] Add debugger mode


