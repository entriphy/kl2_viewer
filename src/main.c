#include "take/object.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <EGL/egl.h>
#include <cglm/cglm.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

#include "take.h"
#include "vu0.h"
#include "romfs.h"
#include "backend.h"

#define SCR_WIDTH  1280
#define SCR_HEIGHT 960
#define FRAMERATE  50

// Status stuff
SFXOBJ *pObj = NULL;
char mimeNames[4][128][32] = {};
int guiLodIndex = 0;
int guiMimeIndex = 0;
float guiMimeWeight = 1.0f;
bool guiPause = false;
bool guiAdjustFramerate = true;
bool guiInterpolate = true;
int guiChrIdx = 0;

char **chrList = NULL;
size_t chrListLength = 0;
const char *lodNames[] = { "Large", "Medium", "Small", "Inflated" };

// Model: pObj->pMot->pBaseCoord->Mtx
// View: Scr.WvMtx (world -> view)
// Projection: Scr.VsMtx (view -> screen)

void writeTga(const char *filename, int width, int height, int channels, u8 *pixels) {
    FILE *out = fopen(filename, "w");
    short header[] = {0, 2, 0, 0, 0, 0, width, height, channels * 8};
    fwrite(header, sizeof(header), 1, out);
    fwrite(pixels, channels * width * height, 1, out);
    fclose(out);
}

static void printMatrix(mat4 m) {
    printf("%.5f %.5f %.5f %.5f\n", m[0][0], m[0][1], m[0][2], m[0][3]);
    printf("%.5f %.5f %.5f %.5f\n", m[1][0], m[1][1], m[1][2], m[1][3]);
    printf("%.5f %.5f %.5f %.5f\n", m[2][0], m[2][1], m[2][2], m[2][3]);
    printf("%.5f %.5f %.5f %.5f\n", m[3][0], m[3][1], m[3][2], m[3][3]);
}

static void renderFunc() {
    static float t = 0.0f;

    backend_bgDraw();

    if (pObj != NULL) {
        short Condition = pObj->Condition;
        MOTION *pMot = NULL;
        MIME *pMime = NULL;
        if (pObj->pMot->CoordNum != 0) {
            int p = 0;
            for (SFXOBJ *pObjTmp = pObj; pObjTmp != NULL; pObjTmp = pObjTmp->pObjSub, p++) {
                if (!(Condition & pObjTmp->Flag)) {
                    continue;
                }

                if (pMot != pObjTmp->pMot) {
                    pMot = pObjTmp->pMot;
                    GetMotion(pObjTmp);
                }

                if (pMime != pObjTmp->pMime) {
                    pMime = pObjTmp->pMime;
                    MimeWork(pObj);
                }

                pObjTmp->ActiveGms = 0;
                if (pObjTmp->GmsTransType == 1) {
                    SetSyncTex(pObjTmp);
                }

                // Fix matrices so we don't have to convert them to mat3/mat4x3
                for (int i = 0; i < pObjTmp->pMot->CoordNum; i++) {
                    glm_vec4_zero(SfxLcLightMtx[i][2]);
                    glm_vec4_zero(SfxLcLightMtx[i][3]);
                    
                    glm_vec4_zero(SfxLvSpecMtx[i][3]);
                    SfxLvSpecMtx[i][3][3] = 1.0f;
                }

                backend_sfxDraw(pObjTmp, p, guiMimeIndex, guiMimeWeight);
            }
        }
    }

    backend_endDraw();
    if (!guiPause) {
        t++;
    }
}


