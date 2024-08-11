#include "camera.h"
#include "object.h"
#include "vu0.h"

void LightInit() {
    Light3.light0[0] = 0.5f;
    Light3.light0[1] = -0.5f;
    Light3.light0[2] = 0.0f;
    Light3.light0[3] = 0.0f;

    Light3.light1[0] = -0.5f;
    Light3.light1[1] = 0.5f;
    Light3.light1[2] = 0.0f;
    Light3.light1[3] = 0.0f;

    Light3.light2[0] = 0.0f;
    Light3.light2[1] = 0.0f;
    Light3.light2[2] = 0.0f;
    Light3.light2[3] = 0.0f;

    Light3.color0[0] = 0.5f;
    Light3.color0[1] = 0.5f;
    Light3.color0[2] = 0.5f;
    Light3.color0[3] = 0.0f;

    Light3.color1[0] = 0.15f;
    Light3.color1[1] = 0.15f;
    Light3.color1[2] = 0.15f;
    Light3.color1[3] = 0.0f;

    Light3.color2[0] = 0.0f;
    Light3.color2[1] = 0.0f;
    Light3.color2[2] = 0.0f;
    Light3.color2[3] = 0.0f;
    
    Light3.ambient[0] = 0.2f;
    Light3.ambient[1] = 0.2f;
    Light3.ambient[2] = 0.2f;
    Light3.ambient[3] = 0.0f;

    Vu0NormalLightMatrix(Light3.NormalLight, Light3.light0, Light3.light1, Light3.light2);
    Vu0LightColorMatrix(Light3.LightColor, Light3.color0, Light3.color1, Light3.color2, Light3.ambient);
}