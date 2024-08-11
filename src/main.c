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
#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include <cimgui.h>
#include <cimgui_impl.h>

#include "take.h"
#include "gms.h"
#include "gs.h"
#include "vu0.h"
#include "romfs.h"

#define SCR_WIDTH  1280
#define SCR_HEIGHT 960
#define FB_WIDTH   960
#define FB_HEIGHT  960
#define FRAMERATE  50

// OpenGL buffers
GLuint bgVAO = 0;
GLuint bgVBO = 0;
GLuint sfxVAO[4][64] = {}; // [lod][part]
GLuint sfxVBO[4][64] = {}; // [lod][part]

// Shaders
GLuint bgShader = 0;
GLuint sfxShader = 0;
GLuint sfxSpecShader = 0;

// Textures
GLuint sfxTextures[4][8][16] = {}; // [lod][channel][texture_id]
GLuint sfxSpecTexture = 0;

// ImGui stuff
ImGuiIO *io = NULL;

// Status stuff
SFXOBJ *pObj = NULL;
struct {
    int part;
    char name[32];
    void *vertex;
    void *normal;
} mimePtrs[4][128] = {}; // [lod][mime]
int lod = 0;
int faceMime = 0;
float mimeWeight = 1.0f;
const char *lodNames[] = { "Large", "Medium", "Small", "Inflated" };

char **chrList = NULL;
size_t chrListLength = 0;
int chrIdx = 0;

bool leftClick = false;
bool rightClick = false;
bool pause = false;
bool adjustFramerate = true;
bool interpolate = true;

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

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render background
    glBindVertexArray(bgVAO);
    glUseProgram(bgShader);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    if (pObj == NULL) {
        goto end;
    }

    glUseProgram(sfxShader);
    glUniformMatrix4fv(glGetUniformLocation(sfxShader, "Model"), 1, GL_FALSE, *pObj->pMot->pBaseCoord->Mtx);
    glUniformMatrix4fv(glGetUniformLocation(sfxShader, "View"), 1, GL_FALSE, *Scr.WvMtx);
    glUniformMatrix4fv(glGetUniformLocation(sfxShader, "Projection"), 1, GL_FALSE, *Scr.VsMtx);
    glUniformMatrix4fv(glGetUniformLocation(sfxShader, "LightColor"), 1, GL_FALSE, **pObj->pLightColor);
    glUniformMatrix4fv(glGetUniformLocation(sfxShader, "NormalLight"), 1, GL_FALSE, **pObj->pNormalLight);

    glUseProgram(sfxSpecShader);
    glUniformMatrix4fv(glGetUniformLocation(sfxSpecShader, "Model"), 1, GL_FALSE, *pObj->pMot->pBaseCoord->Mtx);
    glUniformMatrix4fv(glGetUniformLocation(sfxSpecShader, "View"), 1, GL_FALSE, *Scr.WvMtx);
    glUniformMatrix4fv(glGetUniformLocation(sfxSpecShader, "Projection"), 1, GL_FALSE, *Scr.VsMtx);
    glUniform4fv(glGetUniformLocation(sfxSpecShader, "Ambient"), 1, (*pObj->pLightColor)[3]);

    // Render SFXOBJ
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

            glUseProgram(sfxShader);
            glUniformMatrix4fv(glGetUniformLocation(sfxShader, "Bones"), pObjTmp->pMot->CoordNum, GL_FALSE, **SfxSkinMtx);
            glUniformMatrix4fv(glGetUniformLocation(sfxShader, "NormalLights"), pObjTmp->pMot->CoordNum, GL_FALSE, **SfxLcLightMtx);
            glUseProgram(sfxSpecShader);
            glUniformMatrix4fv(glGetUniformLocation(sfxSpecShader, "Bones"), pObjTmp->pMot->CoordNum, GL_FALSE, **SfxSkinMtx);
            glUniformMatrix4fv(glGetUniformLocation(sfxSpecShader, "Spec"), pObjTmp->pMot->CoordNum, GL_FALSE, **SfxLvSpecMtx);

            for (int i = pObjTmp->PartsNum - 1; i >= 0; i--) { // The game renders the parts in reverse order for some reason (possibly related to the outline effect?)
                PARTS *part = &pObjTmp->pParts[i];

                glUseProgram(sfxShader);
                glBindVertexArray(sfxVAO[p][i]);
                glBindBuffer(GL_ARRAY_BUFFER, sfxVBO[p][i]);
                glBindTexture(GL_TEXTURE_2D, sfxTextures[p][pObjTmp->ActiveGms][part->TextureId]);

                switch (part->type) {
                    case 0: // Fixed
                        glUniform1f(glGetUniformLocation(sfxShader, "MimeWeight"), 0.0f);
                        break;
                    case 1: // Skin
                        glUniform1f(glGetUniformLocation(sfxShader, "MimeWeight"), 0.0f);
                        break;
                    case 3: // Mime
                        if (faceMime != 0) {
                            break;
                        }
                        glUniform1f(glGetUniformLocation(sfxShader, "MimeWeight"), part->MimeWeight);
                        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[p][part->MimeStart].vertex);
                        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[p][part->MimeEnd].vertex);
                        glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[p][part->MimeStart].normal);
                        glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[p][part->MimeEnd].normal);
                        break;
                }

                if (faceMime != 0 && i == mimePtrs[p][faceMime - 1].part) {
                    void *vertexPtr;
                    void *normalPtr;
                    glGetVertexAttribPointerv(0, GL_VERTEX_ATTRIB_ARRAY_POINTER, &vertexPtr);
                    glGetVertexAttribPointerv(1, GL_VERTEX_ATTRIB_ARRAY_POINTER, &normalPtr);
                    glUniform1f(glGetUniformLocation(sfxShader, "MimeWeight"), mimeWeight);
                    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), vertexPtr);
                    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[p][faceMime - 1].vertex);
                    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), normalPtr);
                    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[p][faceMime - 1].normal);
                }

                TYPE_STRIP *strip = (TYPE_STRIP *)part->prim_adrs;
                GLint start = 0;
                for (int tri = 0; tri < part->prim_num; tri++) {
                    glDrawArrays(GL_TRIANGLE_STRIP, start, strip->num);
                    start += strip->num;
                    strip += strip->num;
                }
                // glAlphaFunc(GL_ALPHA_TEST)

                // TODO: Implement specular
                // if (part->SpecType != 0) {
                //     glUseProgram(sfxSpecShader);
                //     glBindTexture(GL_TEXTURE_2D, sfxSpecTexture);

                //     switch (part->type) {
                //         case 0: // Fixed
                //             glUniform1f(glGetUniformLocation(sfxSpecShader, "MimeWeight"), 0.0f);
                //             break;
                //         case 1: // Skin
                //             glUniform1f(glGetUniformLocation(sfxSpecShader, "MimeWeight"), 0.0f);
                //             break;
                //         case 3: // Mime
                //             glUniform1f(glGetUniformLocation(sfxSpecShader, "MimeWeight"), part->MimeWeight);
                //             glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[part->MimeStart].vertex);
                //             glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[part->MimeEnd].vertex);
                //             glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[part->MimeStart].normal);
                //             glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[part->MimeEnd].normal);
                //             break;
                //     }

                //     TYPE_STRIP *strip = (TYPE_STRIP *)part->prim_adrs;
                //     GLint start = 0;
                //     for (int tri = 0; tri < part->prim_num; tri++) {
                //         glDrawArrays(GL_TRIANGLE_STRIP, start, strip->num);
                //         start += strip->num;
                //         strip += strip->num;
                //     }
                    
                //     glUseProgram(sfxShader);
                //     glBindTexture(GL_TEXTURE_2D, 0);
                // }
            }
        }
    }
    
