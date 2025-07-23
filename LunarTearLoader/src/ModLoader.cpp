#include "ModLoader.h"
#include "Common/Logger.h"
#include <set>
#include <vector>
#include <algorithm>
#include <sstream>

using enum Logger::LogCategory;

static std::map<std::string, std::string> s_resolvedFileMap;
static std::mutex s_fileMapMutex;
static std::map<std::string, std::vector<char>> s_fileCache;
static std::mutex s_cacheMutex;

void ScanModsAndResolveConflicts() {
    const std::string mods_root = "LunarTear/mods/";
    if (!std::filesystem::exists(mods_root) || !std::filesystem::is_directory(mods_root)) {
        Logger::Log(Info) << "Mods directory not found, skipping mod scan";
        return;
    }

    Logger::Log(Info) << "Scanning for mods and resolving conflicts...";

    std::map<std::string, std::vector<std::string>> file_to_mods_map;
    for (const auto& mod_entry : std::filesystem::directory_iterator(mods_root)) {
        if (!mod_entry.is_directory()) continue;

        std::string mod_name = mod_entry.path().filename().string();
        Logger::Log(Verbose) << "Found mod: " << mod_name;

        for (const auto& file_entry : std::filesystem::recursive_directory_iterator(mod_entry.path())) {
            if (!file_entry.is_regular_file()) continue;

            std::string relative_path = std::filesystem::relative(file_entry.path(), mod_entry.path()).string();
            std::replace(relative_path.begin(), relative_path.end(), '\\', '/');
            file_to_mods_map[relative_path].push_back(mod_name);
        }
    }

    const std::set<std::string> relevant_extensions = { ".dds", ".lua", ".settbll" };

    const std::lock_guard<std::mutex> lock(s_fileMapMutex);
    s_resolvedFileMap.clear();

    for (auto& [file, mods] : file_to_mods_map) {
        std::filesystem::path p(file);
        std::string extension = p.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char c) { return std::tolower(c); });

        if (!relevant_extensions.count(extension)) {
            continue;
        }

        std::string winner_mod;
        if (mods.size() > 1) {
            std::sort(mods.begin(), mods.end());
            // The alphabetically last mod wins
            winner_mod = mods.back();

            std::stringstream losers_ss;
            for (size_t i = 0; i < mods.size() - 1; ++i) {
                losers_ss << "'" << mods[i] << "'" << (i < mods.size() - 2 ? ", " : "");
            }
            Logger::Log(Warning) << "Conflict for file '" << file << "'. Winner: '" << winner_mod
                << "'. Overridden mods: " << losers_ss.str() << ".";
        }
        else {
            winner_mod = mods[0];
        }

        std::filesystem::path full_path = std::filesystem::path(mods_root) / winner_mod / file;
        s_resolvedFileMap[file] = full_path.string();
        Logger::Log(Verbose) << "Mapping " << file << " -> " << s_resolvedFileMap[file];
    }

    Logger::Log(Info) << "Mod scan complete.";
}

void* LoadLooseFile(const char* filename, size_t& out_size) {


    std::string requested_file(filename);
    std::replace(requested_file.begin(), requested_file.end(), '\\', '/');

    std::string full_path;
    {
        const std::lock_guard<std::mutex> lock(s_fileMapMutex);
        auto it = s_resolvedFileMap.find(requested_file);
        if (it == s_resolvedFileMap.end()) {
            out_size = 0;
            return nullptr;
        }
        full_path = it->second;
    }

    const std::lock_guard<std::mutex> lock(s_cacheMutex);

    if (s_fileCache.count(full_path)) {
        out_size = s_fileCache[full_path].size();
        return s_fileCache[full_path].data();
    }

    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) {
        Logger::Log(Error) << "Error: Could not open resolved file: " << full_path;
        out_size = 0;
        return nullptr;
    }

    s_fileCache[full_path] = std::vector<char>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
    out_size = s_fileCache[full_path].size();
    return s_fileCache[full_path].data();
}