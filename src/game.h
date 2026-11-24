#pragma once

#include "SDL3/SDL_render.h"
#include "gameState.h"

extern "C" {
   __declspec(dllexport) void Initialize(GameData* data, SDL_Window* window, SDL_Renderer* renderer);
   __declspec(dllexport) bool HandleEvents(GameData* data, SDL_Event event);
   __declspec(dllexport) void Draw(GameData* data, SDL_Renderer* renderer);
   __declspec(dllexport) void Update(GameData* data, float dt);
   __declspec(dllexport) void OnQuit(SDL_Renderer* renderer);
   void ChangeScene(GameData* data, SCENE_TYPES new_scene);
   void StartLevel(Gameplay* gameplay, Memory::Arena* arena_commands, Memory::Arena* arena_entities);
}
