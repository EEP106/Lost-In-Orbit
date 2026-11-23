#include "dev_gui.h"
#include "FMOD/fmod.h"
#include "SDL3/SDL_video.h"
#include "command.h"
#include "common.h"
#include "gameState.h"
#include "game.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdlrenderer3.h"
#include "SDL3/SDL_render.h"
#include "imgui/imgui_internal.h"
#include <cstdio>
#include <cstring>
#include <cstddef>
#include <string>
#include <filesystem>
#include <system_error>

using namespace std;

void DEV::Initialize(SDL_Window* window, SDL_Renderer* renderer){
  ImGui::CreateContext();
  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);
  ImGuiStyle* style = &ImGui::GetStyle();
  // style->ScaleAllSizes(5.0);
  style->FontSizeBase = 18;
  ImGuiIO& io = ImGui::GetIO();
  int w, h;
  SDL_GetWindowSize(window, &w, &h);
  io.DisplaySize = ImVec2((float)w, (float)h);
}

void DEV::ProcessEvents(SDL_Event* event){
  ImGui_ImplSDL3_ProcessEvent(event);
}
void DEV::PreDraw(ImGuiContext* saved_context){
  if(ImGui::GetCurrentContext() == nullptr){
    ImGui::SetCurrentContext(saved_context);
  }
  
  ImGui::NewFrame();
}

void Draw_Imgui_Arena_Usage(Arena* arena, string name_of_arena){
  float fraction = (float)arena->used / (float)arena->size;
  string barText = name_of_arena;
  barText += " " + to_string(arena->used);
  barText += " / " + to_string(arena->size);
  ImGui::ProgressBar(fraction, ImVec2(-1,0), barText.c_str());
}

void DrawFPS(GameData* data){
  EditorData* editor = &data->editor_data;
  editor->fps_buffer[editor->fps_buffer_index++] = 1.0 / *data->dt * *data->dt_scaler;
  editor->fps_buffer_index %= editor->fps_buffer_count;
  ImGui::PlotHistogram("fps", editor->fps_buffer, editor->fps_buffer_count,0,nullptr ,0,FPS, ImVec2(-1,35));
}

 
void DrawAudioMemoryPanel(AudioSystem* audio) {
  int current_bytes = 0;
  FMOD_Memory_GetStats(&current_bytes, NULL, false);

  float fraction = (float)current_bytes / (float)AUDIO_MEMORY_ALLOWANCE;

  char overlay[64];
  snprintf(overlay, sizeof(overlay), "%d KB / %d KB", current_bytes / 1024, (int)(AUDIO_MEMORY_ALLOWANCE / 1024));

  ImGui::Text("FMOD Memory Pool");
  ImGui::ProgressBar(fraction, ImVec2(-1, 0), overlay);
}

void Draw_History(LevelData* level, CommandBuffer* commandBuffer){
  int sliderPos = commandBuffer->index;

  if(ImGui::SliderInt("history",&sliderPos, 0, commandBuffer->head)){
    while(commandBuffer->index > sliderPos){
      Undo(commandBuffer, level);
    }
    while(commandBuffer->index < sliderPos){
      Redo(commandBuffer, level);
    }
  }
}

void Draw_Level_List(GameData* data){
  Gameplay* gameplay = &data->scenes.gameplay;
  if(!gameplay->initialized) return;

  ImGui::Text("levels");
  for(int i = 0; i < gameplay->loadedLevelCount; i++){
    const char* path = gameplay->levels[i].level_path;
    const char* name = strrchr(path, '/');

    char label[128];
    snprintf(label, sizeof(label), "%d: %s", i, name);

    ImGui::PushID(i);
    if(ImGui::Selectable(label, i == gameplay->currentLevelIndex)){
      gameplay->currentLevelIndex = i;
      if(data->scene_current != SCENE_TYPES::GAME){
        ChangeScene(data, SCENE_TYPES::GAME);      // starts the level as part of the scene change
      }
      else{
        StartLevel(gameplay, data->arena_commands, data->arena_entities);
      }
    }
    ImGui::PopID();
  }
}

void DEV::Draw(GameData* data, SDL_Renderer* renderer){
  ImGui::Begin("Dev Tools");
  ImGui::Text("memory arena usage amount");
  
  Draw_Imgui_Arena_Usage(data->arena_main, "all memory");
  Draw_Imgui_Arena_Usage(data->arena_images, "images");
  Draw_Imgui_Arena_Usage(data->arena_levels, "levels");
  Draw_Imgui_Arena_Usage(data->arena_commands, "commands");
  Draw_Imgui_Arena_Usage(data->arena_entities, "entities");
  Draw_Imgui_Arena_Usage(data->arena_input, "input");
  Draw_Imgui_Arena_Usage(data->arena_scratch, "scratch");

  if(data->audio.initialized){
    DrawAudioMemoryPanel(&data->audio);
  }
 
  Draw_History(GetCurrentLevel(&data->scenes.gameplay), data->scenes.gameplay.commandBuffer);
  DrawFPS(data);

  ImGui::SliderFloat("deltaTimeScaler", data->dt_scaler, 0.1, 3);  
  
  Draw_Level_List(data);

  if(ImGui::Button("reload level from file")){
    Gameplay* gameplay = &data->scenes.gameplay;
    if(gameplay->initialized){
      LevelData* level = GetCurrentLevel(gameplay);

      namespace fs = std::filesystem;
      fs::path source = fs::path(SOURCE_ASSETS_DIR) / fs::path(level->level_path).lexically_relative("assets");
      std::error_code ec;
      fs::copy_file(source, level->level_path, fs::copy_options::overwrite_existing, ec);
      if(ec){
        printf("could not copy %s: %s\n", source.string().c_str(), ec.message().c_str());
      }

      CreateLevel(data->arena_levels, level, const_cast<Tileset*>(level->tileset), level->level_path);
      StartLevel(gameplay, data->arena_commands, data->arena_entities);
    }
  }

  ImGui::End();

  if(data->editor_data.edit_level){
    EDITOR::DrawObjectPanel(&data->editor_data.editor, data->spriteBuffer);
    EDITOR::DrawPreview(&data->editor_data.editor, &data->input, renderer, GetCurrentLevel(&data->scenes.gameplay), &data->camera, data->spriteBuffer);
   }

  ImGui::Render();
  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer );
}



