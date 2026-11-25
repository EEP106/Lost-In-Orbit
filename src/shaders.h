#pragma once
#include "SDL3/SDL_render.h"

// If the .frag files are not loaded/working this should do nothing and not crash
namespace SHADERS {

  struct RippleParams {
    float time;         // Based on ticks_total
    float amplitude;    // How much the sprite shifts (0.004 = 0.4%)
    float frequency;    // number of vertical shifts
    float speed;
    float phase;        // offset so different sprites do not wave in sync
  };

  const int MAX_RIPPLE_SLOTS = 4;

  void BeginRipple(SDL_Renderer* renderer, int slot, const RippleParams& params);

  void EndShader(SDL_Renderer* renderer);

  void Shutdown(SDL_Renderer* renderer);
}
