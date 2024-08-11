#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void* romfs_read(const char *filename);
size_t romfs_size(const char *filename);
char** romfs_list(size_t *size);

#ifdef __cplusplus
}
#endif