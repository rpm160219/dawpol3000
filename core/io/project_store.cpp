#include "io/project_store.h"
#include "io/fs.h"
#include "io/project_file.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

namespace dp {

void ProjectStore::init(const char* root) {
    snprintf(root_, sizeof root_, "%s", root);
    fs::makeDirs(root_);
    refresh();
}

void ProjectStore::folderName(const char* name, char* out, int n) {
    int k = 0;
    for (const char* p = name; *p && k < n - 1; p++) {
        char ch = (char)toupper((unsigned char)*p);
        if (ch == ' ') ch = '_';
        if (!(isalnum((unsigned char)ch) || ch == '_' || ch == '-' || ch == '#')) continue;
        out[k++] = ch;
    }
    if (k == 0) out[k++] = 'X';
    out[k] = 0;
}

void ProjectStore::pathOf(const char* folder, char* out, int n) const {
    snprintf(out, n, "%s/%s/project.dpp", root_, folder);
}

int ProjectStore::refresh() {
    if (!ready()) return n_ = 0;
    n_ = fs::listDirs(root_, dirs_, MAX);
    for (int i = 0; i < n_; i++) {
        snprintf(disp_[i], 32, "%s", dirs_[i]);
        for (char* p = disp_[i]; *p; p++) if (*p == '_') *p = ' ';
    }
    return n_;
}

bool ProjectStore::exists(const char* name) const {
    char f[64];
    folderName(name, f, sizeof f);
    for (int i = 0; i < n_; i++) if (!strcmp(dirs_[i], f)) return true;
    return false;
}

bool ProjectStore::save(const Project& p) {
    if (!ready()) return false;
    char f[64], dir[512], path[512];
    folderName(p.name, f, sizeof f);
    snprintf(dir, sizeof dir, "%s/%s", root_, f);
    if (!fs::makeDirs(dir)) return false;
    pathOf(f, path, sizeof path);
    bool ok = saveProjectFile(p, path);
    if (ok) setLast(p.name);
    refresh();
    return ok;
}

bool ProjectStore::load(Project& p, int i, char* err, int errLen) {
    if (i < 0 || i >= n_) { snprintf(err, errLen, "NO PROJECT"); return false; }
    char path[512];
    pathOf(dirs_[i], path, sizeof path);
    if (!loadProjectFile(p, path, err, errLen)) return false;
    setLast(p.name);
    return true;
}

bool ProjectStore::remove(int i) {
    if (i < 0 || i >= n_) return false;
    char dir[512];
    snprintf(dir, sizeof dir, "%s/%s", root_, dirs_[i]);
    bool ok = fs::removeTree(dir);
    refresh();
    return ok;
}

void ProjectStore::setLast(const char* name) {
    char path[512], f[64];
    snprintf(path, sizeof path, "%s/_last.txt", root_);
    folderName(name, f, sizeof f);
    FILE* fp = fopen(path, "w");
    if (!fp) return;
    fprintf(fp, "%s\n", f);
    fclose(fp);
}

bool ProjectStore::loadLast(Project& p) {
    char path[512], f[64] = "";
    snprintf(path, sizeof path, "%s/_last.txt", root_);
    FILE* fp = fopen(path, "r");
    if (!fp) return false;
    if (!fgets(f, sizeof f, fp)) { fclose(fp); return false; }
    fclose(fp);
    f[strcspn(f, "\r\n")] = 0;
    char err[64];
    for (int i = 0; i < n_; i++)
        if (!strcmp(dirs_[i], f)) return load(p, i, err, sizeof err);
    return false;
}

}  // namespace dp
