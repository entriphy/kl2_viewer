#include <stdlib.h>
#include <stdint.h>
#include "object.h"
#include "sfxbios.h"
#include "motsys.h"
#include "vu0.h"

#define ALIGN_BUF(x) ((void *)(((size_t)x + 0xF) & ~0xF))

ACTTBL DfMot[128] = {}; // TODO: This needs to be initialized
static char OutLineStatusDef[64] = {};
static char OutLineStatusErase[64] = {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
};
// u128 *SfxPacketBuff[2] = {};
// s32 SpActive = 0;
// DMAPTR pDma = {};
SCRENV Scr = {};
LIGHT3 Light3 = {};
// char MemFlag[512] = {};
char *SfxDataPtr = NULL;
// char *SfxWorkBuff = NULL;
FMATRIX SfxLsMtx[64] = {};
FMATRIX SfxLvMtx[64] = {};
FMATRIX SfxLvSpecMtx[64] = {};
FMATRIX SfxLcLightMtx[64] = {};
FMATRIX SfxSkinMtx[64] = {};
FMATRIX SfxInverseBindMtx[64] = {}; // added this
IVECTOR SfxVertexI[2048] = {};
IVECTOR SfxColorI[2048] = {};
IVECTOR SfxSpecUVI[2048] = {};
// kitOutlineDrawEnv OutlineEnv = {};
IVECTOR bboxTmp[2] = {};
// u32 *DataBuffPtr = NULL;
// u32 SfxFrame = 0;
// u32 SfxInter = 0;
// f32 ActCntSpd = 0.0f;
// f32 *pSfxClip = NULL;
s32 SfxAllPause = 0;
// s32 SfxDebugCnt = 0;

s32 GetSfxOutFlag(SFXOBJ *pObj) {
    return pObj->OutFlag;
}

SFXOBJ* GetSfxObjectData(s32 ChrNum) {
    SFXOBJ *pObj;
    s32 ShType;
    f32 ShSize;
    f32 ShOffset;
    f32 ShRange;
    f32 *Sh; // ?

    if (Fadr(SfxDataPtr, ChrNum) == Fadr(SfxDataPtr, ChrNum + 1)) {
        ChrNum = 820;
    }

    pObj = SetSfxObject(Fadr(SfxDataPtr, ChrNum));
    pObj->ObjNum = ChrNum;

    Sh = Fadr(SfxDataPtr, 936);
    ShType = ((s32 *)Sh + ChrNum * 6)[2];
    ShSize = (Sh + ChrNum * 6)[3];
    ShOffset = (Sh + ChrNum * 6)[4];
    ShRange = (Sh + ChrNum * 6)[5];
    SetObjClipZone(pObj, (Sh + ChrNum * 6)[0], (Sh + ChrNum * 6)[1]);
    SetObjShadow(pObj, ShType, ShSize, ShOffset, ShRange);

    return pObj;
}

void SetSfxData(char *pSfxData) {
    SfxDataPtr = pSfxData;
}