static void initScene(SceneSettings *settings) {
    // Get chr list
    size_t length;
    char **list = romfs_list(&length);
    chrList = malloc(sizeof(char *) * length);
    for (int i = 0; i < length; i++) {
        if (strncmp(list[i], "chr/", 4) == 0) {
            chrList[chrListLength++] = list[i];
        } else {
            free(list[i]);
        }
    }
    free(list);

    // Init light
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

    // Init camera
    glm_mat4_identity(Scr.WvMtx);
    glm_translate_y(Scr.WvMtx, settings->trans_y);
    glm_translate_z(Scr.WvMtx, settings->trans_z);
    glm_rotate_x(Scr.WvMtx, glm_rad(settings->rot_x), Scr.WvMtx);
    glm_rotate_y(Scr.WvMtx, glm_rad(settings->rot_y), Scr.WvMtx);
    glm_perspective(glm_rad(45.0f), (float)settings->width / (float)settings->height, 0.1f, 10000000.0f, Scr.VsMtx);
}

static const char* animationNameGetter(void *data, int idx) {
    if (idx == 0) {
        return "None";
    }
    SFXOBJ *pObjAct = data;
    ACT_HEADER *act0 = Fadr(pObjAct->pMot->ActAdrs, idx);
    ACT_HEADER *act1 = Fadr(pObjAct->pMot->ActAdrs, idx + 1);
    if (act0 != act1) {
        return (char *)act0->ActName;
    } else {
        return "[null]";
    }
}

static const char* mimeNameGetter(void *data, int idx) {
    if (idx == 0) {
        return "None";
    } else {
        return mimeNames[guiLodIndex][idx - 1][0] != 0 ? mimeNames[guiLodIndex][idx - 1] : "[null]";
    }
}

static const char* modelNameGetter(void *data, int idx) {
    return idx == 0 ? "None" : chrList[idx - 1];
}

static int sfxInit(const char *filename) {
    // Init buffers
    void *data = romfs_read(filename);
    if (data == NULL) {
        return 1;
    }
    pObj = SetSfxObject(data);
    backend_sfxInit(pObj);

    // Create mime names
    int p = 0;
    for (SFXOBJ *pObjTmp = pObj; pObjTmp != NULL; pObjTmp = pObjTmp->pObjSub, p++) {
        if (pObjTmp->MimeAdrs != NULL) {
            for (int i = 0; i < *pObjTmp->MimeAdrs; i++) {
                TYPE_SFZ_HEADER *sfz = Fadr((u32 *)pObjTmp->MimeAdrs, i);
                TYPE_SFZ_TBL *sfzPart = (TYPE_SFZ_TBL *)(sfz + 1);
                if (*(int *)sfz != 0) {
                    sprintf(mimeNames[p][i], "%d (part %d)", i, sfzPart->parts_id);
                }
            }
        }
    }

    // Set model matrix
    vec4 rot = { 0.0f, 0.0f, 0.0f, 0.0f };
    vec4 trans = { 0.0f, 0.0f, 0.0f, 1.0f };
    SetBaseMatrix(pObj, rot, trans, 0);

    // Set animation
    SetAct(pObj, 1);

    return 0;
}

static void sfxAct(const char *actName) {
    SFXOBJ *pObjAct = GetActiveSfx(pObj);
    for (int i = 1; i < pObjAct->pMot->ActNumMax; i++) {
        if (Fadr(pObjAct->pMot->ActAdrs, i) == Fadr(pObjAct->pMot->ActAdrs, i + 1)) {
            continue;
        }

        ACT_HEADER *act = Fadr(pObjAct->pMot->ActAdrs, i);
        if (strcmp((char *)act->ActName, actName) == 0) {
            SetAct(pObj, i);
            break;
        }
    }
}

static void sfxClear() {
    if (pObj == NULL) {
        return;
    }
    backend_sfxClear(pObj);

    memset(mimeNames, 0, sizeof(mimeNames));
    free(pObj);
    pObj = NULL;
    guiMimeIndex = 0;
    guiLodIndex = 0;
}

