#include "gms.h"
#include "romfs.h"
#include "sfx.h"
#include "take/object.h"
#include "take/sfxbios.h"
#include <GL/glew.h>
#include <string.h>

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

struct {
    int part;
    void *vertex;
    void *normal;
} mimePtrs[4][128] = {}; // [lod][mime]

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

static void createSfxVertexBuffers(SFXOBJ *pObj) {
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

static void createSfxTextures(SFXOBJ *pObj) {
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

static void createSpecTexture() {
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

int backend_init() {
    GLenum res = glewInit();
    if (res != GLEW_OK) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(res));
        return 1;
    }
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f); // Background color
    glShadeModel(GL_SMOOTH); // Use Gouraud shading
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE); // Disable backface culling, Klonoa 2 does not implement it

    compileShaders();
    createSpecTexture();
    createBgVertexBuffer();

    return 0;
}

void backend_sfxInit(SFXOBJ *pObj) {
    createSfxVertexBuffers(pObj);
    createSfxTextures(pObj);
}

void backend_sfxClear(SFXOBJ *pObj) {
    glDeleteVertexArrays(sizeof(sfxVAO) / sizeof(sfxVAO[0][0]), sfxVAO[0]);
    glDeleteBuffers(sizeof(sfxVBO) / sizeof(sfxVBO[0][0]), sfxVBO[0]);
    glDeleteTextures(sizeof(sfxTextures) / sizeof(sfxTextures[0][0][0]), sfxTextures[0][0]);
    memset(sfxTextures, 0, sizeof(sfxTextures));
    memset(mimePtrs, 0, sizeof(mimePtrs));
}

void backend_bgDraw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(bgVAO);
    glUseProgram(bgShader);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void backend_sfxDraw(SFXOBJ *pObj, int lodIndex, int mimeIndex, float mimeWeight) {
    glUseProgram(sfxShader);
    glUniformMatrix4fv(glGetUniformLocation(sfxShader, "Model"), 1, GL_FALSE, *pObj->pMot->pBaseCoord->Mtx);
    glUniformMatrix4fv(glGetUniformLocation(sfxShader, "View"), 1, GL_FALSE, *Scr.WvMtx);
    glUniformMatrix4fv(glGetUniformLocation(sfxShader, "Projection"), 1, GL_FALSE, *Scr.VsMtx);
    glUniformMatrix4fv(glGetUniformLocation(sfxShader, "LightColor"), 1, GL_FALSE, **pObj->pLightColor);
    glUniformMatrix4fv(glGetUniformLocation(sfxShader, "NormalLight"), 1, GL_FALSE, **pObj->pNormalLight);
    glUniformMatrix4fv(glGetUniformLocation(sfxShader, "Bones"), pObj->pMot->CoordNum, GL_FALSE, **SfxSkinMtx);
    glUniformMatrix4fv(glGetUniformLocation(sfxShader, "NormalLights"), pObj->pMot->CoordNum, GL_FALSE, **SfxLcLightMtx);

    for (int i = pObj->PartsNum - 1; i >= 0; i--) { // The game renders the parts in reverse order for some reason (possibly related to the outline effect?)
        PARTS *part = &pObj->pParts[i];

        glUseProgram(sfxShader);
        glBindVertexArray(sfxVAO[lodIndex][i]);
        glBindBuffer(GL_ARRAY_BUFFER, sfxVBO[lodIndex][i]);
        glBindTexture(GL_TEXTURE_2D, sfxTextures[lodIndex][pObj->ActiveGms][part->TextureId]);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        switch (part->type) {
            case 0: // Fixed
                glUniform1f(glGetUniformLocation(sfxShader, "MimeWeight"), 0.0f);
                break;
            case 1: // Skin
                glUniform1f(glGetUniformLocation(sfxShader, "MimeWeight"), 0.0f);
                break;
            case 3: // Mime
                if (mimeIndex != 0) {
                    break;
                }
                glUniform1f(glGetUniformLocation(sfxShader, "MimeWeight"), part->MimeWeight);
                glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[lodIndex][part->MimeStart].vertex);
                glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[lodIndex][part->MimeEnd].vertex);
                glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[lodIndex][part->MimeStart].normal);
                glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[lodIndex][part->MimeEnd].normal);
                break;
        }

        if (mimeIndex != 0 && i == mimePtrs[lodIndex][mimeIndex - 1].part) {
            void *vertexPtr;
            void *normalPtr;
            glGetVertexAttribPointerv(0, GL_VERTEX_ATTRIB_ARRAY_POINTER, &vertexPtr);
            glGetVertexAttribPointerv(1, GL_VERTEX_ATTRIB_ARRAY_POINTER, &normalPtr);
            glUniform1f(glGetUniformLocation(sfxShader, "MimeWeight"), mimeWeight);
            glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), vertexPtr);
            glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[lodIndex][mimeIndex - 1].vertex);
            glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), normalPtr);
            glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[lodIndex][mimeIndex - 1].normal);
        }

        TYPE_STRIP *strip = (TYPE_STRIP *)part->prim_adrs;
        GLint start = 0;
        for (int tri = 0; tri < part->prim_num; tri++) {
            glDrawArrays(GL_TRIANGLE_STRIP, start, strip->num);
            start += strip->num;
            strip += strip->num;
        }

        if (part->SpecType != 0) {
            glUseProgram(sfxSpecShader);
            glBindTexture(GL_TEXTURE_2D, sfxSpecTexture);
            glBlendColor(0.0f, 0.0f, 0.0f, 0.5f);
            // glBlendFuncSeparate(GL_SRC_COLOR, GL_DST_COLOR, GL_CONSTANT_ALPHA, GL_DST_ALPHA);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);

            switch (part->type) {
                case 0: // Fixed
                    glUniform1f(glGetUniformLocation(sfxSpecShader, "MimeWeight"), 0.0f);
                    break;
                case 1: // Skin
                    glUniform1f(glGetUniformLocation(sfxSpecShader, "MimeWeight"), 0.0f);
                    break;
                case 3: // Mime
                    glUniform1f(glGetUniformLocation(sfxSpecShader, "MimeWeight"), part->MimeWeight);
                    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[lodIndex][part->MimeStart].vertex);
                    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[lodIndex][part->MimeEnd].vertex);
                    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[lodIndex][part->MimeStart].normal);
                    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), mimePtrs[lodIndex][part->MimeEnd].normal);
                    break;
            }

            TYPE_STRIP *strip = (TYPE_STRIP *)part->prim_adrs;
            GLint start = 0;
            for (int tri = 0; tri < part->prim_num; tri++) {
                glDrawArrays(GL_TRIANGLE_STRIP, start, strip->num);
                start += strip->num;
                strip += strip->num;
            }
        }
    }
}

void backend_endDraw() {
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}