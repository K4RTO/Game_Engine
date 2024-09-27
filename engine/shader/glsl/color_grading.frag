#version 310 es

#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(input_attachment_index = 0, set = 0, binding = 0) uniform highp subpassInput in_color;

layout(set = 0, binding = 1) uniform sampler2D color_grading_lut_texture_sampler;

layout(location = 0) out highp vec4 out_color;

void main()
{
    highp ivec2 lut_tex_size = textureSize(color_grading_lut_texture_sampler, 0);
    highp float _COLORS      = float(lut_tex_size.y);

    highp vec4 color = subpassLoad(in_color).rgba;
    

    highp vec3 scaled_color = color.rgb * (_COLORS - 1.0) / _COLORS;
    

    highp vec2 uv;
    uv.x = (floor(scaled_color.b) + 0.5) / _COLORS + scaled_color.r / (_COLORS * _COLORS);
    uv.y = scaled_color.g;

    highp vec3 graded_color = texture(color_grading_lut_texture_sampler, uv).rgb;
    

    out_color = vec4(graded_color, color.a);
}