s32 SetSfxWork(u32 *DataBuff, u32 *SfxAdrs) {
	s32 i;
	SFXOBJ *pObj;
	char *TmpDataBuff;
	TYPE_SFX_HEADER *pSfx;
	s32 PartsBuffSize;
	TYPE_PARTS_TBL *pParts_Skin;
	TYPE_PARTS_TBL2 *pParts_Fix;
	sceGsTex0 MinTex0;
    
    pObj = (SFXOBJ *)DataBuff;
    pSfx = (TYPE_SFX_HEADER *)SfxAdrs;
    TmpDataBuff = (char *)ALIGN_BUF((uintptr_t)DataBuff + sizeof(SFXOBJ));
    pObj->pParts = (PARTS *)TmpDataBuff;
    pObj->PartsNum = pSfx->parts_num;
    pObj->scale = pSfx->scale;
    pObj->ClipFlag = 1;
    pObj->ScaleVector[0] = 1.0f;
    pObj->ScaleVector[1] = 1.0f;
    pObj->ScaleVector[2] = 1.0f;
    pObj->ScaleVector[3] = 1.0f;
    pObj->pLightColor = &Light3.LightColor;
    pObj->pNormalLight = &Light3.NormalLight;
    pObj->LineEnv.partsmax = pObj->PartsNum - 1;
    pObj->OutLineFlag = 1;
    pObj->GmsTransType = 1;
    pParts_Skin = (TYPE_PARTS_TBL *)(pSfx + 1);
    pParts_Fix = (TYPE_PARTS_TBL2 *)pParts_Skin;
    MinTex0.CBP = 0x3FFF;

    for (i = 0; i < pObj->PartsNum; i++, pParts_Skin++, pParts_Fix++) {
        switch (pParts_Skin->type) {
            case 0:
                pObj->pParts[i].type = pParts_Fix->type;
                pObj->pParts[i].vert_num = pParts_Fix->vert_num;
                pObj->pParts[i].norm_num = pParts_Fix->norm_num;
                pObj->pParts[i].uv_num = pParts_Fix->uv_num;
                pObj->pParts[i].prim_num = pParts_Fix->strip_num;
                pObj->pParts[i].sfx_adrs = (s32 *)pSfx;
                pObj->pParts[i].uv_adrs = (s32 *)((uintptr_t)pSfx + (uintptr_t)pParts_Fix->uv_adrs);
                pObj->pParts[i].prim_adrs = (s32 *)((uintptr_t)pSfx + (uintptr_t)pParts_Fix->strip_adrs);
                pObj->pParts[i].vert_adrs = (s32 *)((uintptr_t)pSfx + (uintptr_t)pParts_Fix->vert_adrs);
                pObj->pParts[i].norm_adrs = (s32 *)((uintptr_t)pSfx + (uintptr_t)pParts_Fix->norm_adrs);
                pObj->pParts[i].gs_tex0 = pParts_Fix->gs_tex0;

                pObj->pParts[i].TextureId = pParts_Fix->texture_id;
                break;
            case 1:
            case 3:
                pObj->pParts[i].type = pParts_Skin->type;
                pObj->pParts[i].jblock_num = pParts_Skin->jblock_num;
                pObj->pParts[i].vert_num = pParts_Skin->vert_num;
                pObj->pParts[i].norm_num = pParts_Skin->norm_num;
                pObj->pParts[i].uv_num = pParts_Skin->uv_num;
                pObj->pParts[i].prim_num = pParts_Skin->strip_num;
                pObj->pParts[i].sfx_adrs = (s32 *)pSfx;
                pObj->pParts[i].jblock_adrs = (s32 *)((uintptr_t)pSfx + (uintptr_t)pParts_Skin->jblock_adrs);
                pObj->pParts[i].uv_adrs = (s32 *)((uintptr_t)pSfx + (uintptr_t)pParts_Skin->uv_adrs);
                pObj->pParts[i].prim_adrs = (s32 *)((uintptr_t)pSfx + (uintptr_t)pParts_Skin->strip_adrs);
                pObj->pParts[i].vert_adrs = (s32 *)((uintptr_t)pSfx + (uintptr_t)pParts_Skin->vert_adrs);
                pObj->pParts[i].norm_adrs = (s32 *)((uintptr_t)pSfx + (uintptr_t)pParts_Skin->norm_adrs);
                pObj->pParts[i].gs_tex0 = pParts_Skin->gs_tex0;

                pObj->pParts[i].TextureId = pParts_Skin->texture_id;
                break;
            default:
                break;
        }

        pObj->pParts[i].coord_id = 0;
        pObj->pParts[i].GmsAdr = NULL;
        {
            PARTS *part = &pObj->pParts[i];
            if (MinTex0.CBP > part->gs_tex0.CBP) {
                MinTex0.CBP = part->gs_tex0.CBP;
            }
        }
        
    }

    pObj->Cbp = MinTex0.CBP;
    pObj->ClutNum0 = 0;
    pObj->ClutNum1 = 0;
    pObj->ClutWeight = 0.0f;
    pObj->MotionSyncFlag = 0;
    pObj->SvxAdrs = NULL;
    pObj->SvxWorkAdrs = NULL;
    pObj->GmsNum = 0;
    // PartsEnvInit(pObj);

    {
        u32 var_a0 = ~0xF;
        u32 var_v1 = sizeof(PARTS);
        TmpDataBuff = (char *)((uintptr_t)TmpDataBuff + pObj->PartsNum * var_v1);
        TmpDataBuff = (char *)(((uintptr_t)TmpDataBuff + 0xF) & var_a0);
    }
    
    return (s32)((uintptr_t)TmpDataBuff - (uintptr_t)DataBuff);
}

