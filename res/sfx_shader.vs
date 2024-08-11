#version 330 core

layout (location = 0) in vec4 Position;
layout (location = 1) in vec4 Normal;
layout (location = 2) in vec4 Texcoord;
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
uniform mat4 LightColor;
uniform mat4 NormalLight;

// Skinning
uniform mat4 Bones[64]; // World transform matrix for each joint
uniform mat4 NormalLights[64];

// Mime
uniform float MimeWeight;

// Fragment output
out vec4 v_Color;
out vec2 v_Texcoord;

void main() {
    vec4 Pos = MimeWeight > 0.0 ? mix(MimePosition0, MimePosition1, MimeWeight) : Position;
    vec4 Norm = MimeWeight > 0.0 ? mix(MimeNormal0, MimeNormal1, MimeWeight) : Normal;
    
    mat4 Skin = 
        Weights[0] * Bones[Joints[0]] +
        Weights[1] * Bones[Joints[1]] +
        Weights[2] * Bones[Joints[2]] +
        Weights[3] * Bones[Joints[3]];
    
    vec4 NormalWeights = Weights / 255.0;
    vec4 Light = 
        NormalWeights[0] * NormalLights[Joints[0]] * Norm +
        NormalWeights[1] * NormalLights[Joints[1]] * Norm +
        // NormalWeights[2] * NormalLights[Joints[2]] * Norm +
        NormalWeights[2] * ( // what
            NormalLights[Joints[1]][0] * Norm.x +
            NormalLights[Joints[1]][1] * Norm.y +
            NormalLights[Joints[2]][1] * Norm.y +
            NormalLights[Joints[2]][2] * Norm.z) +
        NormalWeights[3] * NormalLights[Joints[3]] * Norm;

    Light *= max(Light, vec4(0.0, 0.0, 0.0, 0.0));
    vec4 Color = clamp(LightColor * vec4(Light.xyz, 1.0), vec4(0.0, 0.0, 0.0, 1.0), vec4(1.0, 1.0, 1.0, 1.0));
    
    gl_Position = Projection * View * Model * Skin * Pos;
    v_Color = Color * 2; // I think it has to be multiplied by 2 since the PS2 uses 0 - 128 for alpha values? idk
    v_Texcoord = vec2(Texcoord.xy);
}
