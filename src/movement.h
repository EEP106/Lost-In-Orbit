#pragma once

#include "entity.h"
#include "levels.h"

bool TryMove(Entity* mover, LevelData* level, CommandBuffer* commandBuffer, int xDir, int yDir, int strength);
