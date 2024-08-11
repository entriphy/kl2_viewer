#include <cglm/cglm.h>
#include "vu0.h"

// Basic matrix/vector functions

void Vu0UnitMatrix(mat4 m0) {
    glm_mat4_identity(m0);
}

void Vu0CopyVector(vec4 v0, vec4 v1) {
    glm_vec4_copy(v1, v0);
}

void Vu0CopyVectorXYZ(vec4 v0, vec4 v1) {
    glm_vec4_copy3(v1, v0);
}

void Vu0CopyMatrix(mat4 m0, mat4 m1) {
    glm_mat4_copy(m1, m0);
}

void Vu0TransposeMatrix(mat4 m0, mat4 m1) {
    glm_mat4_transpose_to(m1, m0);
}

// Basic vector arithmetic functions

void Vu0AddVector(vec4 v0, vec4 v1, vec4 v2) {
    glm_vec4_add(v1, v2, v0);
}

void Vu0SubVector(vec4 v0, vec4 v1, vec4 v2) {
    glm_vec4_sub(v1, v2, v0);
}

void Vu0DivVector(vec4 v0, vec4 v1, float q) {
    glm_vec4_divs(v1, q, v0);
}

void Vu0DivVectorXYZ(vec4 v0, vec4 v1, float q) {
    glm_vec3_divs(v1, q, v0);
}

void Vu0MulVector(vec4 v0, vec4 v1, vec4 v2) {
    glm_vec4_mul(v1, v2, v0);
}

float Vu0InnerProduct(vec4 v0, vec4 v1) {
    return glm_vec4_dot(v0, v1);
}

void Vu0OuterProduct(vec4 v0, vec4 v1, vec4 v2) {
    // ?
}

void Vu0ScaleVector(vec4 v0, vec4 v1, float t) {
    glm_vec4_scale(v1, t, v0);
}

void Vu0ScaleVectorXYZ(vec4 v0, vec4 v1, float t) {
    glm_vec3_scale(v1, t, v0);
}

void Vu0Normalize(vec4 v0, vec4 v1) {
    glm_vec4_normalize_to(v1, v0);
}

void Vu0InterVector(vec4 v0, vec4 v1, vec4 v2, float t) {
    glm_vec4_scale(v1, t, v0);
    glm_vec4_muladds(v2, 1.0f - t, v0);
}

void Vu0InterVectorXYZ(vec4 v0, vec4 v1, vec4 v2, float t) {
    glm_vec3_scale(v1, t, v0);
    glm_vec3_muladds(v2, 1.0f - t, v0);
    v0[3] = v1[3];
}

void Vu0ClampVector(vec4 v0, vec4 v1, float min, float max) {
    glm_vec4_clamp(v1, min, max);
}

// Basic matrix arithmetic functions

void Vu0ApplyMatrix(vec4 v0, mat4 m0, vec4 v1) {
    glm_mat4_mulv(m0, v1, v0);
}

void Vu0MulMatrix(mat4 m0, mat4 m1, mat4 m2) {
    glm_mat4_mul(m1, m2, m0);
}

void Vu0RotMatrix(mat4 m0, mat4 m1, vec4 rot) {
    glm_rotate_x(m0, rot[0], m0);
    glm_rotate_y(m0, rot[1], m0);
    glm_rotate_z(m1, rot[2], m0);
    
    
}

void Vu0RotMatrixX(mat4 m0, mat4 m1, float rx) {
    glm_rotate_x(m1, rx, m0);
}

void Vu0RotMatrixY(mat4 m0, mat4 m1, float ry) {
    glm_rotate_y(m1, ry, m0);
}

void Vu0RotMatrixZ(mat4 m0, mat4 m1, float rz) {
    glm_rotate_z(m1, rz, m0);
}

void Vu0TransMatrix(mat4 m0, mat4 m1, vec4 tv) {
    glm_translated_to(m1, tv, m0);
}

void Vu0InversMatrix(mat4 m0, mat4 m1) {
    glm_mat4_inv(m1, m0);
}

// Floating point conversion functions

void Vu0FTOI0Vector(ivec4 v0, vec4 v1) {
    v0[0] = (int)v1[0];
    v0[1] = (int)v1[1];
    v0[2] = (int)v1[2];
    v0[3] = (int)v1[3];
}

void Vu0FTOI4Vector(ivec4 v0, vec4 v1) {
    v0[0] = (int)(v1[0] * 16.0f);
    v0[1] = (int)(v1[1] * 16.0f);
    v0[2] = (int)(v1[2] * 16.0f);
    v0[3] = (int)(v1[3] * 16.0f);
}

void Vu0ITOF0Vector(vec4 v0, ivec4 v1) {
    v0[0] = (float)v1[0];
    v0[1] = (float)v1[1];
    v0[2] = (float)v1[2];
    v0[3] = (float)v1[3];
}

void Vu0ITOF4Vector(vec4 v0, ivec4 v1) {
    v0[0] = v1[0] / 16.0f;
    v0[1] = v1[1] / 16.0f;
    v0[2] = v1[2] / 16.0f;
    v0[3] = v1[3] / 16.0f;
}

void Vu0ITOF12Vector(vec4 v0, ivec4 v1) {
    v0[0] = v1[0] / 4096.0f;
    v0[1] = v1[1] / 4096.0f;
    v0[2] = v1[2] / 4096.0f;
    v0[3] = v1[3] / 4096.0f;
}

// View/camera application functions

void Vu0LightColorMatrix(mat4 m, vec4 c0, vec4 c1, vec4 c2, vec4 a) {
    glm_vec4_copy(c0, m[0]);
    glm_vec4_copy(c1, m[1]);
    glm_vec4_copy(c2, m[2]);
    glm_vec4_copy(a,  m[3]);
}

void Vu0NormalLightMatrix(mat4 m, vec4 l0, vec4 l1, vec4 l2) {
    glm_vec4_scale(l0, -1.0f, m[0]);
    glm_vec4_scale(l1, -1.0f, m[1]);
    glm_vec4_scale(l2, -1.0f, m[2]);
    glm_vec4_normalize(m[0]);
    glm_vec4_normalize(m[1]);
    glm_vec4_normalize(m[2]);
    glm_vec4_zero(m[3]); m[3][3] = 1.0f;
    glm_mat4_transpose(m);
}
