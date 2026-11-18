#include "camera.h"
#include "common.h"
#include <cmath>

static float g_screen_w = SCREEN_WIDTH;
static float g_screen_h = SCREEN_HEIGHT;

void camera::SetScreenSize(float w, float h){
  g_screen_w = w;
  g_screen_h = h;
}

static float OriginX(const LevelData* lvl){
  return floorf(g_screen_w / 2.0f - lvl->w * TILE_SIZE_PX_SCALED / 2.0f);
}

static float OriginY(const LevelData* lvl){
  return floorf(g_screen_h / 2.0f - lvl->h * TILE_SIZE_PX_SCALED / 2.0f);
}

bool camera::GetIsPointInsideGrid(float x, float y, const LevelData* lvl){
  int x_grid;
  int y_grid;
  WorldToGrid(x, y, &x_grid, &y_grid, lvl);
  return x_grid >= 0 && y_grid >= 0 && x_grid < lvl->w && y_grid < lvl->h; 
}

void camera::GridToWorld(float* x, float* y, const LevelData* lvl){
  *x = *x * TILE_SIZE_PX_SCALED + TILE_SIZE_PX_SCALED + OriginX(lvl);
  *y = *y * TILE_SIZE_PX_SCALED + TILE_SIZE_PX_SCALED + OriginY(lvl);
}

void camera::WorldToGrid(float x_world, float y_world, int* x, int* y, const LevelData* lvl){
  *x = (int)floorf((x_world - OriginX(lvl)) / TILE_SIZE_PX_SCALED);
  *y = (int)floorf((y_world - OriginY(lvl)) / TILE_SIZE_PX_SCALED);
}
