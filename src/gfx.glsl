//------------------------------------------------------------------------------
//  gfx.glsl
//  Shader for rendering the chip visualization.
//------------------------------------------------------------------------------
@ctype vec4 float4_t
@ctype vec2 float2_t

@vs vs
layout(binding=0) uniform vs_params {
    vec4 color0;
    vec2 half_size;
    vec2 offset;
    vec2 scale;
};

const float max_nodes = 8192.0;

layout(binding=0) uniform texture2D palette_tex;
layout(binding=0) uniform sampler palette_smp;
layout(location=0) in vec2 pos;
layout(location=1) in vec2 uv;
out vec4 color;

void main() {
    vec2 p = (pos - half_size) + offset;
    p *= scale;
    gl_Position = vec4(p, 0.5, 1.0);
    float u = (floor(mod(uv.x * 65536.0, 256.0)) + 0.5) / 256.0;
    float v = (floor(uv.x * 256.0) + 0.5) / (max_nodes / 256.0);
    float r = texture(sampler2D(palette_tex, palette_smp), vec2(u, v)).r;
    color = vec4(color0.xyz * r, color0.w);
}
@end

@fs fs_alpha
in vec4 color;
out vec4 frag_color;

void main() {
    frag_color = color;
}
@end

@fs fs_add
in vec4 color;
out vec4 frag_color;

void main() {
    frag_color = vec4(color.rgb * color.a, 1.0);
}
@end

@program shd_alpha vs fs_alpha
@program shd_add vs fs_add