end:
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    if (!pause) {
        t++;
    }
}

static void createBgVertexBuffer() {
    vec4 vertices[8] = {
        { -1.0f, 1.0f, 1.0f, 1.0f },
        { -1.0f, -1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, -1.0f, 1.0f, 1.0f },

        { 160.0f / 255.0f, 80.0f / 255.0f, 80.0f / 255.0f, 1.0f },
        { 160.0f / 255.0f, 160.0f / 255.0f, 128.0f / 255.0f, 1.0f },
        { 128.0f / 255.0f, 160.0f / 255.0f, 100.0f / 255.0f, 1.0f },
        { 64.0f / 255.0f, 128.0f / 255.0f, 160.0f / 255.0f, 1.0f },
    };

    glGenVertexArrays(1, &bgVAO);
    glGenBuffers(1, &bgVBO);

    glBindVertexArray(bgVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bgVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), (void *)0x00);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), (void *)0x40);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void createSfxVertexBuffer() {
    int p = 0;
    for (SFXOBJ *pObjTmp = pObj; pObjTmp != NULL; pObjTmp = pObjTmp->pObjSub, p++) {
        glGenVertexArrays(pObjTmp->PartsNum, sfxVAO[p]);
        glGenBuffers(pObjTmp->PartsNum, sfxVBO[p]);
        int mimeNum = pObjTmp->MimeAdrs == NULL ? 0 : *(u32 *)pObjTmp->MimeAdrs;
        
        for (int i = 0; i < pObjTmp->PartsNum; i++) {
            PARTS *part = &pObjTmp->pParts[i];
            int mimes[64]; // Array of mimes that apply to this part
            int mimeIndex = 0;

            // Init buffer
            glBindVertexArray(sfxVAO[p][i]);
            glBindBuffer(GL_ARRAY_BUFFER, sfxVBO[p][i]);
            
            // Create buffer
            vec4 *verticesArray = malloc(sizeof(vec4) * part->vert_num);
            vec4 *normalsArray = malloc(sizeof(vec4) * part->norm_num);
            vec4 *texcoordsArray = malloc(sizeof(vec4) * part->uv_num);
            vec4 *vertexWeightsArray = malloc(sizeof(vec4) * part->vert_num);
            ivec4 *vertexJointsArray = malloc(sizeof(ivec4) * part->vert_num);
            vec4 *mimeVerticesArray = mimeNum != 0 ? malloc(sizeof(vec4) * part->vert_num * mimeNum) : NULL;
            vec4 *mimeNormalsArray = mimeNum != 0 ? malloc(sizeof(vec4) * part->norm_num * mimeNum) : NULL;
            
            vec4 *verticesPtr = verticesArray;
            vec4 *normalsPtr = normalsArray;
            vec4 *texcoordsPtr = texcoordsArray;
            vec4 *vertexWeightsPtr = vertexWeightsArray;
            ivec4 *vertexJointsPtr = vertexJointsArray;

            if (part->type == 0) {
                short *vertPtr = (short *)part->vert_adrs;
                for (int j = 0; j < part->vert_num; j++) {
                    (*verticesPtr)[0] = (float)vertPtr[0];
                    (*verticesPtr)[1] = -(float)vertPtr[1];
                    (*verticesPtr)[2] = -(float)vertPtr[2];
                    (*verticesPtr)[3] = 0.0f;
                    glm_vec4_scale(*verticesPtr, pObjTmp->scale, *verticesPtr);
                    (*verticesPtr)[3] = 1.0f;

                    verticesPtr++;
                    vertPtr += 3;
                }

                short *normPtr = (short *)part->norm_adrs;
                for (int j = 0; j < part->norm_num; j++) {
                    (*normalsPtr)[0] = (float)normPtr[0];
                    (*normalsPtr)[1] = -(float)normPtr[1];
                    (*normalsPtr)[2] = -(float)normPtr[2];
                    (*normalsPtr)[3] = 0.0f;
                    glm_vec4_divs(*normalsPtr, 4096.0f, *normalsPtr);
                    (*normalsPtr)[3] = 0.0f;

                    normalsPtr++;
                    normPtr += 3;
                }
            } else {
                for (int j = 0; j < part->jblock_num; j++) {
                    TYPE_JOINT_BLK *jblock = (TYPE_JOINT_BLK *)part->jblock_adrs + j;

                    short *vertPtr = (short *)((uintptr_t)part->vert_adrs + jblock->vert_ofs);
                    for (int k = 0; k < jblock->vert_n; k++) {
                        (*verticesPtr)[0] = (float)vertPtr[0];
                        (*verticesPtr)[1] = -(float)vertPtr[1];
                        (*verticesPtr)[2] = -(float)vertPtr[2];
                        (*verticesPtr)[3] = 0.0f;
                        glm_vec4_scale(*verticesPtr, pObjTmp->scale, *verticesPtr);
                        (*verticesPtr)[3] = 1.0f;
                        verticesPtr++;
                        vertPtr += 3;
                    }

                    unsigned char *vertWtPtr = (unsigned char *)((uintptr_t)part->sfx_adrs + jblock->v_wt_adrs);
                    for (int k = 0; k < jblock->vert_n; k++) {
                        (*vertexJointsPtr)[0] = jblock->joint_id[0] == -1 ? 0 : jblock->joint_id[0];
                        (*vertexJointsPtr)[1] = jblock->joint_id[1] == -1 ? 0 : jblock->joint_id[1];
                        (*vertexJointsPtr)[2] = jblock->joint_id[2] == -1 ? 0 : jblock->joint_id[2];
                        (*vertexJointsPtr)[3] = jblock->joint_id[3] == -1 ? 0 : jblock->joint_id[3];
                        vertexJointsPtr++;

                        (*vertexWeightsPtr)[0] = (float)vertWtPtr[0];
                        (*vertexWeightsPtr)[1] = (float)vertWtPtr[1];
                        (*vertexWeightsPtr)[2] = (float)vertWtPtr[2];
                        (*vertexWeightsPtr)[3] = (float)vertWtPtr[3];
                        vertexWeightsPtr++;
                        vertWtPtr += 4;
                    }

                    short *normPtr = (short *)((uintptr_t)part->norm_adrs + jblock->norm_ofs);
                    for (int k = 0; k < jblock->norm_n; k++) {
                        (*normalsPtr)[0] = (float)normPtr[0];
                        (*normalsPtr)[1] = -(float)normPtr[1];
                        (*normalsPtr)[2] = -(float)normPtr[2];
                        (*normalsPtr)[3] = 0.0f;
                        glm_vec4_divs(*normalsPtr, 4096.0f, *normalsPtr);
                        (*normalsPtr)[3] = 1.0f;

                        normalsPtr++;
                        normPtr += 3;
                    }
                }

                for (int k = 0; k < mimeNum; k++) {
                    TYPE_SFZ_HEADER *sfz = Fadr((u32 *)pObjTmp->MimeAdrs, k);
                    TYPE_SFZ_TBL *sfzPart = (TYPE_SFZ_TBL *)(sfz + 1);
                    if (*(int *)sfz == 0 || sfzPart->parts_id != i) {
                        continue;
                    }
                    mimes[mimeIndex++] = k;

                    vec4 *mimeVerticesPtr = mimeVerticesArray + k * sfzPart->vert_num;
                    vec4 *mimeNormalsPtr = mimeNormalsArray + k * sfzPart->norm_num;
                    for (int j = 0; j < part->jblock_num; j++) {
                        TYPE_JOINT_BLK *jblock = (TYPE_JOINT_BLK *)part->jblock_adrs + j;

                        short *vertPtr = (short *)((uintptr_t)sfz + sfzPart->vert_adrs + jblock->vert_ofs);
                        for (int v = 0; v < jblock->vert_n; v++) {
                            (*mimeVerticesPtr)[0] = (float)vertPtr[0];
                            (*mimeVerticesPtr)[1] = -(float)vertPtr[1];
                            (*mimeVerticesPtr)[2] = -(float)vertPtr[2];
                            (*mimeVerticesPtr)[3] = 0.0f;
                            glm_vec4_scale(*mimeVerticesPtr, sfz->scale, *mimeVerticesPtr);
                            (*mimeVerticesPtr)[3] = 1.0f;
                            mimeVerticesPtr++;
                            vertPtr += 3;
                        }

                        short *normPtr = (short *)((uintptr_t)sfz + sfzPart->norm_adrs + jblock->norm_ofs);
                        for (int n = 0; n < jblock->norm_n; n++) {
                            (*mimeNormalsPtr)[0] = (float)normPtr[0];
                            (*mimeNormalsPtr)[1] = -(float)normPtr[1];
                            (*mimeNormalsPtr)[2] = -(float)normPtr[2];
                            (*mimeNormalsPtr)[3] = 0.0f;
                            glm_vec4_divs(*mimeNormalsPtr, 4096.0f, *mimeNormalsPtr);
                            (*mimeNormalsPtr)[3] = 1.0f;
                            mimeNormalsPtr++;
                            normPtr += 3;
                        }
                    }
                }
            }

            float scaleU = (1 << part->gs_tex0.TW) << 4;
            float scaleV = (1 << part->gs_tex0.TH) << 4;
            u32 *uvPtr = (u32 *)part->uv_adrs;
            for (int j = 0; j < part->uv_num; j++) {
                (*texcoordsPtr)[0] = uvPtr[0] / scaleU;
                (*texcoordsPtr)[1] = uvPtr[1] / scaleV;
                (*texcoordsPtr)[2] = 0.0f;
                (*texcoordsPtr)[3] = 0.0f;
                texcoordsPtr++;
                uvPtr += 2;
            }

            GLuint vertexCount = 0;
            TYPE_STRIP *strip = (TYPE_STRIP *)part->prim_adrs;
            for (int tri = 0; tri < part->prim_num; tri++) {
                vertexCount += strip->num;
                strip += strip->num;
            }

            size_t bufferSize = (sizeof(vec4) * 4 + sizeof(ivec4) + sizeof(vec4) * mimeIndex * 2) * vertexCount;
            vec4 *buffer = malloc(bufferSize);

            vec4 *vertices = buffer;
            vec4 *normals = vertices + vertexCount;
            vec4 *texcoords = normals + vertexCount;
            vec4 *weights = texcoords + vertexCount;
            ivec4 *joints = (ivec4 *)(weights + vertexCount);
            vec4 *mimeData = (vec4 *)(joints + vertexCount);

            void *glVertexBuffer = (void *)((uintptr_t)vertices - (uintptr_t)buffer);
            void *glNormalBuffer = (void *)((uintptr_t)normals - (uintptr_t)buffer);
            void *glTexcoordBuffer = (void *)((uintptr_t)texcoords - (uintptr_t)buffer);
            void *glWeightsBuffer = (void *)((uintptr_t)weights - (uintptr_t)buffer);
            void *glJointsBuffer = (void *)((uintptr_t)joints - (uintptr_t)buffer);

            strip = (TYPE_STRIP *)part->prim_adrs;
            for (int tri = 0; tri < part->prim_num; tri++) {
                for (int j = 0; j < strip->num; j++) {
                    glm_vec4_copy(verticesArray[strip[j].p0], *vertices++);
                    glm_vec4_copy(normalsArray[strip[j].n0], *normals++);
                    glm_vec4_copy(texcoordsArray[strip[j].t0], *texcoords++);
                    glm_vec4_copy(vertexWeightsArray[strip[j].p0], *weights++);
                    glm_ivec4_copy(vertexJointsArray[strip[j].p0], *joints++);
                }
                strip += strip->num;
            }

            for (int k = 0; k < mimeIndex; k++) {
                int mimeI = mimes[k];

                vec4 *mimeVertices = mimeData;
                vec4 *mimeNormals = mimeVertices + vertexCount;
                mimeData = mimeNormals + vertexCount;

                void *glMimeVerticesBuffer = (void *)((uintptr_t)mimeVertices - (uintptr_t)buffer);
                void *glMimeNormalsBuffer = (void *)((uintptr_t)mimeNormals - (uintptr_t)buffer);
                mimePtrs[p][mimeI].part = i;
                mimePtrs[p][mimeI].vertex = glMimeVerticesBuffer;
                mimePtrs[p][mimeI].normal = glMimeNormalsBuffer;

                strip = (TYPE_STRIP *)part->prim_adrs;
                vec4 *mimeVerticesArrayPtr = mimeVerticesArray + mimeI * part->vert_num;
                vec4 *mimeNormalsArrayPtr = mimeNormalsArray + mimeI * part->norm_num;
                for (int tri = 0; tri < part->prim_num; tri++) {
                    for (int j = 0; j < strip->num; j++) {
                        glm_vec4_copy(mimeVerticesArrayPtr[strip[j].p0], *mimeVertices++);
                        glm_vec4_copy(mimeNormalsArrayPtr[strip[j].n0], *mimeNormals++);
                    }
                    strip += strip->num;
                }
            }

            glBufferData(GL_ARRAY_BUFFER, bufferSize, buffer, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glEnableVertexAttribArray(3);
            glEnableVertexAttribArray(4);
            glEnableVertexAttribArray(5);
            glEnableVertexAttribArray(6);
            glEnableVertexAttribArray(7);
            glEnableVertexAttribArray(8);
            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), glVertexBuffer);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), glNormalBuffer);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), glTexcoordBuffer);
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), glWeightsBuffer);
            glVertexAttribIPointer(4, 4, GL_INT, sizeof(ivec4), glJointsBuffer);
            // Mime pointers are set during the draw loop

            free(verticesArray);
            free(normalsArray);
            free(texcoordsArray);
            free(vertexWeightsArray);
            free(vertexJointsArray);
            free(mimeVerticesArray);
            free(mimeNormalsArray);
            free(buffer);
        }
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static GLuint compileShader(const char *shaderFile, GLenum type) {
    GLint success;
    GLchar infoLog[1024];

    GLuint shader = glCreateShader(type);
    const GLchar *text = romfs_read(shaderFile);
    glShaderSource(shader, 1, &text, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
        fprintf(stderr, "Error compiling %s: %s\n", shaderFile, infoLog);
        exit(1);
    }

    return shader;
}

