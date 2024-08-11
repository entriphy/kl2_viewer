#pragma once

#include <cglm/cglm.h>

extern void Vu0UnitMatrix(mat4 m0);
extern void Vu0CopyVector(vec4 v0, vec4 v1);
extern void Vu0CopyVectorXYZ(vec4 v0, vec4 v1);
extern void Vu0CopyMatrix(mat4 m0, mat4 m1);
extern void Vu0TransposeMatrix(mat4 m0, mat4 m1);

extern void Vu0AddVector(vec4 v0, vec4 v1, vec4 v2);
extern void Vu0SubVector(vec4 v0, vec4 v1, vec4 v2);
extern void Vu0DivVector(vec4 v0, vec4 v1, float q);
extern void Vu0DivVectorXYZ(vec4 v0, vec4 v1, float q);
extern void Vu0MulVector(vec4 v0, vec4 v1, vec4 v2);
extern float Vu0InnerProduct(vec4 v0, vec4 v1);
extern void Vu0OuterProduct(vec4 v0, vec4 v1, vec4 v2);
extern void Vu0ScaleVector(vec4 v0, vec4 v1, float t);
extern void Vu0ScaleVectorXYZ(vec4 v0, vec4 v1, float t);
extern void Vu0Normalize(vec4 v0, vec4 v1);
extern void Vu0InterVector(vec4 v0, vec4 v1, vec4 v2, float t);
extern void Vu0InterVectorXYZ(vec4 v0, vec4 v1, vec4 v2, float t);
extern void Vu0ClampVector(vec4 v0, vec4 v1, float min, float max);

extern void Vu0ApplyMatrix(vec4 v0, mat4 m0, vec4 v1);
extern void Vu0MulMatrix(mat4 m0, mat4 m1, mat4 m2);
extern void Vu0RotMatrix(mat4 m0, mat4 m1, vec4 rot);
extern void Vu0RotMatrixX(mat4 m0, mat4 m1, float rx);
extern void Vu0RotMatrixY(mat4 m0, mat4 m1, float ry);
extern void Vu0RotMatrixZ(mat4 m0, mat4 m1, float rz);
extern void Vu0TransMatrix(mat4 m0, mat4 m1, vec4 tv);
extern void Vu0InversMatrix(mat4 m0, mat4 m1);

extern void Vu0FTOI0Vector(ivec4 v0, vec4 v1);
extern void Vu0FTOI4Vector(ivec4 v0, vec4 v1);
extern void Vu0ITOF0Vector(vec4 v0, ivec4 v1);
extern void Vu0ITOF4Vector(vec4 v0, ivec4 v1);
extern void Vu0ITOF12Vector(vec4 v0, ivec4 v1);

extern void Vu0LightColorMatrix(mat4 m, vec4 c0, vec4 c1, vec4 c2, vec4 a);
extern void Vu0NormalLightMatrix(mat4 m, vec4 l0, vec4 l1, vec4 l2);