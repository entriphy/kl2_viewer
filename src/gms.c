#include <stddef.h>
#include <stdio.h>
#include "gms.h"
#include "gs.h"

static u8 vu1[VU1_MEM_SIZE] = {};

s32 read_vifcodes(void *codes, u32 qwc, VuMicroprog prog, GifDirect direct, void *cb_data) {
    s32 read = 0;

    while (read < qwc * 0x10) {
        sceVifCode *code = (sceVifCode *)((char *)codes + read);
        read += 4;

        switch (code->i.cmd) {
            case SCE_VIF_MSCAL:
            case SCE_VIF_MSCALF:
                if (prog != NULL) {
                    prog(code->u.addr, vu1, cb_data);
                }
                break;
            case SCE_VIF_DIRECT:
            case SCE_VIF_DIRECTHL:
                if (direct != NULL) {
                    direct(code->i.imm, (char *)codes + read, cb_data);
                }
                read += code->i.imm * 0x10;
                break;
            case SCE_VIF_NOP:
            case SCE_VIF_STCYCL:
            case SCE_VIF_OFFSET:
            case SCE_VIF_BASE:
            case SCE_VIF_ITOP:
            case SCE_VIF_STMOD:
            case SCE_VIF_MSKPATH3:
            case SCE_VIF_MARK:
            case SCE_VIF_FLUSHE:
            case SCE_VIF_FLUSH:
            case SCE_VIF_FLUSHA:
            case SCE_VIF_MSCNT:
            case SCE_VIF_STMASK:
            case SCE_VIF_STROW:
            case SCE_VIF_STCOL:
            case SCE_VIF_MPG:
                break;
            case SCE_VIF_UNPACK | SCE_VIF_UPK_S_32:
            {
                u32 *src = (u32 *)(code + 1);
                u32 *dst = (u32 *)(vu1 + code->u.addr * 16);
                for (int i = 0; i < code->i.num; i++) {
                    for (int j = 0; j < 3; j++) {
                        u32 s = *src++;
                        *dst++ = s;
                        *dst++ = s;
                        *dst++ = s;
                        *dst++ = s;
                    }
                    read += 12;
                }
                break;
            }
            case SCE_VIF_UNPACK | SCE_VIF_UPK_S_16:
            {
                s16 *src = (s16 *)(code + 1);
                s32 *dst = (s32 *)(vu1 + code->u.addr * 16);
                for (int i = 0; i < code->i.num; i++) {
                    for (int j = 0; j < 3; j++) {
                        s32 s = *src++;
                        *dst++ = s;
                        *dst++ = s;
                        *dst++ = s;
                        *dst++ = s;
                    }
                    read += 8;
                }
                break;
            }
            case SCE_VIF_UNPACK | SCE_VIF_UPK_S_8:
            {
                s8 *src = (s8 *)(code + 1);
                s32 *dst = (s32 *)(vu1 + code->u.addr * 16);
                for (int i = 0; i < code->i.num; i++) {
                    for (int j = 0; j < 3; j++) {
                        s32 s = *src++;
                        *dst++ = s;
                        *dst++ = s;
                        *dst++ = s;
                        *dst++ = s;
                    }
                    read += 4;
                }
                break;
            }
            case SCE_VIF_UNPACK | SCE_VIF_UPK_V2_32:
            {
                u32 *src = (u32 *)(code + 1);
                u32 *dst = (u32 *)(vu1 + code->u.addr * 16);
                for (int i = 0; i < code->i.num; i++) {
                    *dst++ = *src++;
                    *dst++ = *src++;
                    dst++; // indeterminate
                    dst++; // indeterminate
                    read += 8;
                }
                break;
            }
            case SCE_VIF_UNPACK | SCE_VIF_UPK_V2_16:
            {
                s16 *src = (s16 *)(code + 1);
                s32 *dst = (s32 *)(vu1 + code->u.addr * 16);
                for (int i = 0; i < code->i.num; i++) {
                    *dst++ = *src++;
                    *dst++ = *src++;
                    dst++; // indeterminate
                    dst++; // indeterminate
                    read += 4;
                }
                break;
            }
            case SCE_VIF_UNPACK | SCE_VIF_UPK_V2_8:
                break; // ?
            case SCE_VIF_UNPACK | SCE_VIF_UPK_V3_32:
            {
                u32 *src = (u32 *)(code + 1);
                u32 *dst = (u32 *)(vu1 + code->u.addr * 16);
                for (int i = 0; i < code->i.num; i++) {
                    *dst++ = *src++;
                    *dst++ = *src++;
                    *dst++ = *src++;
                    dst++; // indeterminate
                    read += 12;
                }
                break;
            }
            case SCE_VIF_UNPACK | SCE_VIF_UPK_V3_16:
            {
                s16 *src = (s16 *)(code + 1);
                s32 *dst = (s32 *)(vu1 + code->u.addr * 16);
                for (int i = 0; i < code->i.num; i++) {
                    *dst++ = *src++;
                    *dst++ = *src++;
                    *dst++ = *src++;
                    dst++; // indeterminate
                    read += 6;
                }
                break;
            }
            case SCE_VIF_UNPACK | SCE_VIF_UPK_V3_8:
            {
                u8 *src = (u8 *)(code + 1);
                s32 *dst = (s32 *)(vu1 + code->u.addr * 16);
                for (int i = 0; i < code->i.num; i++) {
                    *dst++ = *src++;
                    *dst++ = *src++;
                    *dst++ = *src++;
                    dst++; // indeterminate
                    read += 3;
                }
                break;
            }
            case SCE_VIF_UNPACK | SCE_VIF_UPK_V4_32:
            {
                u32 *src = (u32 *)(code + 1);
                u32 *dst = (u32 *)(vu1 + code->u.addr * 16);
                for (int i = 0; i < code->u.num; i++) {
                    *dst++ = *src++;
                    *dst++ = *src++;
                    *dst++ = *src++;
                    *dst++ = *src++;
                    read += 16;
                }
                break;
            }
            case SCE_VIF_UNPACK | SCE_VIF_UPK_V4_16:
            {
                s16 *src = (s16 *)(code + 1);
                s32 *dst = (s32 *)(vu1 + code->u.addr * 16);
                for (int i = 0; i < code->i.num; i++) {
                    *dst++ = *src++;
                    *dst++ = *src++;
                    *dst++ = *src++;
                    *dst++ = *src++;
                    read += 8;
                }
                break;
            }
            case SCE_VIF_UNPACK | SCE_VIF_UPK_V4_8:
                break; // ?
            case SCE_VIF_UNPACK | SCE_VIF_UPK_V4_5:
                break; // ?
            default:
                return -1;
        }

        read += 3;
        read &= ~0x3;
    }
    
    return read;
}

