#pragma once
#include <vector>
#include <string>

struct ModArchive {
    std::string id;
    std::string archivePath;
    std::string indexPath;
};

//  list of mods that provide an info.arc
extern std::vector<ModArchive> g_patched_archive_mods;

bool GeneratePatchedIndex();