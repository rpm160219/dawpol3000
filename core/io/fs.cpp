#include "io/fs.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

namespace dp {
namespace fs {

bool exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

bool makeDirs(const char* path) {
    char tmp[512];
    snprintf(tmp, sizeof tmp, "%s", path);
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return false;
            *p = '/';
        }
    }
    return mkdir(tmp, 0755) == 0 || errno == EEXIST;
}

bool removeTree(const char* path) {
    DIR* d = opendir(path);
    if (!d) return unlink(path) == 0;
    struct dirent* e;
    char sub[512];
    bool ok = true;
    while ((e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        snprintf(sub, sizeof sub, "%s/%s", path, e->d_name);
        struct stat st;
        if (stat(sub, &st) == 0 && S_ISDIR(st.st_mode)) ok &= removeTree(sub);
        else ok &= unlink(sub) == 0;
    }
    closedir(d);
    return ok && rmdir(path) == 0;
}

int listDirs(const char* dir, char (*names)[32], int max) {
    DIR* d = opendir(dir);
    if (!d) return 0;
    struct Item { char name[32]; long long t; };
    static Item items[256];
    int n = 0;
    struct dirent* e;
    char sub[512];
    while ((e = readdir(d)) && n < 256) {
        if (e->d_name[0] == '.' || e->d_name[0] == '_') continue;
        if (strlen(e->d_name) >= 32) continue;
        snprintf(sub, sizeof sub, "%s/%s", dir, e->d_name);
        struct stat st;
        if (stat(sub, &st) != 0 || !S_ISDIR(st.st_mode)) continue;
        snprintf(items[n].name, 32, "%s", e->d_name);
        items[n].t = (long long)st.st_mtime;
        n++;
    }
    closedir(d);
    qsort(items, n, sizeof(Item), [](const void* a, const void* b) {
        long long ta = ((const Item*)a)->t, tb = ((const Item*)b)->t;
        return ta < tb ? 1 : (ta > tb ? -1 : strcmp(((const Item*)a)->name, ((const Item*)b)->name));
    });
    if (n > max) n = max;
    for (int i = 0; i < n; i++) memcpy(names[i], items[i].name, 32);
    return n;
}

}  // namespace fs
}  // namespace dp
