#include "game.h"
#include "SDL3/SDL_blendmode.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_scancode.h"
#include "arena.h"
#include "audioSystem.h"
#include "button.h"
#include "common.h"
#include "dev_gui.h"
#include "command.h"
#include "entity.h"
#include "fontLibrary.h"
#include "gameState.h"
#include "imgui/imgui.h"
#include "input.h"
#include "leveleditor.h"
#include "levels.h"
#include "mainmenu.h"
#include "rendering.h"
#include "spriteLibrary.h"
#include "levelRenderer.h"
#include "tilesetLibrary.h"
#include "rippleShaders.h"
#include "movement.h"
#include "slime.h"
#include <cmath>
#include <algorithm>

// bool to control whether walkable outlines are shown
static bool g_show_walkable_outlines = false;

// Every level needs it's path here
static const char* LEVEL_PATHS[] = {
  "assets/levels/level_Test.tmj",
  "assets/levels/level_01.tmj",
  "assets/levels/level_02.tmj",
  "assets/levels/level_03.tmj",
  "assets/levels/level_04.tmj",
  "assets/levels/level_05.tmj",
  "assets/levels/level_06.tmj",
  "assets/levels/level_07.tmj",
  "assets/levels/level_08.tmj",
  "assets/levels/level_09.tmj",
  "assets/levels/level_10.tmj",
};
// Every level need a level text or it crashes
static const char* LEVEL_TEXTS[] = {
  "Test level", // level_Test
  "Arrow keys to move (press ESC to pause to look at the controls)", // level_01
  "Slime sticks to you", // level_02
  "Use the slime to cross gaps", // level_03
  "Use the enviroment to move inside the slime", // level_04
  "If slime is not connected to another slime it will detatch", // level_05
  " ", // level_06
  " ", // level_07
  " ", // level_08
  " ", // level_09
  " ", // level_10
};

