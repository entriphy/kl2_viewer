#version 330 core

layout (location = 0) in vec4 Position;
layout (location = 1) in vec4 Color;
out vec4 v_Color;

void main()
{
    gl_Position = vec4(Position.xy, 0.9999999, 1.0);
    v_Color = Color;
}