static void direct(u32 qwc, void *data, void *cb) {
    sceGifTag *giftag = (sceGifTag *)data;
    sceGsBitbltbuf *bitbltbuf = NULL;
    sceGsTrxpos *trxpos = NULL;
    sceGsTrxreg *trxreg = NULL;
    sceGsTrxdir *trxdir = NULL;

    while (giftag->EOP != 1) {
        // printf("GIFtag(%d, %d) @ 0x%04x\n", giftag->NLOOP, giftag->EOP, (u8 *)giftag - (u8 *)data + 0x10);
        qword_uni *data = (qword_uni *)(giftag + 1);
        switch (giftag->FLG) {
            case 0:
                for (int i = 0; i < giftag->NREG; i++) {
                    switch (data->u_u64[1]) {
                        case SCE_GS_BITBLTBUF:
                            bitbltbuf = (sceGsBitbltbuf *)data;
                            break;
                        case SCE_GS_TRXPOS:
                            trxpos = (sceGsTrxpos *)data;
                            break;
                        case SCE_GS_TRXREG:
                            trxreg = (sceGsTrxreg *)data;
                            break;
                        case SCE_GS_TRXDIR:
                            trxdir = (sceGsTrxdir *)data;
                            break;
                    }
                    data++;
                }
                giftag += giftag->NREG * giftag->NLOOP + 1;
                break;
            case 2: // IMAGE
                switch (bitbltbuf->DPSM) {
                    case SCE_GS_PSMCT32:
                        gs_upload_image_PSMCT32((u8 *)data, bitbltbuf->DBP, bitbltbuf->DBW, trxpos->DSAX, trxpos->DSAY, trxreg->RRW, trxreg->RRH);
                        break;
                    case SCE_GS_PSMT8:
                        gs_upload_image_PSMT8((u8 *)data, bitbltbuf->DBP, bitbltbuf->DBW, trxpos->DSAX, trxpos->DSAY, trxreg->RRW, trxreg->RRH);
                        break;
                    case SCE_GS_PSMT4:
                        gs_upload_image_PSMT4((u8 *)data, bitbltbuf->DBP, bitbltbuf->DBW, trxpos->DSAX, trxpos->DSAY, trxreg->RRW, trxreg->RRH);
                        break;
                }
                giftag += giftag->NLOOP + 1;
                break;
            default:
                break;
        }   
    }
}

void upload_gms(void *data) {
    sceDmaTag *dmatag = (sceDmaTag *)data;
    read_vifcodes((char *)data + 4, dmatag->qwc, NULL, direct, NULL);
}
