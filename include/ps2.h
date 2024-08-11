#pragma once

#include "gs.h"

#define VU1_MEM_SIZE 0x4000

#define SCE_VIF_NOP			(0x00U)
#define SCE_VIF_STCYCL		(0x01U)
#define SCE_VIF_OFFSET		(0x02U)
#define SCE_VIF_BASE		(0x03U)
#define SCE_VIF_ITOP		(0x04U)
#define SCE_VIF_STMOD		(0x05U)
#define SCE_VIF_MSKPATH3	(0x06U)
#define SCE_VIF_MARK		(0x07U)
#define SCE_VIF_FLUSHE		(0x10U)
#define SCE_VIF_FLUSH		(0x11U)
#define SCE_VIF_FLUSHA		(0x13U)
#define SCE_VIF_MSCAL		(0x14U)
#define SCE_VIF_MSCALF		(0x15U)
#define SCE_VIF_MSCNT		(0x17U)
#define SCE_VIF_STMASK		(0x20U)
#define SCE_VIF_STROW		(0x30U)
#define SCE_VIF_STCOL		(0x31U)
#define SCE_VIF_MPG			(0x4aU)
#define SCE_VIF_DIRECT		(0x50U)
#define SCE_VIF_DIRECTHL	(0x51U)
#define SCE_VIF_UNPACK		(0x60U)

#define SCE_VIF_UPK_S_32	(0x0U)
#define SCE_VIF_UPK_S_16	(0x1U)
#define SCE_VIF_UPK_S_8		(0x2U)
#define SCE_VIF_UPK_V2_32	(0x4U)
#define SCE_VIF_UPK_V2_16	(0x5U)
#define SCE_VIF_UPK_V2_8	(0x6U)
#define SCE_VIF_UPK_V3_32	(0x8U)
#define SCE_VIF_UPK_V3_16	(0x9U)
#define SCE_VIF_UPK_V3_8	(0xaU)
#define SCE_VIF_UPK_V4_32	(0xcU)
#define SCE_VIF_UPK_V4_16	(0xdU)
#define SCE_VIF_UPK_V4_8	(0xeU)
#define SCE_VIF_UPK_V4_5	(0xfU)

typedef union {
	u32 d;
	struct {
		u32 imm:16;
		u32 num:8;
		u32 cmd:7;
		u32 itr:1;
	} i;
    struct {
		u32 addr:10;
        u32 :4;
        u32 sign:1;
        u32 top:1;
		u32 num:8;
        u32 size:2;
        u32 elem:2;
		u32 cmd:3;
		u32 itr:1;
	} u;
} sceVifCode;

typedef struct {
    u64	qwc : 16;
    u64     : 10;
    u64 pcr : 2;
    u64 id  : 3;
    u64 irq : 1;
    u64 addr: 31;
    u64 mem : 1;
    u64 data: 64;
} sceDmaTag __attribute__ ((aligned(16)));
