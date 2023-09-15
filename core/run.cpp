#include "chip_eight.hpp"
#include <iostream>

int main(int argc, char **argv) {
  if (argc == 1) {
    std::cout << "Usage: " << argv[0] << " [rom_file]" << std::endl;
    exit(1);
  }

  Chip8 c8(argv[1]);

  while (true) {
    c8.tick();

    for (auto& r : c8.disp) {
      for (auto c : r) {
        std::cout << (c ? "■" : "□") << " ";
      }
      std::cout << std::endl;
    }

    std::cout << std::endl;
  }
}
