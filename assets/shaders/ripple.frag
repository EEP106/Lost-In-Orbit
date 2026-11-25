#version 450

// The tint SDL gives the sprite
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0) uniform RippleParams {
    float time;
    float amplitude;   // fraction of the sprite width
    float frequency;   // waves from top to bottom
    float speed;       // radians per second
    float phase;
    float pad0;
    float pad1;
    float pad2;
};

void main() {
    const float TAU = 6.2831853;

    // two waves at different frequencies so the motion does not look mechanical
    float wave = sin(v_uv.y * frequency * TAU + time * speed + phase)
               + 0.5 * sin(v_uv.y * frequency * 2.3 * TAU - time * speed * 1.3 + phase);

    vec2 uv = v_uv;
    uv.x += wave * amplitude / 1.5;

    out_color = texture(u_texture, uv) * v_color;
}