static GLuint compileShaderProgram(const char *vertexShaderFile, const char *fragmentShaderFile) {
    GLint success;
    GLchar infoLog[1024];

    GLuint shaderProgram = glCreateProgram();
    if (!shaderProgram) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }

    GLuint vertexShader = compileShader(vertexShaderFile, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentShaderFile, GL_FRAGMENT_SHADER);
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, sizeof(infoLog), NULL, infoLog);
        fprintf(stderr, "Error linking shader program: %s\n", infoLog);
        exit(1);
    }

    glValidateProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_VALIDATE_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, sizeof(infoLog), NULL, infoLog);
        fprintf(stderr, "Invalid shader program: %s\n", infoLog);
        exit(1);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

static void compileShaders() {
    bgShader = compileShaderProgram("bg_shader.vs", "bg_shader.fs");
    sfxShader = compileShaderProgram("sfx_shader.vs", "sfx_shader.fs");
    sfxSpecShader = compileShaderProgram("sfx_spec_shader.vs", "sfx_spec_shader.fs");
}

static void generateTexture(GLuint *texture, void *pixels, GLsizei width, GLsizei height) {
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void generateSfxTextures() {
    int p = 0;
    for (SFXOBJ *pObjTmp = pObj; pObjTmp != NULL; pObjTmp = pObjTmp->pObjSub, p++) {
        for (int t = 0; t < *pObjTmp->GmsAdrs; t++) {
            if (*(u32 *)Fadr(pObjTmp->GmsAdrs, t) == 0) {
                continue;
            }
            upload_gms(Fadr(pObjTmp->GmsAdrs, 0));
            if (t != 0) {
                upload_gms(Fadr(pObjTmp->GmsAdrs, t));
            }

            for (int i = 0; i < pObjTmp->PartsNum; i++) {
                PARTS *part = &pObjTmp->pParts[i];
                if (sfxTextures[p][t][part->TextureId] != 0) {
                    continue;
                }

                sceGsTex0 *tex0 = &part->gs_tex0;
                GLsizei width = 1 << tex0->TW;
                GLsizei height = 1 << tex0->TH;

                unsigned char *img = malloc(width * height * 4);
                switch (tex0->PSM) {
                    case SCE_GS_PSMT4:
                        gs_read_image_PSMT4_PSMCT32(img, tex0->TBP0, tex0->TBW, width, height, tex0->CBP, tex0->CSA, -1);
                        break;
                    case SCE_GS_PSMT8:
                        gs_read_image_PSMT8_PSMCT32(img, tex0->TBP0, tex0->TBW, width, height, tex0->CBP, tex0->CSA, -1);
                        break;
                    default:
                        break;
                }

                generateTexture(&sfxTextures[p][t][part->TextureId], img, width, height);
            }
        }
    }
}

static void generateSpecTexture() {
    char *specGim = romfs_read("spec.gim");
    GIMHEADER *textureHeader = (GIMHEADER *)(specGim + 0x10);
    GIMHEADER *colorHeader = (GIMHEADER *)(specGim + 0x20 + textureHeader->w * textureHeader->h);
    u8 *texture = (u8 *)(textureHeader + 1);
    u8 *color = (u8 *)(colorHeader + 1);

    unsigned char *img = malloc(textureHeader->w * textureHeader->w * 4);
    gs_upload_image_PSMT8(texture, textureHeader->bp, textureHeader->bw, textureHeader->x, textureHeader->y, textureHeader->w, textureHeader->h);
    gs_upload_image_PSMCT32(color, colorHeader->bp, colorHeader->bw, colorHeader->x, colorHeader->y, colorHeader->w, colorHeader->h);
    gs_read_image_PSMT8_PSMCT32(img, textureHeader->bp, textureHeader->bw, textureHeader->w, textureHeader->h, colorHeader->bp, 0, -1);
    generateTexture(&sfxSpecTexture, img, textureHeader->w, textureHeader->h);
    // writeTga("spec.tga", textureHeader->w, textureHeader->h, 4, img);
    free(img);
}

static void initCamera() {
    glm_mat4_identity(Scr.WvMtx);
    glm_translate_y(Scr.WvMtx, 0.0f);
    glm_translate_z(Scr.WvMtx, -200.0f);

    glm_perspective(glm_rad(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000000.0f, Scr.VsMtx);

    glm_mat4_mul(Scr.VsMtx, Scr.WvMtx, Scr.WsMtx);
}

static void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        glm_translated_y(Scr.WvMtx, -1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        glm_translated_y(Scr.WvMtx, 1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        glm_translated_x(Scr.WvMtx, 1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        glm_translated_x(Scr.WvMtx, -1.0f);
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        glm_rotate_x(Scr.WvMtx, 0.01f, Scr.WvMtx);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        glm_rotate_x(Scr.WvMtx, -0.01f, Scr.WvMtx);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        glm_rotate_y(Scr.WvMtx, 0.01f, Scr.WvMtx);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        glm_rotate_y(Scr.WvMtx, -0.01f, Scr.WvMtx);
    }
}

static void scrollCallback(GLFWwindow *window, double xOffset, double yOffset) {
    if (io->WantCaptureMouse) {
        return;
    }

    glm_translated_z(Scr.WvMtx, yOffset * 10);
}

static void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
    leftClick = button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS;
    rightClick = button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS;
}

static void cursorPositioncallback(GLFWwindow* window, double xpos, double ypos) {
    static double prevX = 0.0;
    static double prevY = 0.0;
    const static float rotationSensitivity = 0.01f;
    const static float translationSensitivity = 0.1f;

    if (io->WantCaptureMouse) {
        return;
    }

    if (prevX == 0.0 && prevY == 0.0) {
        prevX = xpos;
        prevY = ypos;
        return;
    }

    if (leftClick) {
        glm_rotate_y(Scr.WvMtx, rotationSensitivity * (xpos - prevX), Scr.WvMtx);
        glm_rotate_x(Scr.WvMtx, rotationSensitivity * (ypos - prevY), Scr.WvMtx);
    } else if (rightClick) {
        glm_translated_x(Scr.WvMtx, translationSensitivity * (xpos - prevX));
        glm_translated_y(Scr.WvMtx, translationSensitivity * (prevY - ypos));
    }

    prevX = xpos;
    prevY = ypos;
}

static void framebufferSizecallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    glm_perspective(glm_rad(45.0f), (float)width / (float)height, 0.1f, 10000000.0f, Scr.VsMtx);
    glm_mat4_mul(Scr.VsMtx, Scr.WvMtx, Scr.WsMtx);
}

static void initGL() {
    GLenum res = glewInit();
    if (res != GLEW_OK) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(res));
        exit(1);
    }
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f); // Background color
    glShadeModel(GL_SMOOTH); // Use Gouraud shading
    glDisable(GL_CULL_FACE); // Disable backface culling, Klonoa 2 does not implement it
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendColor(1.0f, 1.0f, 1.0f, 0.5f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
}

