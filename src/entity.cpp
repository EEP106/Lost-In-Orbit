#include "entity.h"
#include "command.h"
#include "levels.h"
#include "gameState.h"

void SnapEntityToEndOfAction(Entity* entity){
  entity->x_prev = entity->x;
  entity->y_prev = entity->y;
  entity->facing_previous = entity->facing_current;
  entity->progress_01 = 0;
  entity->action = Actions::NONE;}

Entity* GetActiveEntity(Gameplay* gameplay){
    return gameplay->activePlayerBuffer[gameplay->activePlayerIndex];
}

bool IsActing(Entity* e){
    if(e->active == false) return false;
    return e->action != Actions::NONE;
}

bool IsMoving(Entity* e){
    return e->x != e->x_prev || e->y != e->y_prev;
}

bool HasBehaviour(Entity* entity, Behaviour flags){
return (entity->behaviour & flags) == flags;
}

void InitializeBaseBehaviour(Entity* entity){
assert(entity->active);
switch (entity->id) {
  default:
    assert(false);
    break;
  case ENTITY_ID::PLAYER:
    SetBehaviour(entity, (Behaviour)(CAN_ROTATE | CAN_MOVE | IS_PLAYER | RESPOND_TO_INPUT));
    AddBehaviour(entity, Behaviour::JUMPS);
    entity->strength = 1;
    break;  
  case ENTITY_ID::ROCK:
    SetBehaviour(entity, (Behaviour)CAN_MOVE);
    break;
  }
}

void SetBehaviour(Entity* entity, Behaviour flags){
entity->behaviour = flags;
}

void AddBehaviour(Entity* entity, Behaviour flags){
entity->behaviour = (Behaviour)(entity->behaviour | flags);
}

void RemoveBehaviour(Entity* entity,Behaviour flags){
entity->behaviour = (Behaviour)(entity->behaviour & ~flags);
}

// Leftover from base project
void PostMove(Entity *entity, LevelData* level, CommandBuffer* commandBuffer){
}
void PostRotation(Entity* entity, LevelData* level, CommandBuffer* commandBuffer, Direction from, Direction to){
}
void PreRotation(Entity* entity, LevelData* level,CommandBuffer* commandBuffer, Direction from, Direction to){
}

