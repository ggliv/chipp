#include "chip_eight.hpp"
#include <chrono>
#include <thread>
#include <fstream>
#include <optional>
#include <sstream>
#include <vector>

Chip8::Chip8(Chip8Quirks quirks, const char *romPath, uintmax_t instrHz) {
  // initialize state variables
  this->instrHz = instrHz;
  this->lastTimerDec = std::chrono::steady_clock::now();

  for (auto &b : this->mem)
    b = 0;

  for (auto &w : this->stack)
    w = 0;

  for (auto &b : this->reg)
    b = 0;

  for (auto &row : this->disp)
    for (auto &item : row)
      item = false;

  for (auto &k : this->keypad)
    k = false;

  this->pc = CH8_ENTRY_OFFSET;

  this->i = this->sp = this->delayTimer = this->soundTimer = 0;

  this->quirks = quirks;

  // load fontset into memory
  {
    auto offset = CH8_FONT_OFFSET;
    for (auto c : CH8_FONT)
      this->mem[offset++] = c;
  }

  // load rom into memory
  // TODO emscripten file access broken
  {
    std::ifstream rom(romPath, std::ios::binary);

    if (!rom) {
      std::string message = "Error reading ROM file: ";
      throw std::runtime_error(message.append(romPath));
    }

    std::vector<char> bytes((std::istreambuf_iterator<char>(rom)),
                            (std::istreambuf_iterator<char>()));

    rom.close();

    if (bytes.size() > (CH8_MEM_SIZE - CH8_ENTRY_OFFSET)) {
      throw std::runtime_error("ROM file is too large!");
    }

    auto offset = CH8_ENTRY_OFFSET;
    for (auto &b : bytes)
      // note that this is technically ub, as char
      // is not guaranteed to be either unsigned or
      // eight bits wide. we're using it anyways on
      // the assumption that it is safe.
      this->mem[offset++] = static_cast<uint8_t>(b);
  }
}

Chip8::Chip8(const char *romPath, uintmax_t instrHz)
    : Chip8::Chip8({}, romPath, instrHz) {
  // set sensible defaults for quirks
  this->quirks._8xy1_8xy2_8xy3_reset_vf = true;
  this->quirks._fx55_fx65_changes_i = true;
  this->quirks._fx1e_set_vf = false;
  this->quirks._8xy6_8xye_use_vy = true;
  this->quirks._bnnn_is_bxnn = true;
}

inline void Chip8::dumpAndAbort(std::string message) const {
  std::stringstream messageStream;
  messageStream << message << std::endl;
  messageStream << "Op: 0x" << std::hex << unsigned(this->mem[this->pc - 2])
                << std::endl;
  messageStream << "PC: 0x" << std::hex << this->pc - 2 << std::endl;

  throw std::runtime_error(messageStream.str());
}

inline void Chip8::push(std::uint16_t w) {
  // don't segfault if stack pointer
  // is too large
  if (this->sp >= CH8_STACK_SIZE) {
    this->dumpAndAbort("Stack overflow.");
  }

  this->stack[this->sp++] = w;
}

inline std::uint16_t Chip8::pop() {
  // don't segfault if stack pointer
  // is too small
  if (this->sp == 0) {
    this->dumpAndAbort("Stack underflow.");
  }

  return this->stack[--this->sp];
}

inline bool Chip8::keyPressed(uint8_t key) const {
  return key < sizeof(this->keypad) && this->keypad[key];
}

void Chip8::tick() {
  // Timers
  // Run the number of cycles in one frame
  for (uintmax_t i = 0; i < (this->instrHz / 60); i++) {
    this->doCycle();
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(16));
  auto timeDiff = std::chrono::steady_clock::now() - this->lastTimerDec;
  if (std::chrono::duration_cast<std::chrono::microseconds>(timeDiff).count() >=
      16667) {
    if (this->delayTimer > 0) {
      this->delayTimer--;
    }
    if (this->soundTimer > 0) {
      this->soundTimer--;
    }
    this->lastTimerDec += timeDiff;
  }}

