#include <algorithm>
#include <romfs/romfs.hpp>
#include "romfs.h"

extern "C" {

void* romfs_read(const char *filename) {
    try {
        return (void *)romfs::get(filename).data<char>();
    } catch (...) {
        return nullptr;
    }
}

size_t romfs_size(const char *filename) {
    return romfs::get(filename).size();
}

char** romfs_list(size_t *size) {
    auto list = romfs::list();
    std::sort(list.begin(), list.end());
    char **ret = (char **)malloc(sizeof(char *) * list.size());
    for (int i = 0; i < list.size(); i++) {
        auto str = list[i].string();
        auto len = str.length() + 1;
        ret[i] = (char *)malloc(len);
        str.copy(ret[i], len);
        ret[i][len - 1] = 0;
    }

    if (size != nullptr) *size = list.size();
    return ret;
}

}