static void drawGui() {
    ImVec2 buttonSize = { 0.0f, 0.0f };

    igText("FPS: %.0f", igGetIO()->Framerate);

    if (igListBox_FnStrPtr("Models", &guiChrIdx, modelNameGetter, NULL, chrListLength + 1, 8)) {
        sfxClear();
        if (guiChrIdx != 0) {
            sfxInit(chrList[guiChrIdx - 1]);
        }
    }

    if (pObj != NULL) {
        if (igListBox_Str_arr("Size\nthis is what the\ndevs called the LODs\ndon't @ me", &guiLodIndex, lodNames, 4, 4)) {
            SetObjCondition(pObj, 1 << guiLodIndex);
        }
    }

    if (igCollapsingHeader_TreeNodeFlags("Lighting", 0)) {
        if (igButton("Reset", buttonSize)) {
            LightInit();
        }
        igText("Angle");
        int directionFlag = igSliderFloat3("Light0", Light3.light0, -1.5f, 1.5f, NULL, ImGuiSliderFlags_AlwaysClamp) |
            igSliderFloat3("Light1", Light3.light1, -1.5f, 1.5f, NULL, ImGuiSliderFlags_AlwaysClamp) |
            igSliderFloat3("Light2", Light3.light2, -1.5f, 1.5f, NULL, ImGuiSliderFlags_AlwaysClamp);
        if (directionFlag){
            Vu0NormalLightMatrix(Light3.NormalLight, Light3.light0, Light3.light1, Light3.light2);
        }

        igText("Color");
        int colorFlag = igColorEdit3("Ambient", Light3.ambient, 0) |
            igColorEdit3("Light0", Light3.color0, 0) |
            igColorEdit3("Light1", Light3.color1, 0) |
            igColorEdit3("Light2", Light3.color2, 0);
        if (colorFlag) {
            Vu0LightColorMatrix(Light3.LightColor, Light3.color0, Light3.color1, Light3.color2, Light3.ambient);
        }
    }

    if (pObj == NULL) {
        return;
    }

    SFXOBJ *pObjAct = GetActiveSfx(pObj);
    if (pObjAct == NULL) {
        return;
    }

    if (igCollapsingHeader_TreeNodeFlags("Animation", 0)) {
        int baseIndex = pObjAct->pMot->BaseIndex;

        bool loop = pObjAct->pMot->Mb[baseIndex].StopFlag == 0;
        if (igCheckbox("Loop", &loop)) {
            pObjAct->pMot->Mb[baseIndex].StopFlag = loop ? 0 : 1;
        }
        igCheckbox("Interpolate", &guiInterpolate);
        igCheckbox("Adjust speed to refresh rate", &guiAdjustFramerate);
        igSliderFloat("Frame", &pObjAct->pMot->Mb[baseIndex].MotionCnt, 0.0f, pObjAct->pMot->Mb[baseIndex].MotionEndCnt - 1.0f, NULL, ImGuiSliderFlags_AlwaysClamp);
        igSliderFloat("Speed", &pObjAct->pMot->Mb[baseIndex].CntSpeed, 0.0f, 4.0f, NULL, ImGuiSliderFlags_AlwaysClamp);
        
        s32 act = pObjAct->pMot->ActNum;
        if (igListBox_FnStrPtr("Animations", &act, animationNameGetter, pObjAct, pObjAct->pMot->ActNumMax, 8)) {
            if (guiInterpolate) {
                SetActIp(pObj, act);
            } else {
                SetAct(pObj, act);
            }

            if (guiAdjustFramerate) {
                pObjAct->pMot->Mb[pObjAct->pMot->BaseIndex].CntSpeed = 60.0f / igGetIO()->Framerate;
            }
        }
    }

    if (pObjAct->MimeAdrs != NULL && igCollapsingHeader_TreeNodeFlags("Mime", 0)) {
        igSliderFloat("Weight", &guiMimeWeight, 0.0f, 1.0f, NULL, 0);
        igListBox_FnStrPtr("Mimes", &guiMimeIndex, mimeNameGetter, NULL, *pObjAct->MimeAdrs, 8);
    }
}