s32 SetSfxAct(u32 *DataBuff, SFXOBJ *pObj, u32 *ActAdrs) {
    s32 *TmpDataBuff;
    s32 CoordBuffSize;
    MOTION *m;

    if (*ActAdrs == 0) {
        pObj->pMot = pObj->pObjTop->pMot;
        return 0;
    }

    TmpDataBuff = (s32 *)ALIGN_BUF(DataBuff);
    pObj->pMot = (MOTION *)TmpDataBuff;
    m = pObj->pMot;
    TmpDataBuff = (s32 *)ALIGN_BUF(TmpDataBuff + sizeof(MOTION));
    m->ActAdrs = (u8 *)ActAdrs;
    m->ActNum = 1;
    m->ActNumMax = *(u16 *)ActAdrs;
    InitSfxCoord(pObj->pMot, Fadr(ActAdrs, 0), (tagCOORD *)TmpDataBuff);
    m->pActtbl = DfMot;
    m->pBaseCoord = pObj->pObjTop->pMot->pBaseCoord;
    CoordBuffSize = sizeof(tagCOORD);
    TmpDataBuff = (s32 *)((char *)TmpDataBuff + CoordBuffSize * (m->CoordNum + 1));
    TmpDataBuff = (s32 *)ALIGN_BUF(TmpDataBuff);
    m->pBaseCoord->Trans[0] = 0.0f;
    m->pBaseCoord->Trans[1] = 0.0f;
    m->pBaseCoord->Trans[2] = 0.0f;
    m->pBaseCoord->Trans[3] = 1.0f;
    m->pBaseCoord->Rot[0] = 0.0f;
    m->pBaseCoord->Rot[1] = 0.0f;
    m->pBaseCoord->Rot[2] = 0.0f;
    m->pBaseCoord->Rot[3] = 0.0f;
    SetActSub(m, m->ActNum);
    return (s32)((char *)TmpDataBuff - (char *)DataBuff);
}

void SetObjSubScale(SFXOBJ *pObj, f32 Scale) {
    // Empty function
}

void SetObjScale(SFXOBJ *pObj, FVECTOR ScaleVector) {
    SFXOBJ *pObjTmp;

    for (pObjTmp = pObj; pObjTmp != NULL; pObjTmp = pObjTmp->pObjSub) {
        *(u128 *)pObjTmp->ScaleVector = *(u128 *)ScaleVector;
    }
}

void SetObjGmsTransType(SFXOBJ *pObj, s32 type) {
    SFXOBJ *pObjTmp;

    for (pObjTmp = pObj; pObjTmp != NULL; pObjTmp = pObjTmp->pObjSub) {
        pObjTmp->GmsTransType = type;
    }
}

void SetObjClipFlag(SFXOBJ *pObj, s32 flag) {
    SFXOBJ *pObjTmp;

    for (pObjTmp = pObj; pObjTmp != NULL; pObjTmp = pObjTmp->pObjSub) {
        pObjTmp->ClipFlag = flag;
    }
}

void GetObjMatrixTrans(SFXOBJ *pObj, s32 num, FVECTOR DistVector) {
    FMATRIX ScaleMtx;
    FMATRIX TmpMtx;
    FVECTOR TmpVec;

    GetScaleMtx(ScaleMtx, pObj->ScaleVector);
    Vu0InversMatrix(TmpMtx, pObj->pMot->pBaseCoord->Mtx);
    Vu0MulMatrix(ScaleMtx, ScaleMtx, TmpMtx);
    Vu0MulMatrix(ScaleMtx, pObj->pMot->pBaseCoord->Mtx, ScaleMtx);
    Vu0ApplyMatrix(TmpVec, ScaleMtx, pObj->pMot->pCoord[num].Mtx[3]);
    DistVector[0] = TmpVec[0];
    DistVector[1] = TmpVec[1];
    DistVector[2] = TmpVec[2];
    DistVector[3] = 1.0f;
}

