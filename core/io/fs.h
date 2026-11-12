// Proste operacje na plikach/katalogach (POSIX: dziala na PC i na konsoli).
#pragma once

namespace dp {
namespace fs {

bool exists(const char* path);
bool makeDirs(const char* path);    // jak mkdir -p
bool removeTree(const char* path);  // usuwa katalog z zawartoscia
// Podkatalogi 'dir' (bez ukrytych i zaczynajacych sie od '_'), od najnowszego.
int listDirs(const char* dir, char (*names)[32], int max);
// Pliki z rozszerzeniem ext (np. ".dpr") w katalogu, alfabetycznie; nazwy bez rozszerzenia.
int listFiles(const char* dir, const char* ext, char (*names)[32], int max);

}  // namespace fs
}  // namespace dp
