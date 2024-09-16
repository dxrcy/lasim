# **LASIM** : LC-3 Assembler & Simulator

A simple [LC-3](https://en.wikipedia.org/wiki/Little_Computer_3) assembler and
simulator in C++.

# Usage

> Only *nix systems are supported currently.

- `g++` to compile

- Development dependencies (for `make test` or `make watch`):
    - [`lc3as` from `lc3tools`](https://github.com/chiragsakhuja/lc3tools)
    - [`delta`]
    - [`reflex`](https://github.com/cespare/reflex) *(optional)*

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
- `encrypt`: Encrypt/decrypt a message using a Caesar cipher
- `fibonacci`: Caclulates the n-th fibonacci number (no output)
- `hello_world`: Take a wild guess...
- `store_number`: Reads a 5-digit number and stores it in memory (no output)
- `string_array`: Prints an array of static-memory strings

Some of the examples are taken from
[here](https://github.com/Nguyen-Nhat-Tuan-Minh/LC_3-Assembly-Program) and
[here](https://github.com/dideler/LC-3-Programs).

# C++ Subset

This project only uses the following C++ features (to keep it simple):

- References
- Booleans
- `nullptr`
- `static_cast`/`reinterpret_cast`
- `enum class`
- `std::vector`

# Features to Implement

- [ ] Add debugger mode
- [ ] Make error messages nicer

