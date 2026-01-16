#include "ModLoader.h"
#include "Common/Logger.h"
#include "Common/Settings.h"
#include "API/api.h"
#include "VFS/ArchivePatcher.h"
#include "Common/Json.h"

#include <map>
#include <set>
#include <mutex>
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using enum Logger::LogCategory;

namespace {

    struct Mod {
        std::string id;
        std::filesystem::path rootPath;

        std::vector<std::pair<std::string, std::filesystem::path>> potentialTables;   // <filename, FullPath>
        std::vector<std::pair<std::string, std::filesystem::path>> potentialTextures; // <filename, FullPath>
        std::vector<std::pair<std::string, std::filesystem::path>> potentialScripts;  // <filename, FullPath>
        std::vector<std::filesystem::path> potentialWeapons;
        std::vector<std::filesystem::path> plugins;
        std::filesystem::path archivePath;
    };

    struct CachedFile {
        std::vector<char> data;
    };

    std::map<std::string, Mod> s_mods;

    // "Last one wins" 
    std::map<std::string, std::filesystem::path> s_resolvedTexturesMap;
    std::map<std::string, std::filesystem::path> s_resolvedTablesMap;

    // "Run all" 
    std::vector<std::pair<std::string, std::filesystem::path>> s_resolvedScriptsList;
    std::vector<nlohmann::json> resolvedWeaponsList;

    std::map<std::string, CachedFile> s_fileCache;
    std::mutex s_stateMutex;
    std::mutex s_cacheMutex;

    std::string NormalizePath(const std::string& pathStr) {
        std::string s = pathStr;
        std::replace(s.begin(), s.end(), '\\', '/');
        return s;
    }

    std::string ToLower(const std::string& input) {
        std::string out = input;
        std::transform(out.begin(), out.end(), out.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return out;
    }

    std::string DetermineModID(const std::filesystem::path& modDir) {
        std::filesystem::path manifestPath = modDir / "manifest.json";
        if (std::filesystem::exists(manifestPath)) {
            try {
                std::ifstream f(manifestPath);
                json data = json::parse(f);
                if (data.contains("name") && data["name"].is_string()) {
                    return data["name"];
                }
            }
            catch (...) {}
        }
        return modDir.filename().string();
    }


    void* LoadLooseInternal(const std::string& key, const std::map<std::string, std::filesystem::path>& map, size_t& out_size) {
        std::filesystem::path fullPath;
        {
            std::lock_guard<std::mutex> lock(s_stateMutex);
            auto it = map.find(key);
            if (it == map.end()) {
                out_size = 0;
                return nullptr;
            }
            fullPath = it->second;
        }

        std::string cacheKey = fullPath.string();
        std::lock_guard<std::mutex> lock(s_cacheMutex);

        auto it = s_fileCache.find(cacheKey);
        if (it != s_fileCache.end()) {
            out_size = it->second.data.size();
            return it->second.data.data();
        }

        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            out_size = 0;
            return nullptr;
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0, std::ios::beg);

        if (!file.read(buffer.data(), fileSize)) {
            out_size = 0;
            return nullptr;
        }

        auto& entry = s_fileCache[cacheKey];
        entry.data = std::move(buffer);
        out_size = entry.data.size();
        return entry.data.data();
    }

    enum class AssetType {
        TEX,
        TABLE,
        SCRIPT,
        WEAPON,
        PLUGIN,
    };

    template <typename Func>
    void ScanDirectoryIfExists(const std::filesystem::path& dir, Func action, std::set<AssetType> types, bool recursive = false) {
        if (!std::filesystem::exists(dir)) return;

        if (recursive) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
                if (entry.is_regular_file()) action(entry.path(), types);
            }
        }
        else {
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (entry.is_regular_file()) action(entry.path(), types);
            }
        }
    }
}


