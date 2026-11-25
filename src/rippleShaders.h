#pragma once
#include "SDL3/SDL_render.h"

// If the .frag files are not loaded/working this should do nothing and not crash
namespace SHADERS {

  struct RippleParams {
    float time; // so that the ripples freeze when paused
    float amplitude; // how much the ripple moves the pixels
    float frequency; // how many ripples fit in the screen from top to bottom
    float speed;
    float phase;
  };

  const int MAX_RIPPLE_SLOTS = 10; // allows a max number of uniqe ripple effects to be active at once

  void BeginRipple(SDL_Renderer* renderer, int slot, const RippleParams& params);

  void EndRipple(SDL_Renderer* renderer);

  void KillRipple(SDL_Renderer* renderer);
}
