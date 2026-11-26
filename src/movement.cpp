#include "command.h"
#include "entity.h"
#include "levels.h"

bool TryMove(Entity* mover, LevelData* level,CommandBuffer* commandBuffer, int xDir, int yDir, int strength){
  if(HasBehaviour(mover, CAN_MOVE) == false){
    return false;
  }

  if(strength < 0){
    return false;
  }

  int test_x = mover->x + xDir;
  int test_y = mover->y + yDir;
  Entity* stepInto_entity = GetEntity(level, test_x, test_y);
  uint16_t stepInto_tile_id = GetCellID(level, test_x, test_y);
  if(stepInto_entity == nullptr){
    if(!IsSolid(test_x, test_y, level)){
      MoveCommand mv(mover, xDir, yDir);
      Queue(commandBuffer, mv);
      return true;
    }
    return false;
  }

  if(HasBehaviour(stepInto_entity, CAN_MOVE)){
    if(TryMove(stepInto_entity, level,commandBuffer, xDir, yDir, --strength)){
      MoveCommand mv(mover, xDir, yDir);
      AddBehaviour(mover, Behaviour::IS_PUSHING);
      Queue(commandBuffer, mv);
      return true;
    }
  }

  return false;
}
  
