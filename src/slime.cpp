#include "arena.h"
#include "entity.h"
#include "levels.h"
#include "movement.h"
#include "command.h"

void MoveSlime(Memory::Arena* arena_scratch, LevelData* level, CommandBuffer* command, int xDir, int yDir){
  int slimeCount = 0;
  for (int i = 0; i < level->entityCount; i++){
    Entity* entity = &level->entityBuffer[i];
    if (entity->active && entity->id == ENTITY_ID::SLIME){
      slimeCount++;
    }
  }
  if (slimeCount == 0) return;
  
  Entity** slimeBuffer = ALLOC_ARRAY(arena_scratch, Entity*, slimeCount);
  int index = 0;
  for (int i = 0; i < level->entityCount; i++){
    Entity* entity = &level->entityBuffer[i];
    if (entity->active && entity->id == ENTITY_ID::SLIME){
      slimeBuffer[index] = entity;
      index++;
    }
  }

  enum class SlimeMovable {Undetermined, Movable, Blocked, Pushed};
  SlimeMovable* slimeMovableStates = ALLOC_ARRAY(arena_scratch, SlimeMovable, slimeCount);
  for (int i = 0; i < slimeCount; i++) slimeMovableStates[i] = SlimeMovable::Undetermined;

  // Mark already moving slimes as moving, happens when player pushes slimes.
  for (int i = command->index; i < command->queueHead; i++){
    AnyCommand cmd = command->allCommands[i];

    if (cmd.command.type != CMD_TYPE::MOVE) continue;
    Entity* movedEntity = cmd.move.entity;

    for (int j = 0; j < slimeCount; j++){
      Entity* slime = slimeBuffer[j];
      if (slime == movedEntity) slimeMovableStates[j] = SlimeMovable::Pushed;
    }
  }

  // Conditionally mark slimes as blocked iterativley.
  for (int i = 0; i < slimeCount; i++){
    if (slimeMovableStates[i] == SlimeMovable::Pushed) continue;
    Entity* slime = slimeBuffer[i];
    if (IsWalkable(slime->x + xDir, slime->y + yDir, level)) continue; 
    slimeMovableStates[i] = SlimeMovable::Blocked;
  }

  bool propagating = true;
  while (propagating){
    propagating = false;
    for (int i = 0; i < slimeCount; i++){
      if (slimeMovableStates[i] == SlimeMovable::Pushed || slimeMovableStates[i] == SlimeMovable::Blocked) continue;
      
      Entity* slime = slimeBuffer[i];
      int moveIntoEntityIndex = 0;
      Entity* moveIntoEntity = GetEntity(*slimeBuffer, slimeCount, slime->x + xDir, slime->y + yDir, &moveIntoEntityIndex);

      if (moveIntoEntity == nullptr) continue;
      if (slimeMovableStates[moveIntoEntityIndex] == SlimeMovable::Blocked){
        slimeMovableStates[i] = SlimeMovable::Blocked;
        propagating = true;
      }
    }
  }

  // Seed movable from player
  for (int i = 0; i < level->entityCount; i++){
    Entity* player = &level->entityBuffer[i];
    if (!player->active) continue;
    if (player->id != ENTITY_ID::PLAYER) continue;

    for (int j = 0; j < 4; j++){
      Direction dir = (Direction)j;
      int xDir = 0, yDir = 0;
      XYFromDirection(dir, xDir, yDir);

      int intoEntityIndex;
      Entity* intoEntity = GetEntity(*slimeBuffer, slimeCount, player->x + xDir, player->y + yDir, &intoEntityIndex);
      if (intoEntity == nullptr) continue;
      if (!intoEntity->active) continue;
      if (slimeMovableStates[intoEntityIndex] != SlimeMovable::Undetermined) continue;

      slimeMovableStates[intoEntityIndex] = SlimeMovable::Movable;
    }
  }

  // Propagate movable to adjacent and not blocked.
  propagating = true;
  while (propagating){
    propagating = false;
    for (int i = 0; i < slimeCount; i++){
      if (slimeMovableStates[i] != SlimeMovable::Undetermined) continue;
      Entity* slime = slimeBuffer[i];

      for (int j = 0; j < 4; j++){
        Direction dir = (Direction)j;
        int xDir, yDir = 0;
        XYFromDirection(dir, xDir, yDir);

        int intoEntityIndex;
        Entity* intoEntity = GetEntity(*slimeBuffer, slimeCount, slime->x + xDir, slime->y + yDir, &intoEntityIndex);
        if (intoEntity == nullptr) continue;
        if (!intoEntity->active) continue;

        switch (slimeMovableStates[intoEntityIndex]) {
          case SlimeMovable::Undetermined:
          case SlimeMovable::Blocked:
            break;
          case SlimeMovable::Movable:
          case SlimeMovable::Pushed:
            slimeMovableStates[i] = SlimeMovable::Movable;
            propagating = true;
            break;
        }
      }
    }
  }

  for (int i = 0; i < slimeCount; i++){
    if (slimeMovableStates[i] != SlimeMovable::Movable) continue;
    Entity* slime = slimeBuffer[i];

    Entity* entityInFront = GetEntity(level, slime->x + xDir, slime->y + yDir);
    if (entityInFront == nullptr || entityInFront->id == ENTITY_ID::PLAYER || entityInFront->id == ENTITY_ID::SLIME){
      MoveCommand move(slime, xDir, yDir);
      Queue(command, move);
    }
    else{
      TryMove(slime, level, command, xDir, yDir, 1);
    }
  }
}

bool TryShuffle(LevelData* level, CommandBuffer* command, int xDir, int yDir){
  bool hasShuffled = false;
  for (int i = 0; i < level->entityCount; i++){
    Entity* player = &level->entityBuffer[i];
    if (!player->active) continue;
    if (player->id != ENTITY_ID::PLAYER) continue;

    Entity* inFront = GetEntity(level, player->x + xDir, player->y + yDir);
    if (inFront == nullptr) continue;
    if (!inFront->active) continue;
    if (inFront->id != ENTITY_ID::SLIME) continue;

    MoveCommand movePlayer(player, xDir, yDir);
    MoveCommand moveSlime(inFront, -xDir, -yDir);
    Queue(command, movePlayer);
    Queue(command, moveSlime);
    hasShuffled = true;
  }

  return hasShuffled;
}
