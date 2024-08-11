#pragma once

#include "types.h"

#define SCE_GS_NOINTERLACE		0
#define SCE_GS_INTERLACE		1
#define	SCE_GS_FIELD			0
#define	SCE_GS_FRAME			1
#define SCE_GS_NTSC			2
#define SCE_GS_PAL			3
#define SCE_GS_PSMCT32			0
#define SCE_GS_PSMCT24			1
#define SCE_GS_PSMCT16			2
#define SCE_GS_PSMCT16S			10
#define SCE_GS_PSMT8			19
#define SCE_GS_PSMT4			20
#define SCE_GS_PSMT8H			27
#define SCE_GS_PSMT4HL			36
#define SCE_GS_PSMT4HH			44
#define SCE_GS_PSMZ32			48
#define SCE_GS_PSMZ24			49
#define SCE_GS_PSMZ16			50
#define SCE_GS_PSMZ16S			58
#define SCE_GS_ZNOUSE			0
#define SCE_GS_ZALWAYS			1
#define SCE_GS_ZGEQUAL			2
#define SCE_GS_ZGREATER			3
#define SCE_GS_NOCLEAR			0
#define SCE_GS_CLEAR			1
#define SCE_GS_MODULATE			0
#define SCE_GS_DECAL			1
#define SCE_GS_HILIGHT			2
#define SCE_GS_GHLIGHT2   		SCE_GS_HIGHLIGHT2
#define SCE_GS_HIGHLIGHT2		3
#define SCE_GS_NEAREST			0
#define SCE_GS_LINEAR			1
#define SCE_GS_NEAREST_MIPMAP_NEAREST	2
#define SCE_GS_NEAREST_MIPMAP_LINEAR	SCE_GS_NEAREST_MIPMAP_LENEAR
#define SCE_GS_NEAREST_MIPMAP_LENEAR	3
#define SCE_GS_LINEAR_MIPMAP_NEAREST	4
#define SCE_GS_LINEAR_MIPMAP_LINEAR	5
#define SCE_GS_PRIM_POINT		0
#define SCE_GS_PRIM_LINE		1
#define SCE_GS_PRIM_LINESTRIP		2
#define SCE_GS_PRIM_TRI			3
#define SCE_GS_PRIM_TRISTRIP		4
#define SCE_GS_PRIM_TRIFAN		5
#define SCE_GS_PRIM_SPRITE		6
#define SCE_GS_PRIM_IIP			(1<<3)
#define SCE_GS_PRIM_TME			(1<<4)
#define SCE_GS_PRIM_FGE			(1<<5)
#define SCE_GS_PRIM_ABE			(1<<6)
#define SCE_GS_PRIM_AA1			(1<<7)
#define SCE_GS_PRIM_FST			(1<<8)
#define SCE_GS_PRIM_CTXT1		0
#define SCE_GS_PRIM_CTXT2		(1<<9)
#define SCE_GS_PRIM_FIX			(1<<10)

#define SCE_GS_BITBLTBUF	0x50
#define SCE_GS_TRXPOS		0x51
#define SCE_GS_TRXREG		0x52
#define SCE_GS_TRXDIR		0x53

typedef struct {
    unsigned long TBP0:14;
    unsigned long TBW:6;
    unsigned long PSM:6;
    unsigned long TW:4;
    unsigned long TH:4;
    unsigned long TCC:1;
    unsigned long TFX:2;
    unsigned long CBP:14;
    unsigned long CPSM:4;
    unsigned long CSM:1;
    unsigned long CSA:5;
    unsigned long CLD:3;
} sceGsTex0;

typedef struct {
	u64 NLOOP:15;
	u64 EOP:1;
	u64 :16;
	u64 id:14;
	u64 PRE:1;
	// Start PRIM
	u64 PRIM:3;
	u64 IIP:1;
	u64 TME:1;
	u64 FGE:1;
	u64 ABE:1;
	u64 AA1:1;
	u64 FST:1;
	u64 CTXT:1;
	u64 FIX:1;
	// End PRIM
	u64 FLG:2;
	u64 NREG:4;
	u64 REGS0:4;
	u64 REGS1:4;
	u64 REGS2:4;
	u64 REGS3:4;
	u64 REGS4:4;
	u64 REGS5:4;
	u64 REGS6:4;
	u64 REGS7:4;
	u64 REGS8:4;
	u64 REGS9:4;
	u64 REGS10:4;
	u64 REGS11:4;
	u64 REGS12:4;
	u64 REGS13:4;
	u64 REGS14:4;
	u64 REGS15:4;
} sceGifTag __attribute__((aligned(16)));

typedef struct {
	u64 SBP:14;
	u64 :2;
	u64 SBW:6;
	u64 :2;
	u64 SPSM:6;
	u64 :2;
	u64 DBP:14;
	u64 :2;
	u64 DBW:6;
	u64 :2;
	u64 DPSM:6;
	u64 :2;
} sceGsBitbltbuf;

typedef struct {
	u64 XDR:2;
	u64 pad02:62;
} sceGsTrxdir;

typedef struct {
	u64 SSAX:11;
	u64 pad11:5;
	u64 SSAY:11;
	u64 pad27:5;
	u64 DSAX:11;
	u64 pad43:5;
	u64 DSAY:11;
	u64 DIR:2;
	u64 pad61:3;
} sceGsTrxpos;

typedef struct {
	u64 RRW:12;
	u64 pad12:20;
	u64 RRH:12;
	u64 pad44:20;
} sceGsTrxreg;

extern void gs_upload_image_PSMCT32(u8 *buffer, u32 dbp, u32 dbw, u32 dsax, u32 dsay, u32 rrw, u32 rrh);
extern void gs_upload_image_PSMT8(u8 *buffer, u32 dbp, u32 dbw, u32 dsax, u32 dsay, u32 rrw, u32 rrh);
extern void gs_upload_image_PSMT4(u8 *buffer, u32 dbp, u32 dbw, u32 dsax, u32 dsay, u32 rrw, u32 rrh);
extern void gs_read_image_PSMT4_PSMCT32(u8 *pixels, u32 dbp, u32 dbw, u32 rrw, u32 rrh, u32 cbp, u32 csa, u32 alphaReg);
extern void gs_read_image_PSMT8_PSMCT32(u8 *pixels, u32 dbp, u32 dbw, u32 rrw, u32 rrh, u32 cbp, u32 csa, u32 alphaReg);