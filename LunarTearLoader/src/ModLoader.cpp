#include "ModLoader.h"
#include "Common/Logger.h"
#include "Common/Settings.h"
#include <set>
#include <vector>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <iterator>

using enum Logger::LogCategory;

namespace {
    struct CachedFile {
        std::vector<char> data;
        std::chrono::steady_clock::time_point lastAccessTime;
    };

    static std::map<std::string, std::string> s_resolvedFileMap;
    static std::mutex s_fileMapMutex;
    static std::map<std::string, CachedFile> s_fileCache;
    static std::mutex s_cacheMutex;

    static std::thread s_cleanupThread;
    static std::atomic<bool> s_stopCleanup(false);

    void CacheCleanupRoutine() {
        Logger::Log(Info) << "Texture cache cleanup thread started.";
        while (!s_stopCleanup) {
            std::this_thread::sleep_for(std::chrono::seconds(3));

            const auto now = std::chrono::steady_clock::now();
            const auto unload_delay = std::chrono::seconds(Settings::Instance().TextureUnloadDelaySeconds);

            std::lock_guard<std::mutex> lock(s_cacheMutex);

            float size = 0;

            for (auto it = s_fileCache.begin(); it != s_fileCache.end();) {

                size += it->second.data.size();

                // Only evict textures
                std::filesystem::path p(it->first);
                if (p.extension() == ".dds") {
                    auto elapsed = now - it->second.lastAccessTime;
                    if (elapsed > unload_delay) {
                        Logger::Log(Verbose) << "Unloading texture from cache: " << it->first;
                        it = s_fileCache.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
                else {
                    ++it;
                }
            }

            Logger::Log(Verbose) << "Cache usage: " << size / 1e6 << "MB";
        }
        Logger::Log(Info) << "Texture cache cleanup thread stopped.";
    }
}


void ScanModsAndResolveConflicts() {
    const std::string mods_root = "LunarTear/mods/";
    if (!std::filesystem::exists(mods_root) || !std::filesystem::is_directory(mods_root)) {
        Logger::Log(Info) << "Mods directory not found, skipping mod scan";
        return;
    }


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

    const std::set<std::string> relevant_extensions = { ".dds", ".lua", ".settbll", ".settb"};

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

    // Use find to avoid double lookup (count + at)
    auto cache_it = s_fileCache.find(full_path);
    if (cache_it != s_fileCache.end()) {
        auto& cachedEntry = cache_it->second;
        cachedEntry.lastAccessTime = std::chrono::steady_clock::now();
        out_size = cachedEntry.data.size();
        return cachedEntry.data.data();
    }

    // File not in cache, load it.
    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) {
        Logger::Log(Error) << "Error: Could not open resolved file: " << full_path;
        out_size = 0;
        return nullptr;
    }

    // Construct the CachedFile object with the file data and current time
    CachedFile newFile{
        std::vector<char>(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        ),
        std::chrono::steady_clock::now()
    };

    // Use emplace to insert the new entry and get an iterator to it.
    auto [inserted_it, success] = s_fileCache.emplace(std::move(full_path), std::move(newFile));

    auto& newEntry = inserted_it->second;
    out_size = newEntry.data.size();
    return newEntry.data.data();
}

void StartCacheCleanupThread() {
    if (Settings::Instance().TextureUnloading) {
        s_stopCleanup = false;
        s_cleanupThread = std::thread(CacheCleanupRoutine);
    }
}

void StopCacheCleanupThread() {
    if (s_cleanupThread.joinable()) {
        s_stopCleanup = true;
        s_cleanupThread.join();
    }
}