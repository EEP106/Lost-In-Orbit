#pragma once

#include "arena.h"
#include "entity.h"
#include "levels.h"

void MoveSlime(Memory::Arena* arena_scratch, LevelData* level, CommandBuffer* command, int xDir, int yDir);
bool TryShuffle(LevelData* level, CommandBuffer* command, int xDir, int yDir);
bool CheckAllEntitiesSupported(Memory::Arena* arena_scratch, LevelData* level);
