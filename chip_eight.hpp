#include <cstdint>
#include <string>

#define CH8_MEM_SIZE 0x1000
#define CH8_DISP_ROWS 32
#define CH8_DISP_COLS 64
#define CH8_STACK_SIZE 64
#define CH8_DTIMER_HZ 60
#define CH8_STIMER_HZ 60
#define CH8_REG_COUNT 0x10
#define CH8_ENTRY_OFFSET 0x200
#define CH8_KEY_COUNT 0x10

#define CH8_FONT_OFFSET 0x50
#define CH8_FONT                                                               \
  {                                                                            \
    0xF0, 0x90, 0x90, 0x90, 0xF0,     /* 0 */                                  \
        0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */                                  \
        0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */                                  \
        0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */                                  \
        0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */                                  \
        0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */                                  \
        0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */                                  \
        0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */                                  \
        0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */                                  \
        0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */                                  \
        0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */                                  \
        0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */                                  \
        0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */                                  \
        0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */                                  \
        0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */                                  \
        0xF0, 0x80, 0xF0, 0x80, 0x80  /* F */                                  \
  }

#define CH8_X(n) ((n >> 8) & 0xF)
#define CH8_Y(n) ((n >> 4) & 0xF)
#define CH8_N(n) (n & 0xF)
#define CH8_NN(n) (n & 0xFF)
#define CH8_NNN(n) (n & 0xFFF)

struct Chip8Quirks {
  // AND/OR/XOR ops may or may not
  // reset the flags register to zero
  bool _8xy1_8xy2_8xy3_reset_vf;
  // FX55 and FX65 may or may
  // not modify the I register
  bool _fx55_fx65_changes_i;
  // FX1E may or may not set
  // Vf if the operation overflows
  bool _fx1e_set_vf;
  // Shift operations may or
  // may set Vx to Vy before
  // shifting
  bool _8xy6_8xye_use_vy;
  // BNNN is actually BXNN for
  // CHIP-48 and SUPER-CHIP
  bool _bnnn_is_bxnn;
};

class Chip8 {
  // Quirk toggles
  struct Chip8Quirks quirks;
  // Memory: 4 KiB of RAM
  std::uint8_t mem[CH8_MEM_SIZE];
  // Program counter
  std::uint16_t pc;
  // Index register
  std::uint16_t i;
  // Delay timer
  std::uint8_t delayTimer;
  // Sound timer
  std::uint8_t soundTimer;
  // Registers 0-F
  std::uint8_t reg[CH8_REG_COUNT];
  // Stack
  std::uint16_t stack[CH8_STACK_SIZE];
  // Stack pointer
  std::uint16_t sp;

  // Push/pop from stack
  void push(std::uint16_t word);
  std::uint16_t pop();
  // Check if key is pressed
  bool keyPressed(uint8_t key) const;

  // Throw runtime error with system
  // state and cause as message.
  void dumpAndAbort(std::string message) const;

public:
  // Display: 64x32 monochrome
  // T->white, F->black
  bool disp[CH8_DISP_ROWS][CH8_DISP_COLS];
  // Keypad 0-F
  // T->pressed, F->unpressed
  bool keypad[CH8_KEY_COUNT];
  // Did the last instruction draw
  // to the screen buffer?
  bool justDrew;
  // Decrement the timers
  void decTimers();

  Chip8(Chip8Quirks, const char *);
  Chip8(const char *);

  void tick();
};
