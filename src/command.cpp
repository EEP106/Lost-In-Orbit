#include "command.h"
#include "entity.h"
#include "levels.h"
#include <cassert>

enum class FromRedo {No, Yes};

void Execute(AnyCommand cmd, LevelData* level, CommandBuffer* commandBuffer, FromRedo fromRedo = FromRedo::No){
  switch(cmd.command.type){
  case CMD_TYPE::NONE:
    break;
  case CMD_TYPE::MOVE: {
      MoveCommand mv = cmd.move;
      mv.entity->x_prev = mv.entity->x;
      mv.entity->y_prev = mv.entity->y;
      mv.entity->x += mv.xDir;
      mv.entity->y += mv.yDir;
      if(fromRedo == FromRedo::Yes){
        mv.entity->progress_01 = 1;
      }
      mv.entity->action = Actions::MOVING;
      if(fromRedo == FromRedo::No){
        PostMove(mv.entity, level,commandBuffer);
      }
      break;
    }
  case CMD_TYPE::ROTATE:{
      RotateCommand* rotate = &cmd.rotate;
      if(!HasBehaviour(rotate->entity, CAN_ROTATE)){
        break;
      }
      if(fromRedo == FromRedo::Yes){
        rotate->entity->progress_01 = 1;
      }
      rotate->entity->action = Actions::ROTATING;
      if(fromRedo == FromRedo::No){
        PreRotation(rotate->entity, level, commandBuffer, rotate->from, rotate->to);
      }
      rotate->entity->facing_previous = rotate->from;
      rotate->entity->facing_current = rotate->to;
      if(fromRedo == FromRedo::No){
        PostRotation(rotate->entity, level, commandBuffer, rotate->from, rotate->to);
      }
      break;
    }
  case CMD_TYPE::MODIFY_BEHAVIOUR:{
      ModifyBehaviourCommand modify = cmd.modify;
      if(modify.mode == ModifyBehaviourCommand::ADD){
        AddBehaviour(modify.entity, modify.flag); 
      }
      else{
        RemoveBehaviour(modify.entity, modify.flag); 
      }
      break;
    }
    case CMD_TYPE::ADD:{
      AddCommand* add = &cmd.add;
      AddEntity(add->id, add->x, add->y, level);
      break;
    }
    case CMD_TYPE::REMOVE:{
      RemoveCommand* remove = &cmd.remove;
      RemoveEntity(remove->x, remove->y, level);
      break;          
      }
    case CMD_TYPE::SWAP_ACTIVE:{
      SwapActiveEntityCommand* swap = &cmd.swap_active;
      *swap->value_to_change = swap->index_current;
      break;
      }
    }
}

void Revert(AnyCommand cmd, LevelData* level, CommandBuffer* commandBuffer){    
  switch(cmd.command.type){
    case CMD_TYPE::NONE:
      break;
    case CMD_TYPE::MOVE:{
      MoveCommand mv = cmd.move;
      mv.entity->x -= mv.xDir;      
      mv.entity->y -= mv.yDir;
      mv.entity->progress_01 = 1;
      }
       break;
    case CMD_TYPE::ROTATE:{
      RotateCommand rotate = cmd.rotate;
      if(!HasBehaviour(rotate.entity, CAN_ROTATE)){
        break;
      }
      rotate.entity->facing_current = rotate.from;
      rotate.entity->progress_01 = 1;
      break;
      }
    case CMD_TYPE::MODIFY_BEHAVIOUR: {
      ModifyBehaviourCommand modify = cmd.modify;
      if(modify.mode == ModifyBehaviourCommand::ADD){
        RemoveBehaviour(modify.entity, modify.flag); 
      }
      else{
        AddBehaviour(modify.entity, modify.flag); 
      }
      break;
    }
    case CMD_TYPE::ADD:{
        AddCommand* add = &cmd.add;
        RemoveEntity(add->x, add->y, level);
        break;          
      }
    case CMD_TYPE::REMOVE:{
        RemoveCommand* remove = &cmd.remove;
        AddEntity(remove->storedID, remove->x, remove->y, level);
        Entity* entity = GetEntity(level, remove->x, remove->y);
        SetBehaviour(entity, remove->storedBehaviour); 
        break;            
      }
    case CMD_TYPE::SWAP_ACTIVE:{
        SwapActiveEntityCommand* swap = &cmd.swap_active;
        *swap->value_to_change = swap->index_previous;
        break;
      }
    }
}

