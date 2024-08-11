#pragma once

#include "types.h"
#include "sfx.h"

typedef struct { // 0xf0
    /* 0x00 */ FVECTOR light0;
    /* 0x10 */ FVECTOR light1;
    /* 0x20 */ FVECTOR light2;
    /* 0x30 */ FVECTOR color0;
    /* 0x40 */ FVECTOR color1;
    /* 0x50 */ FVECTOR color2;
    /* 0x60 */ FVECTOR ambient;
    /* 0x70 */ FMATRIX NormalLight;
    /* 0xb0 */ FMATRIX LightColor;
} LIGHT3;

typedef struct {
    FMATRIX WvMtx;
    FMATRIX VsMtx;
    FMATRIX WsMtx;
} SCRENV;

extern LIGHT3 Light3;
extern SCRENV Scr;
extern FMATRIX SfxLsMtx[64];
extern FMATRIX SfxLvMtx[64];
extern FMATRIX SfxLvSpecMtx[64];
extern FMATRIX SfxLcLightMtx[64];
extern FMATRIX SfxSkinMtx[64];
extern FMATRIX SfxInverseBindMtx[64];
extern IVECTOR SfxVertexI[2048];
extern IVECTOR SfxColorI[2048];
extern IVECTOR SfxSpecUVI[2048];
extern s32 SfxAllPause;

extern void ModelDraw(SFXOBJ *pObj);
extern s32 GetSfxOutFlag(SFXOBJ *pObj);
extern SFXOBJ* GetSfxObjectData(s32 ChrNum);
extern void SetSfxData(char *pSfxData);
extern s32 SetSfxWork(u32 *DataBuff, u32 *SfxAdrs);
extern s32 SetSfxAct(u32 *DataBuff, SFXOBJ *pObj, u32 *ActAdrs);
extern void PartsEnvInit(SFXOBJ *pObj);
extern void SetObjSubScale(SFXOBJ *pObj, f32 Scale);
extern void SetObjScale(SFXOBJ *pObj, FVECTOR ScaleVector);
extern void SetObjGmsTransType(SFXOBJ *pObj, s32 type);
extern void SetObjClipFlag(SFXOBJ *pObj, s32 flag);
extern void GetObjMatrixTrans(SFXOBJ *pObj, s32 num, FVECTOR DistVector);
extern void SetObjNormalLight(SFXOBJ *pObj, FMATRIX *NormalLight);
extern void SetObjOutlineOff(SFXOBJ *pObj);
extern void SetObjEffDraw(SFXOBJ *pObj);
extern void SetObjLightColor(SFXOBJ *pObj, FMATRIX *LightColor);
extern void OutLineEnvInit(u32 frame, u32 inter);
extern void LinkActTbl(MOTION *m, ACTTBL *pActtbl);
extern void SetObjCondition(SFXOBJ *pObj, s16 Condition);
extern void SetObjPause(SFXOBJ *pObj, u32 flag);
extern SFXOBJ* GetActiveSfx(SFXOBJ *pObj);
extern SFXOBJ* GetFlagObjPtr(SFXOBJ *pObj, u32 Condition);
extern void EraseSfxObject(SFXOBJ *pObj);
extern void MixClut(u16 Cbp, s32 Num0, s32 Num1, f32 Weight);
extern void SetSfxVariationClut(SFXOBJ *pObj, u32 ClutNum0, u32 ClutNum1, f32 Weight);
extern void SetSfxVariationGms(SFXOBJ *pObj, u32 GmsNum);
extern s32 SetSyncTex(SFXOBJ *pObj);
extern void SetSfxMotionSync(SFXOBJ *pObj0, SFXOBJ *pObj1);
extern s32 SetSfxActSimple(u32 *DataBuff, SFXOBJ *pObj);
extern SFXOBJ* SetSfxObject(u32 *DataAdrs);
extern SFXOBJ* SetSvxObject(u32 *DataAdrs);
extern void SetObjClipZone(SFXOBJ *pObj, f32 offset, f32 zone);
extern s32 GetObjShadowData(SFXOBJ *pObj, FVECTOR TmpVec);
extern void SetObjShadow(SFXOBJ *pObj, s32 Type, f32 Size, f32 Offset, f32 Range);
extern s32 SfxObjBallClipCheck(SFXOBJ *pObj);
extern void SetObjAllPause(s32 flag);