void ScanModsAndResolveConflicts() {
    const std::filesystem::path modsRoot = "LunarTear/mods/";
    if (!std::filesystem::exists(modsRoot)) return;

    std::lock_guard<std::mutex> lock(s_stateMutex);
    Logger::Log(Verbose) << "Starting Mod Scan...";

    s_mods.clear();
    s_resolvedTexturesMap.clear();
    s_resolvedTablesMap.clear();
    s_resolvedScriptsList.clear();
    g_patched_archive_mods.clear();


    for (const auto& entry : std::filesystem::directory_iterator(modsRoot)) {
        if (!entry.is_directory()) continue;

        Mod mod;
        mod.rootPath = entry.path();
        mod.id = DetermineModID(mod.rootPath);

        if (s_mods.count(mod.id)) {
            Logger::Log(Error) << "Duplicate Mod ID: " << mod.id;
            continue;
        }

        if (std::filesystem::exists(mod.rootPath / "info.arc")) {
            mod.archivePath = mod.rootPath / "info.arc";
        }



        auto fileProcessor = [&](const std::filesystem::path& path, std::set<AssetType> types) {
            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            using enum AssetType;

            if (ext == ".dll" && types.contains(PLUGIN)) {
                mod.plugins.push_back(path);
            }
            else if ((ext == ".settbll" || ext == ".settb") && types.contains(TABLE)) {
                mod.potentialTables.push_back({ path.filename().string(), path });
            }
            else if (ext == ".dds" && types.contains(TEX)) {
                mod.potentialTextures.push_back({ path.filename().string(), path });
            }
            else if (ext == ".lua" && types.contains(SCRIPT)) {
                mod.potentialScripts.push_back({ path.filename().string(), path });
            }
            else if (ext == ".json" && types.contains(WEAPON)) {
                mod.potentialWeapons.push_back(path);
            }

            };

        using enum AssetType;

        ScanDirectoryIfExists(mod.rootPath / "textures", fileProcessor, {TEX});
        ScanDirectoryIfExists(mod.rootPath / "tables", fileProcessor, {TABLE});
        ScanDirectoryIfExists(mod.rootPath / "scripts", fileProcessor, {SCRIPT});
        ScanDirectoryIfExists(mod.rootPath / "plugins", fileProcessor, {PLUGIN});
        ScanDirectoryIfExists(mod.rootPath / "weapons", fileProcessor, {WEAPON});

        ScanDirectoryIfExists(mod.rootPath / "loose", fileProcessor, {TEX});
        ScanDirectoryIfExists(mod.rootPath / "inject", fileProcessor, {SCRIPT} ,true);
        ScanDirectoryIfExists(mod.rootPath, fileProcessor, { TEX, TABLE, PLUGIN });

        s_mods.emplace(mod.id, std::move(mod));
    }


    std::vector<std::string> sortedIds;
    for (const auto& [id, _] : s_mods) sortedIds.push_back(id);
    std::sort(sortedIds.begin(), sortedIds.end());

    std::map<std::string, std::vector<std::string>> collisionTracker;

    for (const auto& modId : sortedIds) {
        const auto& mod = s_mods.at(modId);

        if (!mod.archivePath.empty()) {
            g_patched_archive_mods.push_back({ mod.id, "", mod.archivePath.string() });
        }

        for (const auto& item : mod.potentialScripts) {
            s_resolvedScriptsList.push_back(item);
        }

        for (const auto& [name, path] : mod.potentialTextures) {
            std::string lowerName = ToLower(name); 
            if (s_resolvedTexturesMap.count(lowerName)) collisionTracker[lowerName].push_back(modId);
            s_resolvedTexturesMap[lowerName] = path;
        }

        for (const auto& [name, path] : mod.potentialTables) {
            std::string lowerName = ToLower(name);
            if (s_resolvedTablesMap.count(lowerName)) collisionTracker[lowerName].push_back(modId);
            s_resolvedTablesMap[lowerName] = path;
        }

        for (const auto& [name, path] : mod.potentialTables) {
            std::string lowerName = ToLower(name);
            if (s_resolvedTablesMap.count(lowerName)) collisionTracker[lowerName].push_back(modId);
            s_resolvedTablesMap[lowerName] = path;
        }

        for (const auto& path : mod.potentialWeapons) {
            
            auto jdataRes = readJSON(path);
            if (!jdataRes) {
                Logger::Log(Error) << "Failed to read json for weapon: " << path;
                continue;
            }
            json jdata = *jdataRes;
            resolvedWeaponsList.push_back(jdata);
        }

    }

    for (const auto& [file, mods] : collisionTracker) {
        if (mods.size() > 1) {
            Logger::Log(Verbose) << "Conflict '" << file << "'. Winner: [" << mods.back() << "]";
        }
    }

    Logger::Log(Info) << "Mod Scan Complete. Loaded " << sortedIds.size() << " mods.";
}

void LoadPlugins() {
    if (!Settings::Instance().EnablePlugins) return;

    // Collect plugins while holding the lock
    std::vector<std::pair<std::string, std::filesystem::path>> pluginsToLoad;
    {
        std::lock_guard<std::mutex> lock(s_stateMutex);

        std::vector<std::string> sortedIds;
        for (const auto& [id, _] : s_mods) sortedIds.push_back(id);
        std::sort(sortedIds.begin(), sortedIds.end());

        for (const auto& modId : sortedIds) {
            const auto& mod = s_mods.at(modId);
            for (const auto& dllPath : mod.plugins) {
                pluginsToLoad.push_back({ modId, dllPath });
            }
        }
    } 

    // Load plugins without the lock
    // This allows plugins to call API functions (like GetModDirectory) 
    // which reacquire s_stateMutex without causing a deadlock
    for (const auto& [modId, dllPath] : pluginsToLoad) {
        HMODULE hModule = LoadLibraryW(dllPath.c_str());
        if (hModule) {
            Logger::Log(Verbose) << "Loaded Plugin: " << dllPath.filename().string();
            API::InitializePlugin(modId, hModule);
        }
        else {
            Logger::Log(Error) << "Failed to load plugin: " << dllPath.string();
        }
    }
}

void* LoadLooseTexture(const char* relativePath, size_t& out_size) {
    return LoadLooseInternal(ToLower(NormalizePath(relativePath)), s_resolvedTexturesMap, out_size);
}

void* LoadLooseTable(const char* relativePath, size_t& out_size) {
    return LoadLooseInternal(ToLower(NormalizePath(relativePath)), s_resolvedTablesMap, out_size);
}

std::vector<nlohmann::json> GetCustomWeapons() {
    return resolvedWeaponsList;
}

std::vector<std::vector<char>> GetInjectionScripts(const std::string& injectionPoint) {
    std::vector<std::vector<char>> files;
    std::string targetName = injectionPoint + ".lua";

    for (const auto& [name, path] : s_resolvedScriptsList) {
        if (name == targetName) {
            std::ifstream f(path, std::ios::binary | std::ios::ate);
            if (f) {
                size_t size = f.tellg();
                std::vector<char> buf(size);
                f.seekg(0);
                f.read(buf.data(), size);
                files.push_back(std::move(buf));
            }
        }
    }
    return files;
}

std::optional<std::string> GetModPath(const std::string& mod_id) {
    std::lock_guard<std::mutex> lock(s_stateMutex);
    auto it = s_mods.find(mod_id);
    if (it != s_mods.end()) return it->second.rootPath.string();
    return std::nullopt;
}