#include "leveleditor.h"
#include "command.h"
#include "imgui/imgui.h"
#include "levels.h"
#include "rendering.h"
#include "spriteLibrary.h"

namespace EDITOR {
  void DrawObjectPanel(Editor* editor, Sprite* spriteBuffer){
    ImGui::Begin("objects");
    ImVec2 size = {32, 32};

    if(ImGui::ImageButton("Rock", (ImTextureID)GetSprite(SPRITE_ID::Rock, spriteBuffer)->texture, size)){
     editor->object_to_place_id = ENTITY_ID::ROCK;
    }
    
    ImGui::SameLine();
    if(ImGui::ImageButton("Alien", (ImTextureID)GetSprite(SPRITE_ID::Alien_Sprite, spriteBuffer)->texture, size)){
     editor->object_to_place_id = ENTITY_ID::PLAYER;
    }

    ImGui::End();
  }

  void PlaceObject(const int x, const int y, Editor* editor, LevelData* level, CommandBuffer* commandbuffer){
    AddCommand add(x, y, editor->object_to_place_id);
    Push(commandbuffer, add, level);
  }

  void DrawPreview(Editor* editor, Input* input, SDL_Renderer* renderer, LevelData* level, Camera* camera, Sprite* spriteBuffer){
    if(ImGui::GetIO().WantCaptureMouse) return;                                        // mouse is over the dev gui

    int x;
    int y;
    camera::WorldToGrid(input->mouse_x, input->mouse_y, &x, &y, level);

    SpriteRenderInfo preview;
    if(editor->object_to_place_id == ENTITY_ID::PLAYER){
      preview = {0, GetSprite(SPRITE_ID::Alien_Sprite, spriteBuffer)};   // first frame of her rotate sheet
    }
    else{
      preview = GetSpriteFromID(editor->object_to_place_id, spriteBuffer);
    }
    RenderSprite_OnTile(preview, level, renderer, camera, x, y, 1, 0.5f);
  }

  void Update(Editor* editor, Input* input, LevelData* level, CommandBuffer* commandBuffer){
    if(ImGui::GetCurrentContext() == nullptr || ImGui::GetIO().WantCaptureMouse){
      return;
    }
    if(MousePressed(input, MouseButtons::LEFT)){
      if(camera::GetIsPointInsideGrid(input->mouse_x, input->mouse_y, level)){
        int x;
        int y;
        camera::WorldToGrid(input->mouse_x, input->mouse_y, &x, &y, level);
        PlaceObject(x, y, editor, level, commandBuffer);
      }
    }
    else if(MousePressed(input, MouseButtons::RIGHT)){
      if(camera::GetIsPointInsideGrid(input->mouse_x, input->mouse_y, level)){
        int x;
        int y;
        camera::WorldToGrid(input->mouse_x, input->mouse_y, &x, &y, level);
        Entity* entity = GetEntity(level, x, y);
        if(entity == nullptr){
          return;
        }
        RemoveCommand remove(entity);
        Push(commandBuffer, remove, level);
       }
    }
  }
}

