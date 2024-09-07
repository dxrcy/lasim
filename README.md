# LC-3 Simulator

An attempt at implementing an LC-3 simulator in C++.

Only *nix systems are supported.

# Usage

An [LC-3 assembler](https://github.com/chiragsakhuja/lc3tools) must be installed
if you wish to assemble object files.

```sh
# Download
git clone https://github.com/dxrcy/lc3sim lc3sim
cd lc3sim

# Compile
make

# Assemble and execute examples/hello_world.asm
make example

# Assemble and execute a specific file
lc3as examples/count_bits.asm
./lc3sim examples/count_bits.obj
```

# Examples

See `/examples/`

More LC-3 programs can be found
[here](https://github.com/Nguyen-Nhat-Tuan-Minh/LC_3-Assembly-Program).

# C++ Subset

- References
- `nullptr`
- `new` / `delete`
- `static_cast` / `reinterpret_cast`

# TODO

- Create assembler

