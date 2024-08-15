#version 330 core

in vec4 v_Color;
in vec2 v_Texcoord;
in float v_Alpha;
uniform vec4 Ambient;

uniform sampler2D v_Texture;

void main()
{
    gl_FragColor = vec4(texture(v_Texture, v_Texcoord).xyz, v_Alpha);
}