static void init() {
    // Init GL
    initGL();

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

    // Init scene
    LightInit();
    initCamera();

    // Init GL stuff
    compileShaders();
    generateSpecTexture();
    createBgVertexBuffer();
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
        return mimePtrs[lod][idx - 1].vertex != NULL ? mimePtrs[lod][idx - 1].name : "[null]";
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
    createSfxVertexBuffer();
    generateSfxTextures();

    // Create mime names
    int p = 0;
    for (SFXOBJ *pObjTmp = pObj; pObjTmp != NULL; pObjTmp = pObjTmp->pObjSub, p++) {
        if (pObjTmp->MimeAdrs != NULL) {
            for (int i = 0; i < *pObjTmp->MimeAdrs; i++) {
                if (mimePtrs[p][i].vertex != NULL) {
                    sprintf(mimePtrs[p][i].name, "%d (part %d)", i, mimePtrs[p][i].part);
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

    glDeleteVertexArrays(sizeof(sfxVAO) / sizeof(sfxVAO[0][0]), sfxVAO[0]);
    glDeleteBuffers(sizeof(sfxVBO) / sizeof(sfxVBO[0][0]), sfxVBO[0]);
    glDeleteTextures(sizeof(sfxTextures) / sizeof(sfxTextures[0][0][0]), sfxTextures[0][0]);
    memset(sfxTextures, 0, sizeof(sfxTextures));
    memset(mimePtrs, 0, sizeof(mimePtrs));
    free(pObj);
    pObj = NULL;
    faceMime = 0;
    lod = 0;
}

static void drawGui() {
    ImVec2 buttonSize = { 0.0f, 0.0f };

    igText("FPS: %.0f", io->Framerate);

    if (igListBox_FnStrPtr("Models", &chrIdx, modelNameGetter, NULL, chrListLength + 1, 8)) {
        sfxClear();
        if (chrIdx != 0) {
            sfxInit(chrList[chrIdx - 1]);
        }
    }

    if (pObj != NULL) {
        if (igListBox_Str_arr("Size\nthis is what the\ndevs called the LODs\ndon't @ me", &lod, lodNames, 4, 4)) {
            SetObjCondition(pObj, 1 << lod);
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

    // if (igCollapsingHeader_TreeNodeFlags("Camera", 0)) {

    // }

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
        igCheckbox("Interpolate", &interpolate);
        igCheckbox("Adjust speed to refresh rate", &adjustFramerate);
        igSliderFloat("Frame", &pObjAct->pMot->Mb[baseIndex].MotionCnt, 0.0f, pObjAct->pMot->Mb[baseIndex].MotionEndCnt - 1.0f, NULL, ImGuiSliderFlags_AlwaysClamp);
        igSliderFloat("Speed", &pObjAct->pMot->Mb[baseIndex].CntSpeed, 0.0f, 4.0f, NULL, ImGuiSliderFlags_AlwaysClamp);
        
        s32 act = pObjAct->pMot->ActNum;
        if (igListBox_FnStrPtr("Animations", &act, animationNameGetter, pObjAct, pObjAct->pMot->ActNumMax, 8)) {
            if (interpolate) {
                SetActIp(pObj, act);
            } else {
                SetAct(pObj, act);
            }

            if (adjustFramerate) {
                pObjAct->pMot->Mb[pObjAct->pMot->BaseIndex].CntSpeed = 60.0f / io->Framerate;
            }
        }
    }

    if (pObjAct->MimeAdrs != NULL && igCollapsingHeader_TreeNodeFlags("Mime", 0)) {
        igSliderFloat("Weight", &mimeWeight, 0.0f, 1.0f, NULL, 0);
        igListBox_FnStrPtr("Mimes", &faceMime, mimeNameGetter, NULL, *pObjAct->MimeAdrs, 8);
    }
}

int windowMode() {
    // Init GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Klonoa 2 Renderer", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to create GLFW window\n");
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizecallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetCursorPosCallback(window, cursorPositioncallback);
    glfwSetMouseButtonCallback(window, mouseCallback);

    // Init ImGui
    igCreateContext(NULL);
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
    igStyleColorsDark(NULL);
    io = igGetIO();

    // Other init stuff
    init();

    // Custom lighting
    SETVEC(Light3.light2, -0.5f, -0.5f, 0.0f, 0.0f);
    SETVEC(Light3.color2, 0.15f, 0.15f, 0.15f, 0);
    Vu0NormalLightMatrix(Light3.NormalLight, Light3.light0, Light3.light1, Light3.light2);
    Vu0LightColorMatrix(Light3.LightColor, Light3.color0, Light3.color1, Light3.color2, Light3.ambient);

    // Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        processInput(window);

        renderFunc();

        // ImGui overlay
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        igNewFrame();
        {
            igBegin("Controls", NULL, 0);
            drawGui();
            igEnd();
        }
        igRender();
        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    igDestroyContext(NULL);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int framerate;
    float trans_y;
    float trans_z;
    float rot_x;
    float rot_y;
} RenderSettings;

int renderMode(const char *filename, const char *anim, RenderSettings *settings) {
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint major, minor;
    eglInitialize(display, &major, &minor);

    EGLint numConfigs;
    EGLConfig cfg;
    EGLint pbattr[] = {
        EGL_WIDTH, settings->width,
        EGL_HEIGHT, settings->height,
        EGL_NONE,
    };
    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_NONE
    };    

    eglChooseConfig(display, configAttribs, &cfg, 1, &numConfigs);
    EGLSurface surface = eglCreatePbufferSurface(display, cfg, pbattr);
    eglBindAPI(EGL_OPENGL_API);
    EGLContext eglCtx = eglCreateContext(display, cfg, EGL_NO_CONTEXT, NULL);    
    eglMakeCurrent(display, surface, surface, eglCtx);

    init();
    glm_mat4_identity(Scr.WvMtx);
    glm_translate_y(Scr.WvMtx, settings->trans_y);
    glm_translate_z(Scr.WvMtx, settings->trans_z);
    glm_rotate_x(Scr.WvMtx, glm_rad(settings->rot_x), Scr.WvMtx);
    glm_rotate_y(Scr.WvMtx, glm_rad(settings->rot_y), Scr.WvMtx);
    glm_perspective(glm_rad(45.0f), (float)settings->width / (float)settings->height, 0.1f, 10000000.0f, Scr.VsMtx);
    sfxInit(filename);

    SFXOBJ *pObjAct = GetActiveSfx(pObj);
    sfxAct(anim);
    pObjAct->pMot->Mb[pObjAct->pMot->BaseIndex].CntSpeed = 60.0f / FRAMERATE;
    pObjAct->pMot->Mb[pObjAct->pMot->BaseIndex].StopFlag = 1;

    u8 *img = malloc(settings->width * settings->height * 3);
    for (int i = 0; pObjAct->pMot->Mb[pObjAct->pMot->BaseIndex].MotionCnt < pObjAct->pMot->Mb[pObjAct->pMot->BaseIndex].MotionEndCnt; i++) {
        renderFunc();
        eglSwapBuffers(display, surface);
        glFlush();
        glReadPixels(0, 0, settings->width, settings->height, GL_BGR, GL_UNSIGNED_BYTE, img);

        char name[32];
        sprintf(name, "fb/fb%04d.tga", i);
        writeTga(name, settings->width, settings->height, 3, img);
        printf("%d\n", i);
    }

    free(img);
    eglTerminate(display);
    return 0;
}

int main(int argc, char *argv[]) {
    RenderSettings settings = {
        .width = SCR_WIDTH,
        .height = SCR_HEIGHT,
        .framerate = FRAMERATE,
        .trans_y = -22.0f,
        .trans_z = -80.0f,
        .rot_x = 0.0f,
        .rot_y = 0.0f
    };

    ivec4 normal = { 0xACF, 0xA34, 0x5EA, 0x00 };
    ivec4 weights = { 0xFF, 0x00, 0x00, 0x00 };
    mat4 spec[4];
    glm_mat4_identity(spec[0]);
    glm_mat4_identity(spec[1]);
    glm_mat4_identity(spec[2]);
    glm_mat4_identity(spec[3]);

    vec4 vf01, vf02, vf03, vf04, vf05, vf06;
    ivec4 out;
    Vu0ITOF12Vector(vf01, normal);
    Vu0ITOF0Vector(vf02, weights);
    glm_vec4_divs(vf02, 255.0f, vf02);
    glm_mat4_mulv(spec[0], vf01, vf03);
    glm_mat4_mulv(spec[1], vf01, vf04);
    glm_mat4_mulv(spec[2], vf01, vf05);
    glm_mat4_mulv(spec[3], vf01, vf06);
    glm_vec4_scale(vf03, vf02[0], vf01);
    glm_vec4_muladds(vf04, vf02[1], vf01);
    glm_vec4_muladds(vf05, vf02[2], vf01);
    glm_vec4_muladds(vf06, vf02[3], vf01);
    glm_vec4_adds(vf01, 1.0f, vf01);
    glm_vec4_scale(vf01, 32.0f, vf01);
    Vu0FTOI4Vector(out, vf01);

    if (argc == 1) {
        return windowMode();
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

    return renderMode(argv[1], argv[2], &settings);
}
