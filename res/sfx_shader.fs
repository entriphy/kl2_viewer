#version 330 core

in vec4 v_Color;
in vec2 v_Texcoord;
out vec4 FragColor;

uniform sampler2D v_Texture;

void main()
{
    gl_FragColor = texture(v_Texture, v_Texcoord) * vec4(v_Color.xyz, 254.0 / 255.0); // The game uses a fixed alpha value of 127
}