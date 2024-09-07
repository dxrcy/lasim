# LC-3 Simulator

An attempt at implementing an LC-3 simulator in C++.

Only *nix systems are supported.

# Usage

An [LC-3 assembler](https://github.com/chiragsakhuja/lc3tools) must be installed
if you wish to assemble object files.

```sh
git clone https://github.com/dxrcy/lc3sim lc3sim
cd lc3sim

make

lc3as example/example.asm
./lc3sim example/example.obj
```

# More Examples

More LC-3 programs can be found
[here](https://github.com/Nguyen-Nhat-Tuan-Minh/LC_3-Assembly-Program).

# C++ Subset

- References
- `nullptr`
- `new` / `delete`
- Cast functions

# TODO

- Check bounds before memory/register access
- Create assembler