void SetObjNormalLight(SFXOBJ *pObj, FMATRIX *NormalLight) {
    SFXOBJ *pObjTmp;

    pObjTmp = pObj;
    do {
        pObjTmp->pNormalLight = NormalLight;
        pObjTmp = pObjTmp->pObjSub;
    } while (pObjTmp != NULL);
}

void SetObjOutlineOff(SFXOBJ *pObj) {
    SFXOBJ *pObjSub;
    s32 i;

    for (pObjSub = pObj; pObjSub != NULL; pObjSub = pObjSub->pObjSub) {
        for (i = 0; i < pObjSub->PartsNum; i++) {
            pObjSub->pParts[i].OutLine = 4;
        }
        pObjSub->LineEnv.status = (u8 *)OutLineStatusErase;
    }
}

void SetObjEffDraw(SFXOBJ *pObj) {
    SFXOBJ *pObjSub;

    for (pObjSub = pObj; pObjSub != NULL; pObjSub = pObjSub->pObjSub) {
        pObjSub->OutLineFlag = 0;
    }
}

void SetObjLightColor(SFXOBJ *pObj, FMATRIX *LightColor) {
    SFXOBJ *pObjTmp;

    pObjTmp = pObj;
    do {
        pObjTmp->pLightColor = LightColor;
        pObjTmp = pObjTmp->pObjSub;
    } while (pObjTmp != NULL);
}

void LinkActTbl(MOTION *m, ACTTBL *pActtbl) {
    m->pActtbl = pActtbl;
}

void SetObjCondition(SFXOBJ *pObj, s16 Condition) {
    pObj->Condition = Condition;
}

void SetObjPause(SFXOBJ *pObj, u32 flag) {
    pObj->Pause = flag;
}

SFXOBJ* GetActiveSfx(SFXOBJ *pObj) {
    SFXOBJ *pObjTmp;

    for (pObjTmp = pObj->pObjTop; pObjTmp != NULL; pObjTmp = pObjTmp->pObjSub) {
        if (pObjTmp->Flag == (pObjTmp->Flag & pObjTmp->pObjTop->Condition)) {
            return pObjTmp;
        }
    }

    return NULL;
}

SFXOBJ* GetFlagObjPtr(SFXOBJ *pObj, u32 Condition) {
    if (pObj->Flag != (Condition & pObj->Flag)) {
        return GetFlagObjPtr(pObj->pObjSub, Condition);
    } else {
        return pObj;
    }
}

void EraseSfxObject(SFXOBJ *pObj) {
    if (pObj != NULL) {
        free(pObj);
    }
}

void SetSfxVariationClut(SFXOBJ *pObj, u32 ClutNum0, u32 ClutNum1, f32 Weight) {
    SFXOBJ *pObjTmp;

    for (pObjTmp = pObj; pObjTmp != NULL; pObjTmp = pObjTmp->pObjSub) {
        pObjTmp->ClutNum0 = ClutNum0;
        pObjTmp->ClutNum1 = ClutNum1;
        pObjTmp->ClutWeight = Weight;
    }
}

void SetSfxVariationGms(SFXOBJ *pObj, u32 GmsNum) {
    SFXOBJ *pObjTmp;

    for (pObjTmp = pObj; pObjTmp != NULL; pObjTmp = pObjTmp->pObjSub) {
        pObjTmp->GmsNum = GmsNum;
    }
}

// Custom function
void SetSfxActiveGms(SFXOBJ *pObj, u32 ActiveGms) {
    SFXOBJ *pObjTmp;

    for (pObjTmp = pObj->pObjTop; pObjTmp != NULL; pObjTmp = pObjTmp->pObjSub) {
        pObjTmp->ActiveGms = ActiveGms;
    }
}

