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
#define DISP_ON_COLOR 255, 255, 255, 255
#define DISP_OFF_COLOR 0, 0, 0, 0

#define DISP_WINDOW_SCALE 8

SDL_Window *window;
SDL_Renderer *renderer;
Chip8 *c8;

#ifdef __EMSCRIPTEN__
extern "C" {
#endif
static void EMSCRIPTEN_KEEPALIVE mainloop(void) {
  // Handle events
  {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT:
          exit(0);
          break;
      }
    }
  }

  c8->tick();

  // Render any new updates
  if (c8->justDrew) {
    for (auto i = 0; i < CH8_DISP_ROWS; i++) {
      for (auto j = 0; j < CH8_DISP_COLS; j++) {
        if (c8->disp[i][j]) {
          SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        } else {
          SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
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