int windowMode(SceneSettings *settings) {
    backend_windowInit(settings->width, settings->height);
    backend_init();
    igCreateContext(NULL);
    igStyleColorsDark(NULL);
    backend_igInit();
    initScene(settings);

    // Custom lighting
    SETVEC(Light3.light2, -0.5f, -0.5f, 0.0f, 0.0f);
    SETVEC(Light3.color2, 0.15f, 0.15f, 0.15f, 0);
    Vu0NormalLightMatrix(Light3.NormalLight, Light3.light0, Light3.light1, Light3.light2);
    Vu0LightColorMatrix(Light3.LightColor, Light3.color0, Light3.color1, Light3.color2, Light3.ambient);

    // Loop
    backend_windowLoop(renderFunc, drawGui);

    // Cleanup
    backend_igClear();
    igDestroyContext(NULL);
    backend_windowClear();

    return 0;
}

int renderMode(SceneSettings *settings) {
    backend_renderInit(settings->width, settings->height);
    backend_init();
    initScene(settings);

    sfxInit(settings->chr_name);

    SFXOBJ *pObjAct = GetActiveSfx(pObj);
    sfxAct(settings->act_name);
    pObjAct->pMot->Mb[pObjAct->pMot->BaseIndex].CntSpeed = 60.0f / settings->framerate;
    pObjAct->pMot->Mb[pObjAct->pMot->BaseIndex].StopFlag = 1;

    u8 *img = malloc(settings->width * settings->height * 3);
    for (int i = 0; pObjAct->pMot->Mb[pObjAct->pMot->BaseIndex].MotionCnt < pObjAct->pMot->Mb[pObjAct->pMot->BaseIndex].MotionEndCnt; i++) {
        backend_renderFrame(renderFunc, settings->width, settings->height, img);

        char name[32];
        sprintf(name, "fb/fb%04d.tga", i);
        writeTga(name, settings->width, settings->height, 3, img);
        printf("%d\n", i);
    }

    free(img);
    backend_renderClear();
    return 0;
}

int main(int argc, char *argv[]) {
    SceneSettings settings = {
        .width = SCR_WIDTH,
        .height = SCR_HEIGHT,
        .framerate = FRAMERATE,
        .trans_y = 0.0f,
        .trans_z = -200.0f,
        .rot_x = 0.0f,
        .rot_y = 0.0f,
        .chr_name = NULL,
        .act_name = NULL
    };

    if (argc == 1) {
        return windowMode(&settings);
    }

    if (argc < 3) {
        printf("Usage: kl2_renderer filename anim_name [-w width] [-h height] [-fr framerate] [-ty trans_y] [-tz trans_z] [-ry rot_y] [-rz rot_z]\n");
    }

    for (int i = 3; i < argc; i++) {
        char *arg = argv[i];
        if (*arg != '-') {
            printf("Invalid argument '%s'\n", arg);
            return 1;
        }
        arg++;

        if (strcmp(arg, "w") == 0) {
            settings.width = strtof(argv[++i], NULL);
        } else if (strcmp(arg, "h") == 0) {
            settings.height = strtof(argv[++i], NULL);
        } else if (strcmp(arg, "fr") == 0) {
            settings.framerate = strtol(argv[++i], NULL, 0);
        } else if (strcmp(arg, "ty") == 0) {
            settings.trans_y = strtof(argv[++i], NULL);
        } else if (strcmp(arg, "tz") == 0) {
            settings.trans_z = strtof(argv[++i], NULL);
        } else if (strcmp(arg, "rx") == 0) {
            settings.rot_x = strtof(argv[++i], NULL);
        } else if (strcmp(arg, "ry") == 0) {
            settings.rot_y = strtof(argv[++i], NULL);
        } else {
            printf("Invalid argument '%s'\n", arg);
            return 1;
        }
    }

    settings.chr_name = argv[1];
    settings.act_name = argv[2];

    return renderMode(&settings);
}