s32 SetSyncTex(SFXOBJ *pObj) {
    s32 ii;
    u8 *pTex;
    ACT_HEADER *pActHeader;
    s16 TexNum;
    s32 MotionCnt;
    MOTION *m;

    if (pObj->GmsNum != 0) {
        SetSfxActiveGms(pObj, pObj->GmsNum + 1);
        pObj->GmsNum = 0;
    } else {
        m = pObj->pMot;
        MotionCnt = (u32)m->Mb[m->BaseIndex].MotionCnt;
        pActHeader = m->Mb[m->BaseIndex].pAct;
        if (pActHeader->TexAddrs != 0) {
            pTex = (u8 *)pActHeader + pActHeader->TexAddrs;
            TexNum = *pTex;
            pTex = pTex + (MotionCnt * TexNum + 1);
            if (TexNum > 1) {
                printf("@@@ SetSyncTex: TexNum > 1 you idiot\n");
            }
            for (ii = 0; ii < TexNum; ii++) {
                s32 yeet = *pTex++;
                if (yeet > 0) {
                    SetSfxActiveGms(pObj, yeet + 1);
                } else {
                    SetSfxActiveGms(pObj, 0);
                }
            }
        }
    }
}

void SetSfxMotionSync(SFXOBJ *pObj0, SFXOBJ *pObj1) {
    SFXOBJ *pObjTmp0;
    SFXOBJ *pObjTmp1;

    if (pObj0->pMot->CoordNum == pObj1->pMot->CoordNum && pObj0->PartsNum == pObj1->PartsNum) {
        for (pObjTmp0 = pObj0, pObjTmp1 = pObj1; pObjTmp0 != NULL; pObjTmp0 = pObjTmp0->pObjSub, pObjTmp1 = pObjTmp1->pObjSub) {
            pObjTmp0->MotionSyncFlag = 1;
            pObjTmp1->MotionSyncFlag = 2;
            pObjTmp0->pMot = pObjTmp1->pMot;
        }
    }
}

s32 SetSfxActSimple(u32 *DataBuff, SFXOBJ *pObj) {
    s32 *TmpDataBuff;
    MOTION *m; // ?

    TmpDataBuff = (s32 *)ALIGN_BUF(DataBuff);
    m = (MOTION *)TmpDataBuff;
    pObj->pMot = m;
    TmpDataBuff = (s32 *)ALIGN_BUF(TmpDataBuff + sizeof(MOTION));
    m->CoordNum = 0;
    m->pBaseCoord = (tagCOORD *)TmpDataBuff;
    TmpDataBuff = (s32 *)((uintptr_t)TmpDataBuff + sizeof(tagCOORD));
    m->pBaseCoord->Trans[0] = 0.0f;
    m->pBaseCoord->Trans[1] = 0.0f;
    m->pBaseCoord->Trans[2] = 0.0f;
    m->pBaseCoord->Trans[3] = 1.0f;
    m->pBaseCoord->Rot[0] = 0.0f;
    m->pBaseCoord->Rot[1] = 0.0f;
    m->pBaseCoord->Rot[2] = 0.0f;
    m->pBaseCoord->Rot[3] = 0.0f;

    return (s32)((uintptr_t)TmpDataBuff - (uintptr_t)DataBuff);
}

// SFX layout:
// 0: SFX file
// 1: GMS pack
// 2: SFZ pack
// 3: 

