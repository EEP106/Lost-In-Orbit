#pragma once

#include "levels.h"

bool TryMove(Entity* mover, LevelData* level,CommandBuffer* commandBuffer, int xDir, int yDir, int strength);
