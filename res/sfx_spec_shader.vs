#version 330 core

layout (location = 0) in vec4 Position;
layout (location = 1) in vec4 Normal;
layout (location = 3) in vec4 Weights;
layout (location = 4) in ivec4 Joints;
layout (location = 5) in vec4 MimePosition0;
layout (location = 6) in vec4 MimePosition1;
layout (location = 7) in vec4 MimeNormal0;
layout (location = 8) in vec4 MimeNormal1;

// Scene
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;
uniform vec4 Ambient;

// Skinning
uniform mat4 Bones[64];
uniform mat4 Spec[64];

// Mime
uniform float MimeWeight;

// Fragment output
out vec4 v_Color;
out vec2 v_Texcoord;
out float v_Alpha;

// Cv = ((A - B) * C) / 0x80 + D
// SFX:  A = src, B = fb, C = 0x7F,  D = fb
// Spec: A = src, B = 0,  C = alpha, D = fb

void main() {
    vec4 Pos = MimeWeight > 0.0 ? mix(MimePosition0, MimePosition1, MimeWeight) : Position;
    vec4 Norm = MimeWeight > 0.0 ? mix(MimeNormal0, MimeNormal1, MimeWeight) : Normal;
    
    mat4 Skin = 
        Weights[0] * Bones[Joints[0]] +
        Weights[1] * Bones[Joints[1]] +
        Weights[2] * Bones[Joints[2]] +
        Weights[3] * Bones[Joints[3]];
    
    vec4 NormalWeights = Weights / 255.0;
    vec4 Texcoord = 
        NormalWeights[0] * Spec[Joints[0]] * Norm +
        NormalWeights[1] * Spec[Joints[1]] * Norm +
        NormalWeights[2] * Spec[Joints[2]] * Norm +
        NormalWeights[3] * Spec[Joints[3]] * Norm;

    Texcoord += 1.0;
    Texcoord /= 4.0;
    
    gl_Position = Projection * View * Model * Skin * Pos;
    v_Texcoord = vec2(Texcoord.xy);
    v_Alpha = min(Ambient[0] + Ambient[1] + Ambient[2], 0.8) / 0.8;
    // v_Alpha = 0.5;
    // v_Alpha = 104.0 / 128.0;
}