void Chip8::doCycle() {
  // Fetch
  uint16_t op = (this->mem[this->pc] << 8) | this->mem[this->pc + 1];
  this->pc += 2;

  // Decode+execute
  switch (op & 0xF000) {
  case 0x0000: {
    switch (op & 0x0FFF) {
    // 00E0: Clear screen
    case 0x00E0: {
      for (auto &row : this->disp)
        for (auto &item : row)
          item = false;

      break;
    }

    // 00EE: Return from subroutine
    case 0x00EE: {
      this->pc = this->pop();
      break;
    }

    default:
      this->dumpAndAbort("Unimplemented opcode.");
    }
    break;
  }

  // 1NNN: Jump
  case 0x1000: {
    this->pc = CH8_NNN(op);
    break;
  }

  // 2NNN: Call into subroutine
  case 0x2000: {
    this->push(this->pc);
    this->pc = CH8_NNN(op);
    break;
  }

  // 3XNN: Skip if Vx == NN
  case 0x3000: {
    if (this->reg[CH8_X(op)] == CH8_NN(op)) {
      this->pc += 2;
    }
    break;
  }

  // 4XNN: Skip if Vx != NN
  case 0x4000: {
    if (this->reg[CH8_X(op)] != CH8_NN(op)) {
      this->pc += 2;
    }
    break;
  }

  case 0x5000: {
    switch (op & 0x000F) {
    // 5XY0: Skip if Vx == Vy
    case 0x0000: {
      if (this->reg[CH8_X(op)] == this->reg[CH8_Y(op)]) {
        this->pc += 2;
      }
      break;
    }

    default:
      this->dumpAndAbort("Unimplemented opcode.");
    }

    break;
  }

  // 6XNN: Set
  case 0x6000: {
    this->reg[CH8_X(op)] = CH8_NN(op);
    break;
  }

  // 7XNN: Add
  case 0x7000: {
    this->reg[CH8_X(op)] += CH8_NN(op);
    break;
  }

  case 0x8000: {
    switch (op & 0x000F) {
    // 8XY0: Set
    case 0x0000: {
      this->reg[CH8_X(op)] = this->reg[CH8_Y(op)];
      break;
    }

    // 8XY1: Binary OR
    case 0x0001: {
      this->reg[CH8_X(op)] |= this->reg[CH8_Y(op)];
      if (this->quirks._8xy1_8xy2_8xy3_reset_vf) {
        this->reg[0xF] = 0;
      }
      break;
    }

    // 8XY2: Binary AND
    case 0x0002: {
      this->reg[CH8_X(op)] &= this->reg[CH8_Y(op)];
      if (this->quirks._8xy1_8xy2_8xy3_reset_vf) {
        this->reg[0xF] = 0;
      }
      break;
    }

    // 8XY3: Binary XOR
    case 0x0003: {
      this->reg[CH8_X(op)] ^= this->reg[CH8_Y(op)];
      if (this->quirks._8xy1_8xy2_8xy3_reset_vf) {
        this->reg[0xF] = 0;
      }
      break;
    }

    // 8XY4: Add
    case 0x0004: {
      uint8_t res = this->reg[CH8_X(op)] + this->reg[CH8_Y(op)];

      auto shouldFlag = res < this->reg[CH8_X(op)];

      this->reg[CH8_X(op)] = res;

      if (shouldFlag) {
        // overflow
        this->reg[0xF] = 1;
      } else {
        // no overflow
        this->reg[0xF] = 0;
      }
      break;
    }

    // 8XY5: Subtract (Vx = Vx - Vy)
    case 0x0005: {
      auto shouldFlag = this->reg[CH8_X(op)] > this->reg[CH8_Y(op)];

      this->reg[CH8_X(op)] -= this->reg[CH8_Y(op)];

      if (shouldFlag) {
        this->reg[0xF] = 1;
      } else {
        this->reg[0xF] = 0;
      }
      break;
    }

    // 8XY6: Shift right
    case 0x0006: {
      if (this->quirks._8xy6_8xye_use_vy) {
        this->reg[CH8_X(op)] = this->reg[CH8_Y(op)];
      }

      auto shiftBit = this->reg[CH8_X(op)] & 1;

      this->reg[CH8_X(op)] >>= 1;

      this->reg[0xF] = shiftBit;
      break;
    }

    // 8XY7: Subtract (Vx = Vy - Vx)
    case 0x0007: {
      auto shouldFlag = this->reg[CH8_Y(op)] > this->reg[CH8_X(op)];

      this->reg[CH8_X(op)] = this->reg[CH8_Y(op)] - this->reg[CH8_X(op)];

      if (shouldFlag) {
        this->reg[0xF] = 1;
      } else {
        this->reg[0xF] = 0;
      }
      break;
    }

    // 8XYE: Shift left
    case 0x000E: {
      if (this->quirks._8xy6_8xye_use_vy) {
        this->reg[CH8_X(op)] = this->reg[CH8_Y(op)];
      }

      auto shiftBit =
          (this->reg[CH8_X(op)] >> (sizeof(this->reg[0]) * 8 - 1)) & 1;

      this->reg[CH8_X(op)] <<= 1;

      this->reg[0xF] = shiftBit;
      break;
    }

    default:
      this->dumpAndAbort("Unimplemented opcode.");
    }

    break;
  }

  case 0x9000: {
    switch (op & 0x000F) {
    // 9XY0: Skip if Vx != Vy
    case 0x0000: {
      if (this->reg[CH8_X(op)] != this->reg[CH8_Y(op)]) {
        this->pc += 2;
      }
      break;
    }

    default:
      this->dumpAndAbort("Unimplemented opcode.");
    }

    break;
  }

  // ANNN: Set index register
  case 0xA000: {
    this->i = CH8_NNN(op);
    break;
  }

  // BNNN: Jump with offset
  case 0xB000: {
    this->pc = CH8_NNN(op);

    if (this->quirks._bnnn_is_bxnn) {
      this->pc += this->reg[0x0];
    } else {
      this->pc += this->reg[CH8_X(op)];
    }
    break;
  }

  // CXNN: Random
  case 0xC000: {
    auto rand = std::rand() % 0xFF;
    this->reg[CH8_X(op)] = rand & CH8_NN(op);
    break;
  }

  // DXYN: Draw
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

  case 0xE000: {
    switch (op & 0x00FF) {
    // EX9E: Skip if key pressed
    case 0x009E: {
      if (this->keyPressed(this->reg[CH8_X(op)])) {
        this->pc += 2;
      }
      break;
    }

    // EXA1: Skip if key not pressed
    case 0x00A1: {
      if (!this->keyPressed(this->reg[CH8_X(op)])) {
        this->pc += 2;
      }
      break;
    }

    default:
      this->dumpAndAbort("Unimplemented opcode.");
    }

    break;
  }

  case 0xF000: {
    switch (op & 0x00FF) {
    // FX07: Vx = delayTimer
    case 0x0007: {
      this->reg[CH8_X(op)] = this->delayTimer;
      break;
    }

    // FX15: delayTimer = Vx
    case 0x0015: {
      this->delayTimer = this->reg[CH8_X(op)];
      break;
    }

    // FX18: soundTimer = Vx
    case 0x0018: {
      this->soundTimer = this->reg[CH8_X(op)];
      break;
    }

    // FX1E: Add to index
    case 0x001E: {
      this->i += this->reg[CH8_X(op)];
      if (this->quirks._fx1e_set_vf && this->i >= 0x1000) {
        this->reg[0xF] = 1;
      }
      break;
    }

    // FX0A: Get key
    case 0x000A: {
      // C++11 doesn't have optional so we're using this instead
      // to wait for press AND release
      static std::pair<uint8_t, bool> waitingForRelease(0, false);

      if (waitingForRelease.second &&
          !this->keyPressed(waitingForRelease.first)) {
        waitingForRelease.second = false;
        break;
      } else {
        for (uint8_t k = 0; k < sizeof(this->keypad); k++) {
          if (this->keyPressed(k)) {
            waitingForRelease.first = k;
            waitingForRelease.second = true;
            break;
          }
        }
      }

      this->pc -= 2;
      break;
    }

    // FX29: Font character
    case 0x0029: {
      this->i = 5 * CH8_N(this->reg[CH8_X(op)]) + CH8_FONT_OFFSET;
      break;
    }

    // FX33: BCD convert
    case 0x0033: {
      auto n = this->reg[CH8_X(op)];
      // Hundreds
      this->mem[this->i] = n / 100;
      // Tens
      this->mem[this->i + 1] = (n % 100) / 10;
      // Ones
      this->mem[this->i + 2] = n % 10;
      break;
    }

    // FX55: Store memory
    case 0x0055: {
      auto r = 0;
      for (; r <= CH8_X(op); r++) {
        this->mem[this->i + r] = this->reg[r];
      }

      if (this->quirks._fx55_fx65_changes_i) {
        this->i += r;
      }
      break;
    }

    // FX65: Load memory
    case 0x0065: {
      auto r = 0;
      for (; r <= CH8_X(op); r++) {
        this->reg[r] = this->mem[this->i + r];
      }

      if (this->quirks._fx55_fx65_changes_i) {
        this->i += r;
      }
      break;
    }

    default:
      this->dumpAndAbort("Unimplemented opcode.");
    }

    break;
  }

  default:
    this->dumpAndAbort("Unimplemented opcode.");
  }
}