void Queue(CommandBuffer *buffer, AnyCommand cmd){
  assert(cmd.command.type != CMD_TYPE::NONE);

  if (buffer->head > buffer->index){
    buffer->head = buffer->index;
    buffer->queueHead = buffer->index;
  }
  assert(buffer->queueHead < buffer->capacity);

  buffer->allCommands[buffer->queueHead] = cmd;
  buffer->allCommands[buffer->queueHead].command.timestamp = buffer->timestamp;
  buffer->queueHead++;
}

void Push(CommandBuffer* buffer, AnyCommand cmd, LevelData* level){
  assert(buffer->queueHead == buffer->index); // Trying to push to non flushed queue!
  
  buffer->head = buffer->index;
  Queue(buffer, cmd);
  Execute(cmd, level, buffer, FromRedo::No);
  buffer->index = buffer->queueHead;
  buffer->head = buffer->queueHead;
}

void Flush(CommandBuffer* buffer, LevelData* level){
  assert(buffer->index < buffer->queueHead); // Nothing to flush
  while(buffer->index < buffer->queueHead){
    AnyCommand command = buffer->allCommands[buffer->index];
    Execute(command, level, buffer);
    buffer->index++;
  }
  buffer->head = buffer->index;
}

void CancleQueue(CommandBuffer* buffer, LevelData* level){
  assert(buffer->queueHead > buffer->head); // No queue to cancel
  buffer->queueHead = buffer->head;
  buffer->index = buffer->head;
}

void Undo(CommandBuffer* cmdBuffer, LevelData* level){
  if(cmdBuffer->index == 0){
    return;
  }
  cmdBuffer->index--;
  cmdBuffer->queueHead = cmdBuffer->index;

  AnyCommand cmd = cmdBuffer->allCommands[cmdBuffer->index];
  assert(cmd.command.type != CMD_TYPE::NONE);
  uint32_t timestamp = cmd.command.timestamp;
  Revert(cmd, level, cmdBuffer);

  if(cmdBuffer->index > 0){
    if(cmdBuffer->allCommands[cmdBuffer->index - 1].command.timestamp == timestamp){
      Undo(cmdBuffer, level);
    } 
  }
}

void UndoTo(CommandBuffer* cmdBuffer, LevelData* level, int index){
  assert(cmdBuffer->index > index);
  assert(index >= 0);

  while (cmdBuffer->index > index){
    cmdBuffer->index--;
    AnyCommand command = cmdBuffer->allCommands[cmdBuffer->index];
    Revert(command, level, cmdBuffer);
  }
  cmdBuffer->queueHead = cmdBuffer->index;
  cmdBuffer->head = cmdBuffer->index;
}

void Redo(CommandBuffer* cmdBuffer, LevelData* level){
  AnyCommand cmd = cmdBuffer->allCommands[cmdBuffer->index];
    if(cmdBuffer->index == cmdBuffer->head){
    return;
  }
  assert(cmd.command.type != CMD_TYPE::NONE);
  uint32_t timestamp = cmd.command.timestamp;
  Execute(cmd, level, cmdBuffer, FromRedo::Yes);

  cmdBuffer->index++;
  cmdBuffer->queueHead = cmdBuffer->index;

  if(cmdBuffer->index != cmdBuffer->head){
    AnyCommand nextCommand = cmdBuffer->allCommands[cmdBuffer->index];
    if(nextCommand.command.timestamp == timestamp){
      Redo(cmdBuffer, level);
    }
  }
}
void ResetCommandBuffer(CommandBuffer* buffer){
  buffer->index = 0;
  buffer->queueHead = 0;
  buffer->head = 0;
  buffer->timestamp = 0;
}