// Not matching: https://decomp.me/scratch/4yr7l
SFXOBJ* SetSfxObject(u32 *DataAdrs) {
    s32 i;
    u32 *TmpDataAdrs;
    u32 DataSize = 0;
    s32 ModelNum = *DataAdrs;
    s32 PartsNum;
    s32 CoordNum;
    u32 *ActAdrs;
    u32 *GmsAdrs;
    u32 *MimeAdrs;
    u32 *EnvAdrs;
    s32 CoordBuffSize;
    s32 PartsBuffSize;
    s32 MimeBuffSize = 0;
    u32 *BuffAdrs;
    SFXOBJ *pObj;
    SFXOBJ *pObjSub;
    SFXOBJ *pObjLast;
    u32 SimpleFlag;

    for (i = 0; i < ModelNum; i++) {
        TmpDataAdrs = Fadr(DataAdrs, i);
        DataSize = (DataSize + 0xF) & ~0xF;
        DataSize += sizeof(SFXOBJ);
        DataSize = (DataSize + 0xF) & ~0xF;

        BuffAdrs = Fadr(TmpDataAdrs, 0);
        PartsNum = ((TYPE_SFX_HEADER *)BuffAdrs)->parts_num;
        PartsBuffSize = PartsNum;
        PartsBuffSize *= sizeof(PARTS);
        DataSize += PartsBuffSize;
        DataSize = (DataSize + 0xF) & ~0xF;

        ActAdrs = Fadr(TmpDataAdrs, 3);
        if (*ActAdrs != 0) {
            BuffAdrs = Fadr(ActAdrs, 0);
            CoordNum = ((ACT_HEADER *)BuffAdrs)->PartsNum;
            DataSize += sizeof(MOTION);
            DataSize = (DataSize + 0xF) & ~0xF;
            CoordBuffSize = sizeof(tagCOORD);
            CoordBuffSize *= CoordNum + 1;
            DataSize += CoordBuffSize;
        }

        if (*ActAdrs == 0 && i == 0) {
            DataSize += sizeof(MOTION);
            DataSize = (DataSize + 0xF) & ~0xF;
            DataSize += sizeof(tagCOORD);
        }
    }

    TmpDataAdrs = Fadr(DataAdrs, 0);
    TmpDataAdrs = Fadr(TmpDataAdrs, 2);
    if (*TmpDataAdrs != 0) {
        DataSize = (DataSize + 0xF) & ~0xF;
        MimeBuffSize = (sizeof(MIME) + 0xF) & ~0xF;
        DataSize += (sizeof(MIME) + 0xF) & ~0xF;
    }
    
    pObj = malloc(DataSize);
    pObj->pObjTop = pObj;
    pObj->pObjSub = NULL;
    pObjLast = pObj;

    TmpDataAdrs = Fadr(DataAdrs, 0);
    BuffAdrs = Fadr(TmpDataAdrs, 0);
    BuffAdrs = (u32 *)((uintptr_t)pObj + SetSfxWork((u32 *)pObj, BuffAdrs));
    BuffAdrs = ALIGN_BUF(BuffAdrs);
    
    if (*(u32 *)Fadr(TmpDataAdrs, 3) != 0) {
        SimpleFlag = 0;
        BuffAdrs = (u32 *)((uintptr_t)BuffAdrs + SetSfxAct(BuffAdrs, pObj, Fadr(TmpDataAdrs, 3)));
        BuffAdrs = ALIGN_BUF(BuffAdrs);
    } else {
        SimpleFlag = 1;
        BuffAdrs = (u32 *)((uintptr_t)BuffAdrs + SetSfxActSimple(BuffAdrs, pObj));
        BuffAdrs = ALIGN_BUF(BuffAdrs);
    }
    
    if (Fadr(TmpDataAdrs, 1) == NULL) {
        pObj->GmsAdrs = NULL;
    } else {
        pObj->GmsAdrs = Fadr(TmpDataAdrs, 1);
    }
    
    if (Fadr(TmpDataAdrs, 2) == NULL) {
        pObj->MimeAdrs = NULL;
    } else {
        pObj->MimeAdrs = Fadr(TmpDataAdrs, 2);
    }

    if ((s32)*TmpDataAdrs < 5) {
        pObj->EnvAdrs = NULL;
    } else {
        EnvAdrs = Fadr(TmpDataAdrs, 4);
        if (*EnvAdrs != 0) {
            pObj->EnvAdrs = (SFXENV *)EnvAdrs;
        } else {
            pObj->EnvAdrs = NULL;
        }
    }

    pObj->Condition = 1;
    pObj->Flag = 1;
    pObj->Pause = 0;

    if (!SimpleFlag) {
        for (i = 1; i < ModelNum; i++) {
            TmpDataAdrs = Fadr(DataAdrs, i);
            pObjSub = (SFXOBJ *)BuffAdrs;
            pObjLast->pObjSub = pObjSub;
            pObjSub->pObjSub = NULL;
            pObjSub->pObjTop = pObj;
            BuffAdrs = (u32 *)((size_t)BuffAdrs + SetSfxWork(BuffAdrs, Fadr(TmpDataAdrs, 0)));
            BuffAdrs = ALIGN_BUF(BuffAdrs);

            BuffAdrs = (u32 *)((size_t)BuffAdrs + SetSfxAct(BuffAdrs, pObjSub, Fadr(TmpDataAdrs, 3)));
            BuffAdrs = ALIGN_BUF(BuffAdrs);
            
            GmsAdrs = Fadr(TmpDataAdrs, 1);
            if (*GmsAdrs != 0) {
                pObjSub->GmsAdrs = (s32 *)GmsAdrs;
            } else {
                pObjSub->GmsAdrs = pObj->GmsAdrs;
            }
    
            MimeAdrs = Fadr(TmpDataAdrs, 2);
            if (*MimeAdrs != 0) {
                pObjSub->MimeAdrs = (s32 *)MimeAdrs;
            } else {
                pObjSub->MimeAdrs = NULL;
            }
    
            if ((s32)*TmpDataAdrs < 5) {
                pObjSub->EnvAdrs = NULL;
            } else {
                EnvAdrs = Fadr(TmpDataAdrs, 4);
                if (*EnvAdrs != 0) {
                    pObjSub->EnvAdrs = (SFXENV *)EnvAdrs;
                } else {
                    pObjSub->EnvAdrs = NULL;
                }
            }
            
            pObjSub->Flag = 1 << i;
            pObjLast = pObjSub;
        }

        if (MimeBuffSize != 0) {
            for (pObjSub = pObj; pObjSub != NULL; pObjSub = pObjSub->pObjSub) {
                pObjSub->pMime = (MIME *)BuffAdrs;
                pObjSub->pMime->IdFlag = 0;
                pObjSub->pMime->pVmime = NULL;
            }
        } else {
            for (pObjSub = pObj; pObjSub != NULL; pObjSub = pObjSub->pObjSub) {
                pObjSub->pMime = NULL;
            }
        }
        
        for (pObjSub = pObj; pObjSub != NULL; pObjSub = pObjSub->pObjSub) {
            pObjSub->ClipOffset = -15.0f;
            pObjSub->ClipZone = 50.0f;
        }
    }

    if (pObj != NULL) {
        for (pObjSub = pObj; pObjSub != NULL; pObjSub = pObjSub->pObjSub) {
            if (pObjSub->EnvAdrs != NULL) {
                for (i = 0; i < pObjSub->PartsNum; i++) {
                    pObjSub->pParts[i].SpecType = pObjSub->EnvAdrs->spectype[i];
                    pObjSub->pParts[i].OutLine = pObjSub->EnvAdrs->outline[i];
                }
                pObjSub->LineEnv.status = (u8 *)pObjSub->EnvAdrs->outline;
            } else {
                for (i = 0; i < pObjSub->PartsNum; i++) {
                    pObjSub->pParts[i].SpecType = 0;
                    pObjSub->pParts[i].OutLine = 0;
                }
                pObjSub->LineEnv.status = (u8 *)OutLineStatusDef;
            }
        }
    }
    
    return pObj;
}

