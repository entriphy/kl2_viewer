#pragma once
#include "sfx.h"

typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int framerate;
    float trans_y;
    float trans_z;
    float rot_x;
    float rot_y;
    char *chr_name;
    char *act_name;
} SceneSettings;

extern int backend_init();
extern void backend_sfxInit(SFXOBJ *pObj);
extern void backend_sfxClear(SFXOBJ *pObj);
extern void backend_bgDraw();
extern void backend_sfxDraw(SFXOBJ *pObj, int lodIndex, int mimeIndex, float mimeWeight);
extern void backend_endDraw();
extern int backend_windowInit(int width, int height);
extern int backend_igInit();
extern void backend_windowLoop(void (*renderFunc)(), void (*igFunc)());
extern void backend_igClear();
extern void backend_windowClear();
extern void backend_renderInit(int width, int height);
extern void backend_renderClear();
extern void backend_renderFrame(void (*renderFunc)(), int width, int height, void *pixels);