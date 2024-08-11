#pragma once

#include "ps2.h"

typedef void (*VuMicroprog)(u32 addr, u8 *mem, void *cb);
typedef void (*GifDirect)(u32 qwc, void *data, void *cb);

extern s32 read_vifcodes(void *codes, u32 qwc, VuMicroprog prog, GifDirect direct, void *cb_data);
extern void upload_gms(void *data);