void SetObjClipZone(SFXOBJ *pObj, f32 offset, f32 zone) {
	SFXOBJ *pObjTmp;

    for (pObjTmp = pObj; pObjTmp != NULL; pObjTmp = pObjTmp->pObjSub) {
        pObjTmp->ClipOffset = offset;
        pObjTmp->ClipZone = zone;
    }
}

s32 GetObjShadowData(SFXOBJ *pObj, FVECTOR TmpVec) {
	s32 i;
	f32 MaxScale;

    for (i = 0, MaxScale = 0.0f; i < 3; i++) {
        if (pObj->ScaleVector[i] > MaxScale) {
            MaxScale = pObj->ScaleVector[i];
        }
    }

    TmpVec[0] = MaxScale * pObj->ShadowSize;
    TmpVec[1] = MaxScale * pObj->ShadowOffset;
    TmpVec[2] = MaxScale * pObj->ShadowRange;
    return pObj->ShadowType;
}

void SetObjShadow(SFXOBJ *pObj, s32 Type, f32 Size, f32 Offset, f32 Range) {
	SFXOBJ *pObjTmp;

    for (pObjTmp = pObj; pObjTmp != NULL; pObjTmp = pObjTmp->pObjSub) {
        pObjTmp->ShadowType = Type;
        pObjTmp->ShadowSize = Size;
        pObjTmp->ShadowOffset = Offset;
        pObjTmp->ShadowRange = Range;
    }
}

void SetObjAllPause(s32 flag) {
    SfxAllPause = flag;
}