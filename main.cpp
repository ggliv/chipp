#include "chip_eight.hpp"
#include "sdl/include/SDL.h"
#include "sdl/include/SDL_render.h"
#include "sdl/include/SDL_video.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif
#include <cstdint>
#include <iostream>

// R, G, B, A
#define DISP_ON_COLOR 0xFF, 0xCC, 0x00, 0xFF
#define DISP_OFF_COLOR 0x99, 0x66, 0x00, 255

#define DISP_WINDOW_SCALE 8

SDL_Window *window;
SDL_Renderer *renderer;
Chip8 *c8;

uint8_t getKey(SDL_Scancode sym) {
  switch (sym) {
    case SDL_SCANCODE_1: { return 0x1;  }
    case SDL_SCANCODE_2: { return 0x2;  }
    case SDL_SCANCODE_3: { return 0x3;  }
    case SDL_SCANCODE_4: { return 0xC;  }
    case SDL_SCANCODE_Q: { return 0x4;  }
    case SDL_SCANCODE_W: { return 0x5;  }
    case SDL_SCANCODE_E: { return 0x6;  }
    case SDL_SCANCODE_R: { return 0xD;  }
    case SDL_SCANCODE_A: { return 0x7;  }
    case SDL_SCANCODE_S: { return 0x8;  }
    case SDL_SCANCODE_D: { return 0x9;  }
    case SDL_SCANCODE_F: { return 0xE;  }
    case SDL_SCANCODE_Z: { return 0xA;  }
    case SDL_SCANCODE_X: { return 0x0;  }
    case SDL_SCANCODE_C: { return 0xB;  }
    case SDL_SCANCODE_V: { return 0xF;  }
    default:             { return 0x10; }
  }
}

#ifdef __EMSCRIPTEN__
extern "C" {
#endif
static void EMSCRIPTEN_KEEPALIVE mainloop(void) {
  // Handle events
  {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT: {
          exit(0);
          break;
        }

        case SDL_KEYDOWN: {
          auto key = getKey(e.key.keysym.scancode);
          if (key > 0xF) break;
          c8->keypad[key] = true;
          break;
        }

        case SDL_KEYUP: {
          auto key = getKey(e.key.keysym.scancode);
          if (key > 0xF) break;
          c8->keypad[key] = false;
          break;
        }
      }
    }
  }

  c8->tick();

  // Render any new updates
  if (c8->justDrew) {
    for (auto i = 0; i < CH8_DISP_ROWS; i++) {
      for (auto j = 0; j < CH8_DISP_COLS; j++) {
        if (c8->disp[i][j]) {
          SDL_SetRenderDrawColor(renderer, DISP_ON_COLOR);
        } else {
          SDL_SetRenderDrawColor(renderer, DISP_OFF_COLOR);
        }

        SDL_RenderDrawPoint(renderer, j, i);
      }
    }

    SDL_RenderPresent(renderer);
  }
}
#ifdef __EMSCRIPTEN__
}
#endif

#ifdef __EMSCRIPTEN__
extern "C" {
#endif
int EMSCRIPTEN_KEEPALIVE main(int argc, char **argv) {
  // Set up SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "Could not initialize SDL: " << SDL_GetError() << std::endl;
    exit(1);
  }

  SDL_CreateWindowAndRenderer(CH8_DISP_COLS * DISP_WINDOW_SCALE, CH8_DISP_ROWS * DISP_WINDOW_SCALE, 0, &window, &renderer);

  SDL_SetWindowTitle(window, argv[0]);

  SDL_RenderSetScale(renderer, DISP_WINDOW_SCALE, DISP_WINDOW_SCALE);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);

  // Set up Chip-8

  #ifdef __EMSCRIPTEN__

  Chip8 realC8("/rom.ch8");
  c8 = &realC8;

  emscripten_set_main_loop(mainloop, -1, 1);

  #else

  if (argc == 1) {
    std::cout << "Usage: " << argv[0] << " [rom_file]" << std::endl;
    exit(1);
  }

  Chip8 realC8(argv[1]);
  c8 = &realC8;

  while (true) { mainloop(); }

  #endif
}
#ifdef __EMSCRIPTEN__
}
#endif

