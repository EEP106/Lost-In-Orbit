#include "rippleShaders.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_properties.h"
#include "SDL3/SDL_stdinc.h"

namespace {

  const char* PROP_SHADER = "lio.shader.ripple";
  const char* PROP_FAILED = "lio.shader.ripple.failed";
  const char* RIPPLE_SPIRV_PATH = "assets/shaders/ripple.frag.spv";

  struct RippleUniforms {
    float time; // so that the ripples freeze when paused
    float amplitude; // how much the ripple moves the pixels
    float frequency; // how many ripples fit in the screen from top to bottom
    float speed;
    float phase;
    float pad0;
    float pad1;
    float pad2;
  };

  void SlotPropertyName(char* out, size_t size, int slot){
    SDL_snprintf(out, size, "lio.shader.ripple.state.%d", slot);
  }

  SDL_GPUShader* GetRippleShader(SDL_Renderer* renderer){
    SDL_PropertiesID props = SDL_GetGlobalProperties();
    SDL_GPUShader* shader = (SDL_GPUShader*)SDL_GetPointerProperty(props, PROP_SHADER, nullptr);
    if(shader != nullptr){
      return shader;
    }

    SDL_GPUDevice* device = SDL_GetGPURendererDevice(renderer);
    if(device == nullptr){
      SDL_Log("ripple shader: renderer is not a GPU renderer, effect disabled");
      return nullptr;
    }
    if((SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_SPIRV) == 0){
      SDL_Log("ripple shader: GPU device does not accept SPIR-V, effect disabled");
      return nullptr;
    }

    size_t code_size = 0;
    void* code = SDL_LoadFile(RIPPLE_SPIRV_PATH, &code_size);
    if(code == nullptr){
      SDL_Log("ripple shader: could not load %s: %s", RIPPLE_SPIRV_PATH, SDL_GetError());
      return nullptr;
    }

    SDL_GPUShaderCreateInfo info = {};
    info.code = (const Uint8*)code;
    info.code_size = code_size;
    info.entrypoint = "main";
    info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    info.num_samplers = 1;          // the sprite texture
    info.num_uniform_buffers = 1;   // RippleParams

    shader = SDL_CreateGPUShader(device, &info);
    SDL_free(code);
    if(shader == nullptr){
      SDL_Log("ripple shader: SDL_CreateGPUShader failed: %s", SDL_GetError());
      return nullptr;
    }

    SDL_SetPointerProperty(props, PROP_SHADER, shader);
    return shader;
  }

  SDL_GPURenderState* GetRippleState(SDL_Renderer* renderer, int slot){
    if(slot < 0 || slot >= SHADERS::MAX_RIPPLE_SLOTS){
      return nullptr;
    }

    SDL_PropertiesID props = SDL_GetGlobalProperties();
    if(SDL_GetBooleanProperty(props, PROP_FAILED, false)){
      return nullptr;   // do not retry (and spam the log) every frame
    }

    char name[64];
    SlotPropertyName(name, sizeof(name), slot);
    SDL_GPURenderState* state = (SDL_GPURenderState*)SDL_GetPointerProperty(props, name, nullptr);
    if(state != nullptr){
      return state;
    }

    SDL_GPUShader* shader = GetRippleShader(renderer);
    if(shader == nullptr){
      SDL_SetBooleanProperty(props, PROP_FAILED, true);
      return nullptr;
    }

    SDL_GPURenderStateCreateInfo createinfo = {};
    createinfo.fragment_shader = shader;
    state = SDL_CreateGPURenderState(renderer, &createinfo);
    if(state == nullptr){
      SDL_Log("ripple shader: SDL_CreateGPURenderState failed: %s", SDL_GetError());
      SDL_SetBooleanProperty(props, PROP_FAILED, true);
      return nullptr;
    }

    SDL_SetPointerProperty(props, name, state);
    return state;
  }
}

namespace SHADERS {

  void BeginRipple(SDL_Renderer* renderer, int slot, const RippleParams& params){
    SDL_GPURenderState* state = GetRippleState(renderer, slot);
    if(state == nullptr){
      return;
    }

    RippleUniforms uniforms = {};
    uniforms.time = params.time;
    uniforms.amplitude = params.amplitude;
    uniforms.frequency = params.frequency;
    uniforms.speed = params.speed;
    uniforms.phase = params.phase;

    SDL_SetGPURenderStateFragmentUniforms(state, 0, &uniforms, sizeof(uniforms));
    SDL_SetGPURenderState(renderer, state);
  }

  void EndRipple(SDL_Renderer* renderer){
    SDL_SetGPURenderState(renderer, nullptr);
  }

  void KillRipple(SDL_Renderer* renderer){
    SDL_PropertiesID props = SDL_GetGlobalProperties();
    SDL_SetGPURenderState(renderer, nullptr);

    for(int slot = 0; slot < MAX_RIPPLE_SLOTS; slot++){
      char name[64];
      SlotPropertyName(name, sizeof(name), slot);
      SDL_GPURenderState* state = (SDL_GPURenderState*)SDL_GetPointerProperty(props, name, nullptr);
      if(state != nullptr){
        SDL_DestroyGPURenderState(state);
        SDL_ClearProperty(props, name);
      }
    }

    SDL_GPUShader* shader = (SDL_GPUShader*)SDL_GetPointerProperty(props, PROP_SHADER, nullptr);
    SDL_GPUDevice* device = SDL_GetGPURendererDevice(renderer);
    if(shader != nullptr && device != nullptr){
      SDL_ReleaseGPUShader(device, shader);
    }
    SDL_ClearProperty(props, PROP_SHADER);
    SDL_ClearProperty(props, PROP_FAILED);
  }
}
