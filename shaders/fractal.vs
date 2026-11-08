#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in float z;
layout(location = 2) in float depth;

uniform mat4 mvp;

out float vDepth;

void main()
{
    vec4 clip = mvp * vec4(position, 0.0, 1.0);
    gl_Position = vec4(clip.xy, z * 2.0 - 1.0, clip.w);
    vDepth = depth;
}
