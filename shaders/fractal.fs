#version 330 core

in float vDepth;

out vec4 out_col;

void main()
{
    out_col = vec4(mix(vec3(1.0, 0.2, 0.2), vec3(0.2, 0.2, 1.0), vDepth), 1.0);
}
