
#include "spriteLibrary.h"
#include "SDL3_Image/SDL_image.h"
#include "common.h"
#include "entity.h"
#include <cassert>
#include <cmath>

using namespace std;


const char* FALLBACK_PATH = "assets/sprites/fallback.png";

// X axis is negative ofset, so if you want to move the sprite to the left make the first number after the sprite folder path higher

static const SpriteDataEntry all_sprite_data[] = {
  {SPRITE_ID::Fallback, FALLBACK_PATH,8,8},
  {SPRITE_ID::Rock, "assets/sprites/fallback.png"},
  //{SPRITE_ID::Medusa_Rotate, "assets/sprites/medusa_rotate.png", 12, 24, 8, 1},
  /*{
  .id = SPRITE_ID::Medusa_Idle_Left,
  .path = "assets/sprites/medusa_idle_left.png",
  .pivot_x = 12,
  .pivot_y = 24,
  .tileset_cell_count_x = 4,
  .tileset_cell_count_y = 1,
  .framerate = 8
  },
  */
  {SPRITE_ID::Slime, "assets/sprites/slime.png"},
  {SPRITE_ID::Dropshadow, "assets/sprites/dropshadow.png", 256, 128},
  {SPRITE_ID::black_1x1, "assets/sprites/1x1_black.png",0,0},
  {SPRITE_ID::titlescreen_background, "assets/sprites/titlescreen.png"},
  {SPRITE_ID::Space_Test_Background, "assets/sprites/Space_Test_Background.png"},
  {SPRITE_ID::selection_marker, "assets/sprites/selection_marker.png",9,9},
  {.id = SPRITE_ID::dungeon_tileset, .path = "assets/sprites/Lost_In_Orbit_Tileset.png", .pivot_x = 0, .pivot_y = 0, .cell_width = 512, .cell_height = 512},
  {SPRITE_ID::Goal, "assets/sprites/goal.png"},
  {SPRITE_ID::Menu_Horizon, "assets/sprites/mainmenu_background.png"},
  {SPRITE_ID::Menu_Cloud_Back, "assets/sprites/mainmenu_cloud_back.png"},
  {SPRITE_ID::Menu_Cloud_Front, "assets/sprites/mainmenu_cloud_front.png"},
  {SPRITE_ID::Menu_Middle, "assets/sprites/mainmenu_middle.png"},
  {SPRITE_ID::Menu_Front, "assets/sprites/mainmenu_front.png"},
  {SPRITE_ID::Button_Basic, "assets/sprites/basic_button.png",0,0,3,3},
  {SPRITE_ID::Alien_Sprite, "assets/sprites/Test_Entity.png",256,956},
  {SPRITE_ID::Alien_Idle_Right, "assets/sprites/Alien_Idle_Right.png",256,956,2,1,2},
  {SPRITE_ID::Alien_Idle_Left, "assets/sprites/Alien_Idle_Left.png",256,956,2,1,2},
  {SPRITE_ID::Alien_Idle_Up, "assets/sprites/Alien_Idle_Up.png",256,956,2,1,2},
  {SPRITE_ID::Alien_Idle_Down, "assets/sprites/Alien_Idle_Down.png",256,956,2,1,2},
  {SPRITE_ID::Alien_Walk_Right, "assets/sprites/Alien_Walk_Right.png", 256, 956, 4, 1, 2},
  {SPRITE_ID::Alien_Walk_Left, "assets/sprites/Alien_Walk_Left.png", 256, 956, 4, 1, 2},
  {SPRITE_ID::Alien_Walk_Up, "assets/sprites/Alien_Walk_Up.png", 256, 956, 4, 1, 2},
  {SPRITE_ID::Alien_Walk_Down, "assets/sprites/Alien_Walk_Down.png", 256, 956, 4, 1, 2},
  {SPRITE_ID::test_entity, "assets/sprites/fallback.png"},
  {SPRITE_ID::Background_Asteroid_1, "assets/sprites/Background_Asteroid_1.png"},
  {SPRITE_ID::Background_Asteroid_2, "assets/sprites/Background_Asteroid_2.png"},
};

static SpriteRenderInfo WalkFrame(Entity* entity, Sprite* sprite, float cycles = 1.0f){
  int count = GetSpriteCount(sprite);
  int frame = (int)(entity->progress_01 * cycles * count) % count;
  return {frame, sprite};
}

static SpriteRenderInfo IdleFrame(Sprite* sprite, const uint64_t* ticks_total){
  int count = GetSpriteCount(sprite);
  int frame = 0;
  if(count > 1 && sprite->framerate > 0){
    frame = (int)((*ticks_total * sprite->framerate) / FPS % count);
  }
  return {frame, sprite};
}

