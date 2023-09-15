#include "chip_eight.hpp"
#include <fstream>
#include <sstream>
#include <vector>

Chip8::Chip8(const char *rom_file_name) {
  // Initialize state variables
  for (auto& b : this->mem)
    b = 0;

  for (auto& w : this->stack)
    w = 0;

  for (auto& b : this->reg)
    b = 0;

  for (auto& row : this->disp)
    for (auto& item : row)
      item = false;

  for (auto& b : this->keypad)
      i = false;

  this->pc = CH8_ENTRY_OFFSET;

  this->i          =
  this->delayTimer =
  this->soundTimer = 0;

  // Load fontset into memory
  {
    auto offset = CH8_FONT_OFFSET;
    for (auto c : CH8_FONT)
      this->mem[offset++] = c;
  }

  // Load ROM into memory
  {
    std::ifstream rom(rom_file_name, std::ios::binary);

    if (!rom) {
      std::string message = "Error reading ROM file: ";
      throw std::runtime_error(message.append(rom_file_name));
    }

    std::vector<char> bytes(
         (std::istreambuf_iterator<char>(rom)),
         (std::istreambuf_iterator<char>()));

    rom.close();

    if (bytes.size() > (CH8_MEM_SIZE - CH8_ENTRY_OFFSET)) {
      throw std::runtime_error("ROM file is too large!");
    }

    auto offset = CH8_ENTRY_OFFSET;
    for (auto& b : bytes)
      // note that this is technically UB, as char
      // is not guaranteed to be either unsigned or
      // eight bits wide. we're using it anyways on
      // the assumption that it is safe.
      this->mem[offset++] = static_cast<uint8_t>(b);
  }
}

inline void throw_unimpl_op(uint16_t op) {
  std::stringstream message;
  message << "Unimplemented opcode: " << std::hex << op;
  throw std::runtime_error(message.str());
}

void Chip8::tick() {
  // Fetch
  uint16_t op = (this->mem[this->pc] << 8) | this->mem[this->pc + 1];
  this->pc += 2;

  // Decode+execute
  switch (op & 0xF000) {
    case 0x0000: {
      switch (op & 0x0FFF) {
        // Clear screen
        case 0x00E0: {
          for (auto& row : this->disp)
            for (auto& item : row)
              item = false;
          break;
        }

        default: throw_unimpl_op(op);
      }
      break;
    }

    // Jump
    case 0x1000: {
      this->pc = CH8_NNN(op);
      break;
    }

    // Set register Vx
    case 0x6000: {
      this->reg[CH8_X(op)] = CH8_NN(op);
      break;
    }

    // Add value to register Vx
    case 0x7000: {
      this->reg[CH8_X(op)] += CH8_NN(op);
      break;
    }

    // Set index register
    case 0xA000: {
      this->i = CH8_NNN(op);
      break;
    }

    // Draw
    case 0xD000: {
      this->reg[0xF] = 0;

      auto y = this->reg[CH8_Y(op)] % CH8_DISP_ROWS;
      for (auto n = 0; n < CH8_N(op); n++) {
        auto x = this->reg[CH8_X(op)] % CH8_DISP_COLS;
        auto byte = this->mem[this->i + n];

        for (auto b = 7; b >= 0; b--) {
          auto bit = (byte >> b) & 1;

          if (bit && this->disp[y][x]) {
            this->disp[y][x] = false;
            this->reg[0xF] = 1;
          } else if (bit && !this->disp[y][x]) {
            this->disp[y][x] = true;
          }

          if (++x >= CH8_DISP_COLS) {
            break;
          }
        }

        if (++y >= CH8_DISP_ROWS) {
          break;
        }
      }

      break;
    }

    default: throw_unimpl_op(op);
  }
}