extern "C" {
  
  void InitializeGame(Gameplay* gameplay, Arena* arena_levels, Tileset* tilesetBuffer){
    gameplay->paused = false;
    gameplay->pause_index = 0;
    assert(gameplay->initialized == false);
    gameplay->loadedLevelCount = 0;
    
    for(const char* path : LEVEL_PATHS){
      LevelData* level = &gameplay->levels[gameplay->loadedLevelCount++];
      *level = {};
      CreateLevel(arena_levels, level, &tilesetBuffer[(int)TILESETS::Dungeon], path);
    }
    
    gameplay->initialized = true;
  }
  
  void Initialize(GameData* data, SDL_Window* window, SDL_Renderer* renderer){
    data->editor_data.editor.object_to_place_id = ENTITY_ID::ROCK;
    *data->ticks_total = 0;
    DEV::Initialize(window, renderer);
    InitializeAudioSystem(&data->audio, data->arena_main);
    AssetManagement::LoadAllSFX(&data->audio);
    AssetManagement::LoadAllSprites(data->spriteBuffer, renderer);
    data->imGui_context = ImGui::GetCurrentContext();
    AssetManagement::LoadFont(renderer, "assets/fonts/ByteBounce.ttf", &data->font, 48);
    AssetManagement::LoadAllTilesets(data->tilesetBuffer, data->arena_images);
    
    SDL_Texture* blackfade = GetSprite(SPRITE_ID::black_1x1, data->spriteBuffer)->texture;
    SDL_SetTextureBlendMode(blackfade, SDL_BLENDMODE_BLEND);
    InitializeGame(&data->scenes.gameplay, data->arena_levels, data->tilesetBuffer);
    InitializeMenu(&data->scenes.mainMenu, data->spriteBuffer, &data->font, data->arena_main);
    PlaySong(SONG_ID::THEME);
    ChangeScene(data, SCENE_TYPES::MAINMENU);
  }
  
  void StartLevel(Gameplay* gameplay, Arena* arena_commands, Arena* arena_entities){
    ResetCommandBuffer(gameplay->commandBuffer);
    Reset(arena_commands);    
    CreateEntities(&gameplay->levels[gameplay->currentLevelIndex], arena_entities);
    gameplay->activePlayerIndex = 0;
  }
  
  void ChangeScene(GameData* data, SCENE_TYPES new_scene){
    assert(new_scene != data->scene_current);
    data->scene_previous = data->scene_current;
    data->scene_current = new_scene;
    data->transition.state = data->scene_previous == SCENE_TYPES::NONE ? Transition::FadeFrom : Transition::FadeTo;
    data->transition.fade_time_elapsed = 0;
    switch (data->scene_current) {
      case SCENE_TYPES::TITLESCREEN:
      data->transition.fade_time_duration = 1;
      break;          
      case SCENE_TYPES::MAINMENU:
      
      break;
      case SCENE_TYPES::GAME:{
        data->transition.fade_time_duration = 0.5f;
        Gameplay* gameplay = &data->scenes.gameplay;
        assert(gameplay->initialized);
        StartLevel(gameplay, data->arena_commands, data->arena_entities);
        break;
      }
      case SCENE_TYPES::CREDITS:
      break;
      case SCENE_TYPES::NONE:
      assert(false);
      break;
    }
  }
  
  bool HandleEvents(GameData *data, SDL_Event event){
    DEV::ProcessEvents(&event);
    
    if(event.type != SDL_EVENT_KEY_DOWN){
      return true;
    }
    if(event.key.key == SDLK_ESCAPE && data->scene_current != SCENE_TYPES::GAME){
      return false;
    }
    
    return true;
  }
  
  void UpdateTitlescreen(TitleScreen* titlescreen, const float dt){
  }
  static const int PAUSE_BUTTON_COUNT = 3;
  static Button g_pause_buttons[PAUSE_BUTTON_COUNT];
  static bool g_pause_ready = false;
  static float g_pause_w = SCREEN_WIDTH;
  static float g_pause_h = SCREEN_HEIGHT;
  static const int SPEED_UP_MULTIPLIER = 2;
  
  static void InitPauseButtons(GameData* data){
    if(g_pause_ready) return;
    const ButtonType types[PAUSE_BUTTON_COUNT] = {ButtonType::RESUME, ButtonType::MAIN_MENU, ButtonType::QUIT};
    const char* labels[PAUSE_BUTTON_COUNT] = {"Resume", "Main Menu", "Quit"};
    for(int i = 0; i < PAUSE_BUTTON_COUNT; i++){
      SetupButton(&g_pause_buttons[i], types[i], data->spriteBuffer, {0, 0, 300, 80}, Alignment::Centered, &data->font, labels[i]);
      g_pause_buttons[i].is_dynamic = true;
      g_pause_buttons[i].sprite = GetSprite(SPRITE_ID::Button_Basic, data->spriteBuffer);
    }
    g_pause_ready = true;
  }
  
  static void LayoutPauseButtons(){
    const float spacing = 100.0f;
    for(int i = 0; i < PAUSE_BUTTON_COUNT; i++){
      Button* b = &g_pause_buttons[i];
      b->rect.x = g_pause_w / 2.0f - b->rect.w / 2.0f;
      b->rect.y = g_pause_h / 2.0f + (i - (PAUSE_BUTTON_COUNT - 1) / 2.0f) * spacing - b->rect.h / 2.0f;
    }
  }
  
  static void PressPauseButton(Button* button, GameData* data){
    switch(button->type){
      case ButtonType::RESUME:
      data->scenes.gameplay.paused = false;
      break;
      case ButtonType::MAIN_MENU:
      data->scenes.gameplay.paused = false;
      ChangeScene(data, SCENE_TYPES::MAINMENU);
      break;
      case ButtonType::QUIT:
      data->running = false;
      break;
      default:
      break;
    }
  }
  
  static void UpdatePauseMenu(GameData* data){
    Input* input = &data->input;
    int& index = data->scenes.gameplay.pause_index;
    InitPauseButtons(data);
    LayoutPauseButtons();
    
    if(input->mouse_magnitude > 0.1){
      for(int i = 0; i < PAUSE_BUTTON_COUNT; i++){
        if(IsHoveredOver(&g_pause_buttons[i], input->mouse_x, input->mouse_y)){
          index = i;
          break;
        }
      }
    }
    if(KeyPressed(input, SDL_SCANCODE_UP)){
      index = (index - 1 + PAUSE_BUTTON_COUNT) % PAUSE_BUTTON_COUNT;
    }
    if(KeyPressed(input, SDL_SCANCODE_DOWN)){
      index = (index + 1) % PAUSE_BUTTON_COUNT;
    }
    
    Button* selected = &g_pause_buttons[index];
    bool clicked = MousePressed(input, MouseButtons::LEFT) && IsHoveredOver(selected, input->mouse_x, input->mouse_y);
    if(KeyPressed(input, SDL_SCANCODE_RETURN) || clicked){
      PressPauseButton(selected, data);
    }
  }
  
  static void DrawPauseMenu(GameData* data, SDL_Renderer* renderer, float w, float h){
    g_pause_w = w;
    g_pause_h = h;
    InitPauseButtons(data);
    LayoutPauseButtons();
    int index = data->scenes.gameplay.pause_index;
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
    SDL_FRect full = {0, 0, w, h};
    SDL_RenderFillRect(renderer, &full);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    for(int i = 0; i < PAUSE_BUTTON_COUNT; i++){
      Button* b = &g_pause_buttons[i];
      if(b->is_dynamic){
        RenderButton_Dynamic(b, i == index, renderer);
      }
      else{
        RenderButton(b, i == index, renderer);
      }
    }
  }
  enum class QueueActionResult {Nothing, Rotate, Move, MoveFailed};
  
  QueueActionResult QueueAction(Gameplay* gameplay, LevelData* level, Arena* arena_scratch){
    Entity* entity = GetActiveEntity(gameplay);
    
    if(!HasBehaviour(entity, (Behaviour)(RESPOND_TO_INPUT | CAN_MOVE))){
      return QueueActionResult::Nothing;
    }
    
    int xDir = gameplay->input_buffer[gameplay->input_buffer_read_count % gameplay->input_buffer_capacity].x;
    int yDir = gameplay->input_buffer[gameplay->input_buffer_read_count % gameplay->input_buffer_capacity].y;
    
    Direction new_facing = DirectionFromXY(xDir, yDir);
    if(new_facing != entity->facing_current){ 
      RotateCommand rotate(entity, entity->facing_current, new_facing);
      Queue(gameplay->commandBuffer, rotate);
      return QueueActionResult::Rotate;
    }
    
    if(!IsActing(entity)){
      bool moved = TryMove(entity, level, gameplay->commandBuffer, xDir, yDir, entity->strength);
      if(moved){
        PlaySFX(SFX_ID::JUMP);
        MoveSlime(arena_scratch, level, gameplay->commandBuffer, xDir, yDir);
        return QueueActionResult::Move;
      }
      else if (TryShuffle(level, gameplay->commandBuffer, xDir, yDir)){
        return QueueActionResult::Move;
      }
      return QueueActionResult::MoveFailed;
    }
    
    return QueueActionResult::Nothing;
  }
  
  void UpdateGame(Gameplay* gameplay, Input* input, Arena* arena_scratch, Arena* arena_commands, Arena* arena_entities, const float dt){
    
    if(KeyPressed(input, SDL_SCANCODE_R)){ // restart level on `R`
      StartLevel(gameplay, arena_commands, arena_entities);
      return;
    }
    float speed_factor = input->keys_current[SDL_SCANCODE_SPACE] ? (float)SPEED_UP_MULTIPLIER : 1.0f;
    float move_speed = MOVE_SPEED * speed_factor;
    
    bool are_entities_acting = false;
    LevelData *level = GetCurrentLevel(gameplay);
    Entity* entityBuffer = level->entityBuffer;
    
    for (int i = 0; i < level->entityCount; i++){
      if(IsActing(&entityBuffer[i])){
        are_entities_acting = true;
        break;
      }
    }
    
    if(KeyPressed(input, SDL_SCANCODE_Z) || KeyHeld_ForTime(input, SDL_SCANCODE_Z, UNDO_REPEAT_TIME / (speed_factor * 2))){
      ResetKeyHeldTime(input, SDL_SCANCODE_Z);
      if(KeyHeld(input, SDL_SCANCODE_LSHIFT)){
        Redo(gameplay->commandBuffer, GetCurrentLevel(gameplay));
      }
      else{
        Undo(gameplay->commandBuffer, GetCurrentLevel(gameplay));
      }
    }
    bool can_accept_input =! are_entities_acting && gameplay->input_buffer_read_count == gameplay->input_buffer_write_count;
    if (can_accept_input)
    {
      if(KeyPressed(input,SDL_SCANCODE_RIGHT) || KeyHeld_ForTime(input,SDL_SCANCODE_RIGHT, (1 / move_speed) * 1)){
        ResetKeyHeldTime(input, SDL_SCANCODE_RIGHT);
        gameplay->input_buffer[gameplay->input_buffer_write_count++ % gameplay->input_buffer_capacity] = {1, 0};
      }
      else if(KeyPressed(input,SDL_SCANCODE_LEFT) || KeyHeld_ForTime(input,SDL_SCANCODE_LEFT, (1 / move_speed) * 1)){
        ResetKeyHeldTime(input, SDL_SCANCODE_LEFT);
        gameplay->input_buffer[gameplay->input_buffer_write_count++ % gameplay->input_buffer_capacity] = {-1, 0};
      }
      else if(KeyPressed(input,SDL_SCANCODE_UP) || KeyHeld_ForTime(input,SDL_SCANCODE_UP, (1 / move_speed) * 1)){
        ResetKeyHeldTime(input, SDL_SCANCODE_UP);
        gameplay->input_buffer[gameplay->input_buffer_write_count++ % gameplay->input_buffer_capacity] = {0, -1};
      }
      else if(KeyPressed(input,SDL_SCANCODE_DOWN) || KeyHeld_ForTime(input,SDL_SCANCODE_DOWN, (1 / move_speed) * 1)){
        ResetKeyHeldTime(input, SDL_SCANCODE_DOWN);
        gameplay->input_buffer[gameplay->input_buffer_write_count++ % gameplay->input_buffer_capacity] = {0, 1};
      }
    }
    
    
    for (int i = 0; i < level->goalCount; i++) {
      Entity* entity = GetEntity(level, level->goals[i].x, level->goals[i].y);
      if(entity != nullptr && !IsActing(entity)){
        level->goals[i].blink_timer += dt;
      }
      else{
        level->goals[i].blink_timer = 0;
      }
    }
    
    if(level->goalCount > 0){
      int goals_reached = 0;
      for (int i = 0; i < level->goalCount; i++) {
        Goal goal = level->goals[i];
        Entity* entity = GetEntity(level, goal.x, goal.y);
        if(entity == nullptr){
          break;
        }
        else if(HasBehaviour(entity, Behaviour::IS_PLAYER)){
          goals_reached++;
        }
      }
      if(goals_reached == level->goalCount && !are_entities_acting){
        gameplay->currentLevelIndex++;
        StartLevel(gameplay, arena_commands, arena_entities);
        return;
      }      
    }
    
    for (int i = 0; i < level->entityCount; i++){
      Entity* entity = &entityBuffer[i];
      if(!entity->active) continue;
      switch(entity->action){
        case Actions::NONE:
        continue;
        case Actions::MOVING:
        entity->progress_01 += move_speed * dt;
        break;
        case Actions::ROTATING:
        // Change the variable after progress_01 for the speed of rotating (higher is faster)
        entity->progress_01 += 100 * dt;
        break;
      }
    }
    for (int i = 0; i < level->entityCount; i++){
      Entity* entity = &entityBuffer[i];
      if(entity->progress_01 >= 1){
        entity->x_prev = entity->x;
        entity->y_prev = entity->y;
        entity->facing_previous = entity->facing_current;
        entity->action = Actions::NONE;
        entity->progress_01 = 0;
        if(HasBehaviour(entity, Behaviour::IS_PUSHING)){
          RemoveBehaviour(entity, Behaviour::IS_PUSHING);
        }
      }
    }
    
    int player_count = 0;
    for (int i = 0; i < level->entityCount; i++) {
      if(entityBuffer[i].active == false){
        continue;
      }
      if(HasBehaviour(&level->entityBuffer[i], (Behaviour)(IS_PLAYER))){
        player_count++;
      }
    }
    
    int index = 0;
    gameplay->activePlayerBuffer = ALLOC_ARRAY(arena_scratch, Entity*, player_count);
    for (int i = 0; i < level->entityCount; i++) {
      if(entityBuffer[i].active == false){
        continue;
      }
      if(HasBehaviour(&level->entityBuffer[i], (Behaviour)(IS_PLAYER))){
        gameplay->activePlayerBuffer[index++] = &level->entityBuffer[i];
      }
    }
    
    if(are_entities_acting == false && KeyPressed(input, SDL_SCANCODE_X) && player_count > 0){
      SwapActiveEntityCommand swap(&gameplay->activePlayerIndex, player_count);
      Push(gameplay->commandBuffer, swap, GetCurrentLevel(gameplay));
      gameplay->commandBuffer->timestamp += 1;
    }
    
    if(are_entities_acting){
      return;
    }
    if(gameplay->input_buffer_read_count == gameplay->input_buffer_write_count){
      return;
    }
    
    QueueActionResult result = QueueAction(gameplay, level, arena_scratch);
    
    switch (result) {
      case QueueActionResult::Nothing:
      break;
      case QueueActionResult::Rotate:
      Flush(gameplay->commandBuffer, level);
      break;
      case QueueActionResult::Move:{
      int preFlushIndex = gameplay->commandBuffer->index;
      Flush(gameplay->commandBuffer, level);
      gameplay->commandBuffer->timestamp++;
      gameplay->input_buffer_read_count++;

      if (!CheckAllEntitiesSupported(arena_scratch, level)){
        UndoTo(gameplay->commandBuffer, level, preFlushIndex);
      }
      break;}
      case QueueActionResult::MoveFailed:
      gameplay->input_buffer_read_count++;
      break;
    }
  }  
  
  void Update(GameData* data,float dt){
    TitleScreen* titlescreen = &data->scenes.titlescreen;
    Gameplay* gameplay = &data->scenes.gameplay;
    EditorData* editorData = &data->editor_data;
    Transition* transition = &data->transition;
    
    // "Freezes" the game when paused my making animations and movement of background assets pause
    bool is_paused = data->scene_current == SCENE_TYPES::GAME && gameplay->paused;
    if(!is_paused){
      bool speeding_up = data->scene_current == SCENE_TYPES::GAME &&
      data->input.keys_current[SDL_SCANCODE_SPACE];
      *data->ticks_total += speeding_up ? SPEED_UP_MULTIPLIER : 1;
    }
    
    if(KeyPressed(&data->input, SDL_SCANCODE_F2)){
      editorData->edit_level = !editorData->edit_level;
    }
    if(KeyPressed(&data->input, SDL_SCANCODE_TAB)){
      g_show_walkable_outlines = !g_show_walkable_outlines;
    }
    if(editorData->edit_level){
      EDITOR::Update(&editorData->editor, &data->input, GetCurrentLevel(gameplay), gameplay->commandBuffer);
    }
    if(KeyPressed(&data->input, SDL_SCANCODE_5)){
      ChangeScene(data, SCENE_TYPES::TITLESCREEN);
      return;
    }
    
    if(transition->state != Transition::Inactive){
      transition->fade_time_elapsed += dt;
      if(transition->fade_time_elapsed >= transition->fade_time_duration){
        transition->fade_time_elapsed = 0;
        switch (transition->state) {
          case Transition::Inactive:
          break;
          case Transition::FadeTo:
          transition->state = Transition::FadeFrom;
          break;
          case Transition::FadeFrom:
          transition->state = Transition::Inactive;
          break;
        }
      }
    }
    
    switch(data->scene_current){
      case SCENE_TYPES::TITLESCREEN:
      UpdateTitlescreen(titlescreen, dt);
      if(AnyKeyPressed(&data->input)){
        if(transition->state == Transition::FadeTo || transition->state == Transition::Inactive){
          ChangeScene(data, SCENE_TYPES::GAME);
        }
      }
      break;
      case SCENE_TYPES::MAINMENU:
      UpdateMenu(data);
      break;
      case SCENE_TYPES::GAME:
      if(KeyPressed(&data->input, SDL_SCANCODE_ESCAPE)){
        gameplay->paused = !gameplay->paused;
        gameplay->pause_index = 0;
      }
      if(gameplay->paused){
        UpdatePauseMenu(data);
      }
      else{
        UpdateGame(gameplay, &data->input, data->arena_scratch, data->arena_commands, data->arena_entities, dt);
      }
      break;
      case SCENE_TYPES::CREDITS:
      break;
      case SCENE_TYPES::NONE:
      assert(false);
      break;
    }
  }  
  
  static void DrawWalkableOutlines(GameData* data, SDL_Renderer* renderer){
    LevelData* level = GetCurrentLevel(&data->scenes.gameplay);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // color of the outline (r,g,b,alpha)
    
    const float size = TILE_SIZE_PX_SCALED;
    const float gap = size * 0.1f; // gap between the tile edge and the square
    const float border = UPSCALE_FACTOR; // border size based on upscale factor so the tickness is allways the same even at different zoom levels
    
    const float inner = size - 2 * gap;   // side length of the square
    
    for(int gx = 0; gx < level->w; gx++){
      for(int gy = 0; gy < level->h; gy++){
        if(!IsWalkable(gx, gy, level)) continue;
        
        float x = gx;
        float y = gy;
        camera::GridToWorld(&x, &y, level);
        x += gap;
        y += gap;
        
        SDL_FRect top    = {x,                 y,                 inner,  border};
        SDL_FRect bottom = {x,                 y + inner - border, inner,  border};
        SDL_FRect left   = {x,                 y + border,        border, inner - 2 * border};
        SDL_FRect right  = {x + inner - border, y + border,        border, inner - 2 * border};
        SDL_RenderFillRect(renderer, &top);
        SDL_RenderFillRect(renderer, &bottom);
        SDL_RenderFillRect(renderer, &left);
        SDL_RenderFillRect(renderer, &right);
      }
    }
  }
  
  void DrawScene(GameData* data, SCENE_TYPES scene, SDL_Renderer* renderer){
    switch(scene){
      case SCENE_TYPES::TITLESCREEN:
      RenderSprite_World(data->scenes.titlescreen.background, renderer, &data->camera, 0, 0);
      break;
      case SCENE_TYPES::MAINMENU:
      DrawMenu(&data->scenes.mainMenu, renderer, data->spriteBuffer, &data->input);
      break;
      case SCENE_TYPES::GAME:{
        
        Sprite* level_background = GetSprite(SPRITE_ID::Space_Test_Background, data->spriteBuffer);
        int w, h;
        SDL_GetCurrentRenderOutputSize(renderer, &w, &h);
        float ticks = (float)(*data->ticks_total) / FPS;   // freezes while paused
        
        float scale = std::max((float)w / level_background->width, (float)h / level_background->height) / UPSCALE_FACTOR;
        scale *= 1.02f; // 2% scale up so the ripple effect doesn't show the edges of the background sprite
        SHADERS::BeginRipple(renderer, 0, {ticks, 0.004f, 3.0f, 1.2f, 0.0f});
        RenderSprite_World(level_background, renderer, nullptr, w / 2.0f, h / 2.0f, scale);
        SHADERS::EndRipple(renderer);
        
        Sprite* background_asset = GetSprite(SPRITE_ID::Background_Asteroid_1, data->spriteBuffer);
        Sprite* background_asset_2 = GetSprite(SPRITE_ID::Background_Asteroid_2, data->spriteBuffer);
        float asset_scale_background_asset = std::max((float)w / background_asset->width, (float)h / background_asset->height) / (UPSCALE_FACTOR*3);
        float asset_scale_background_asset_2 = std::max((float)w / background_asset_2->width, (float)h / background_asset_2->height) / (UPSCALE_FACTOR*3);
        float asteroid_speed = 0.007f;
        
        float background_asset_x = -0.15f + fmodf(ticks * asteroid_speed, 1.4f);
        float background_asset_y = 0.3f;
        SHADERS::BeginRipple(renderer, 1, {ticks, 0.01f, 2.0f, 0.6f, 1.0f});
        RenderSprite_World(background_asset, renderer, nullptr, w * background_asset_x, h * background_asset_y, asset_scale_background_asset);
        SHADERS::EndRipple(renderer);
        
        float background_asset_2_x = 1.15f - fmodf(ticks * asteroid_speed, 1.4f);
        float background_asset_2_y = 0.7f;
        SHADERS::BeginRipple(renderer, 2, {ticks, 0.01f, 2.0f, 0.6f, 3.0f});
        RenderSprite_World(background_asset_2, renderer, nullptr, w * background_asset_2_x, h * background_asset_2_y, asset_scale_background_asset_2);
        SHADERS::EndRipple(renderer);
        
        RenderLevel(data, renderer);
        if(g_show_walkable_outlines){
          DrawWalkableOutlines(data, renderer);
        }
        RenderEntities(data, renderer);
        
        int level_index = data->scenes.gameplay.currentLevelIndex;
        int text_count = (int)(sizeof(LEVEL_TEXTS) / sizeof(LEVEL_TEXTS[0]));
        if(level_index >= 0 && level_index < text_count && !IsStringEmpty(LEVEL_TEXTS[level_index])){
          RenderText(&data->font, LEVEL_TEXTS[level_index], renderer, nullptr, w / 2.0f, h * 0.1f, Alignment::Centered);
        }
        if(data->scenes.gameplay.paused){
          DrawPauseMenu(data, renderer, (float)w, (float)h);
          RenderText(&data->font, "Arrow keys for movement", renderer, nullptr, 50, h * 0.7f, Alignment::Left);
          RenderText(&data->font, "Z to undo", renderer, nullptr, 50, h * 0.75f, Alignment::Left);
          RenderText(&data->font, "Shift + Z to redo", renderer, nullptr, 50, h * 0.8f, Alignment::Left);
          RenderText(&data->font, "R to reset", renderer, nullptr, 50, h * 0.85f, Alignment::Left);
          RenderText(&data->font, "Space to speed up", renderer, nullptr, 50, h * 0.9f, Alignment::Left);
        }
        break;}
        case SCENE_TYPES::CREDITS:
        break;
        case SCENE_TYPES::NONE:
        assert(false);
        break;
      }
    }
    
    void Draw(GameData* data, SDL_Renderer* renderer){
      DEV::PreDraw(data->imGui_context);
      int w, h;
      SDL_GetCurrentRenderOutputSize(renderer, &w, &h);
      camera::SetScreenSize((float)w, (float)h);
      data->scenes.mainMenu.SetScreenSize((float)w, (float)h);
      SDL_SetRenderDrawColor(renderer, 120, 70, 120, 255);
      SDL_RenderClear(renderer);
      
      switch(data->transition.state){
        case Transition::Inactive:
        DrawScene(data, data->scene_current, renderer);
        break;
        case Transition::FadeTo: {
          DrawScene(data, data->scene_previous, renderer);
          float alpha = data->transition.fade_time_elapsed / data->transition.fade_time_duration;
          RenderSprite_World(GetSprite(SPRITE_ID::black_1x1, data->spriteBuffer), renderer, &data->camera, 0, 0, w,alpha);
          break;
        }
        case Transition::FadeFrom:{
          DrawScene(data, data->scene_current, renderer);
          float alpha = 1 - data->transition.fade_time_elapsed / data->transition.fade_time_duration;
          RenderSprite_World(GetSprite(SPRITE_ID::black_1x1, data->spriteBuffer), renderer, &data->camera, 0, 0, w,alpha);
          break;
        }
      }
      
      DEV::Draw(data, renderer);
      SDL_RenderPresent(renderer);
    }
    
    void OnQuit(SDL_Renderer* renderer){
      SDL_DestroyRenderer(renderer);
      SHADERS::KillRipple(renderer);
    }
    
  }
  
  
  
