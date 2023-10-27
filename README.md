# chipp
toy implementation of [the chip-8 virtual machine](https://en.wikipedia.org/wiki/CHIP-8) in c++

made with the help of [tobias langhoff's excellent system documentation](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/)

## to compile:
```bash
git clone --recurse-submodules https://github.com/ggliv/chipp
cd chipp
mkdir build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
./chipp $ROM_FILE
```

## controls:
| | | | |
|---|---|---|---|
| 1 | 2 | 3 | 4 |
| Q | W | E | R |
| A | S | D | F |
| Z | X | C | V |
