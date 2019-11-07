//------------------------------------------------------------------------------
//  chipvis.glsl
//  Shader for rendering the chip visualization.
//------------------------------------------------------------------------------
@ctype vec4 float4_t
@ctype vec2 float2_t

@vs vs
uniform vs_params {
    vec4 color0;
    vec2 half_size;
    vec2 offset;
    vec2 scale;
};
uniform sampler2D palette_tex;
in vec2 pos;
in vec2 uv;
out vec4 color;

void main() {
    vec2 p = (pos - half_size) + offset;
    p *= scale;
    gl_Position = vec4(p, 0.5, 1.0);
    float u = ((uv.x*65535.0) + 0.5f) / 2048.0;
    float r = texture(palette_tex, vec2(u, 0.5)).r;
    color = vec4(color0.xyz, r);
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

@program chipvis_alpha vs fs_alpha
@program chipvis_add vs fs_add
