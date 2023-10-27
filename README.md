# chipp
toy implementation of [the chip-8 virtual machine](https://en.wikipedia.org/wiki/CHIP-8) in c++

made with the help of [tobias langhoff's excellent documentation](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/)

## to compile:
```bash
git clone --recurse-submodules https://github.com/ggliv/chipp
cd chipp
mkdir build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

## to run:
```bash
./chipp $PATH_TO_ROM_FILE
```
you can find some cool roms to play around with [here](https://johnearnest.github.io/chip8Archive/)

## controls:
most chip-8 games are programmed to use the cosmac vip's keypad layout, which looks like this:
| | | | |
|---|---|---|---|
| 1 | 2 | 3 | C |
| 4 | 5 | 6 | D |
| 7 | 8 | 9 | E |
| A | 0 | B | F |

on a qwerty layout, those are mapped like this:

| | | | |
|---|---|---|---|
| 1 | 2 | 3 | 4 |
| Q | W | E | R |
| A | S | D | F |
| Z | X | C | V |