SpriteRenderInfo GetSprite_FromEntityState(Entity* entity, Sprite* spritebuffer, const uint64_t* ticks_total){  
  /*
  if(entity->id == ENTITY_ID::PLAYER && entity->action == Actions::ROTATING){
  Sprite* spritesheet = GetSprite(SPRITE_ID::Medusa_Rotate, spritebuffer);
  
  int start = 0;
  int end = 0;
  switch(entity->facing_previous){
  case Direction::RIGHT:
  start = 6;
  break;
  case Direction::LEFT:
  start = 2;
  break;
  case Direction::UP:
  start = 4;
  break;
  case Direction::DOWN:
  start = 0;
  break;
  }
  
  switch(entity->facing_current){
  case Direction::RIGHT:
  end = 6;
  break;
  case Direction::LEFT:
  end = 2;
  break;
  case Direction::UP:
  end = 4;
  break;
  case Direction::DOWN:
  end = 0;
  break;
  }
  
  int sprite_count = GetSpriteCount(spritesheet);
  int forward = ((end - start) % sprite_count + sprite_count) % sprite_count;
  int backward = sprite_count - forward;
  end = (forward <= backward) ? (start + forward) : (start - backward);
  int current_frame = (((int)lerp(start, end, entity->progress_01) + sprite_count) % sprite_count);
  return {current_frame, spritesheet};
  }
  */
  switch (entity->id) {
    case ENTITY_ID::PLAYER:{
      
      Sprite* sprite = nullptr;
      int frame = 0;
      switch (entity->facing_current) {
        case Direction::RIGHT:
        if(entity->action == Actions::MOVING){
          sprite = GetSprite(SPRITE_ID::Alien_Walk_Right, spritebuffer);
          return WalkFrame(entity, sprite);
        }
        sprite = GetSprite(SPRITE_ID::Alien_Idle_Right, spritebuffer);
        return IdleFrame(sprite, ticks_total);
        
        case Direction::LEFT:
        if(entity->action == Actions::MOVING){
          sprite = GetSprite(SPRITE_ID::Alien_Walk_Left, spritebuffer);
          return WalkFrame(entity, sprite);
        }
        sprite = GetSprite(SPRITE_ID::Alien_Idle_Left, spritebuffer);
        return IdleFrame(sprite, ticks_total);
        
        case Direction::DOWN: //Up and down is flipped
        if(entity->action == Actions::MOVING){
          sprite = GetSprite(SPRITE_ID::Alien_Walk_Up, spritebuffer);
          return WalkFrame(entity, sprite);
        }
        sprite = GetSprite(SPRITE_ID::Alien_Idle_Up, spritebuffer);
        return IdleFrame(sprite, ticks_total);
        
        case Direction::UP:
        if(entity->action == Actions::MOVING){
          sprite = GetSprite(SPRITE_ID::Alien_Walk_Down, spritebuffer);
          return WalkFrame(entity, sprite);
        }
        sprite = GetSprite(SPRITE_ID::Alien_Idle_Down, spritebuffer);
        return IdleFrame(sprite, ticks_total);
      }
      
      return GetSprite(SPRITE_ID::Alien_Sprite, spritebuffer);
    } 
    case ENTITY_ID::ROCK:
    return GetSprite(SPRITE_ID::Rock, spritebuffer);
    case ENTITY_ID::TEST_ENTITY:
    return GetSprite(SPRITE_ID::test_entity, spritebuffer);
    case ENTITY_ID::SLIME:
    return GetSprite(SPRITE_ID::Slime, spritebuffer);
    default:
    return GetSprite(SPRITE_ID::Fallback, spritebuffer);
  }  
}

Sprite* GetSprite(SPRITE_ID sprite_id, Sprite* spriteBuffer){
  Sprite* sprite = &spriteBuffer[(int)sprite_id];
  if(sprite == nullptr || sprite->texture == nullptr){
    assert(sprite_id != SPRITE_ID::Fallback);
    return GetSprite(SPRITE_ID::Fallback, spriteBuffer);
  }
  return &spriteBuffer[(int)sprite_id];
}

Sprite* GetSpriteFromID(ENTITY_ID id, Sprite* spriteBuffer){
  Sprite* sprite_to_return = nullptr;
  
  switch (id) {
    case ENTITY_ID::ROCK:
    sprite_to_return = &spriteBuffer[(int)SPRITE_ID::Rock];
    break;
    case ENTITY_ID::TEST_ENTITY:
    sprite_to_return = &spriteBuffer[(int)SPRITE_ID::test_entity];
    break;
    case ENTITY_ID::PLAYER:
    sprite_to_return = nullptr;
    break;
    case ENTITY_ID::SLIME:
    sprite_to_return = &spriteBuffer[(int)SPRITE_ID::Slime];
    break;
  }
  
  if(sprite_to_return == nullptr || sprite_to_return->texture == nullptr){
    sprite_to_return = &spriteBuffer[(int)SPRITE_ID::Fallback];
  }
  
  return sprite_to_return;
}



namespace AssetManagement{
  void LoadAllSprites(Sprite* spriteBuffer, SDL_Renderer *renderer){
    for (SpriteDataEntry entry : all_sprite_data) {
      LoadSprite(spriteBuffer, entry, renderer);
    }
  }
  
  void LoadSprite(Sprite* spriteBuffer, SpriteDataEntry entry, SDL_Renderer* renderer){
    SDL_Surface* surface = IMG_Load(entry.path); 
    if(surface == nullptr){
      surface = IMG_Load(FALLBACK_PATH);
    }
    assert(surface != nullptr);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    Sprite* sprite = &spriteBuffer[(int)entry.id];
    sprite->texture = texture;
    sprite->height = texture->h;
    sprite->width = texture->w;
    if(entry.pivot_x == NOT_SET || entry.pivot_y == NOT_SET){
      sprite->pivot_x = sprite->width / 2;
      sprite->pivot_y = sprite->height / 2;
    }
    else{
      sprite->pivot_x = entry.pivot_x;
      sprite->pivot_y = entry.pivot_y;
    }
    
    if(entry.cell_width != NOT_SET && entry.cell_height != NOT_SET){
      sprite->sprite_count_x = sprite->width  / entry.cell_width;
      sprite->sprite_count_y = sprite->height / entry.cell_height;
    }
    else{
      sprite->sprite_count_x = entry.tileset_cell_count_x;
      sprite->sprite_count_y = entry.tileset_cell_count_y;
    }
    sprite->framerate = entry.framerate;
    float cell_w = sprite->sprite_count_x > 0 ? (float)sprite->width / sprite->sprite_count_x : (float)sprite->width;
    sprite->pixels_per_tile = entry.pixels_per_tile > 0 ? entry.pixels_per_tile : cell_w;
    
    SDL_DestroySurface(surface);
  }
}
