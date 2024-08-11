#pragma once
#include "sfx.h"

extern void GetMotion(SFXOBJ *pObj);
extern void GetMotionSimple(SFXOBJ *pObj);
extern void GetLocalMotion(MOTION *m);
extern void GetSimpleSfxMatrix(SFXOBJ *pObj);
extern void GetScreenMotion(SFXOBJ *pObj);
extern void GetNormalLightMatrix(SFXOBJ *pObj);
extern void InitSfxCoord(MOTION *m, u8 *pInf, tagCOORD *pCoord);
extern void InitCoord(MOTION *m, u8 *pInf, u8 *pItr, u8 *pItrW, tagCOORD *pCoord);
extern void SetLocalMatrix(FVECTOR Rot, FVECTOR Tra, FMATRIX Mtx);
extern void InterPolateMatrix(FMATRIX dm, FMATRIX m0, FMATRIX m1, f32 Weight);
extern void MotionMix(FMATRIX *dm, FMATRIX *m0, FMATRIX *m1, s32 CoordNum, f32 Weight, u64 OnFlag);
extern void CopyRotMatrix(FMATRIX m0, FMATRIX m1);
extern void ChangeLocalMatrix(FMATRIX lm,FMATRIX wm,FMATRIX lwm);
extern void GetLwMtx(tagCOORD *pCoord);
extern void GetRotTransMatrixXYZ(FMATRIX mtx, FVECTOR rot, FVECTOR tra);
extern void GetRotTransMatrixYXZ(FMATRIX mtx, FVECTOR rot, FVECTOR tra);
extern void GetRotTransMatrixXZY(FMATRIX mtx, FVECTOR rot, FVECTOR tra);
extern void GetRotTransMatrixZXY(FMATRIX mtx, FVECTOR rot, FVECTOR tra);
extern void GetRotTransMatrixYZX(FMATRIX mtx, FVECTOR rot, FVECTOR tra);
extern void GetRotTransMatrixZYX(FMATRIX mtx, FVECTOR rot, FVECTOR tra);
extern void SetMotionWorldIp(MOTION *m, f32 *wipcnt);
extern void SetAct(SFXOBJ *pObj, s32 Actnum);
extern void SetActIp(SFXOBJ *pObj, s32 Actnum);
extern void SetActMix(SFXOBJ *pObj, s32 Actnum);
extern void SetActSub(MOTION *m, s32 ActNum);
extern void SetActIpSub(MOTION *m, s32 ActNum);
extern void SetActMixSub(MOTION *m, s32 Actnum);
extern s32 GetActStopFlag(SFXOBJ *pObj);
extern f32 GetActEndCnt(SFXOBJ *pObj);
extern f32 GetActCnt(SFXOBJ *pObj);
extern void SetBaseMatrix(SFXOBJ *pObj, FVECTOR Rot, FVECTOR Trans, s32 RotOrder);
extern void SetBaseMatrix2(SFXOBJ *pObj, FMATRIX SrcMatrix);
extern void GetScaleMtx(FMATRIX mtx, FVECTOR vec);
extern void NormalMatrix(FMATRIX m0, FMATRIX m1);
extern void DecodeMotion(FMATRIX *DecodeBuff, MOTION *m, s32 Ind);
extern void ClearQwordMem(u32 Addrs, u32 Num);
extern void AcxDecodeMotion(FMATRIX *DecodeBuff, MOTION *m, s32 Ind);