#include "sfxbios.h"

void* Fadr(void *pAddr, s32 nNum) {
    return (char *)pAddr + ((u32 *)pAddr)[nNum + 1];
}
