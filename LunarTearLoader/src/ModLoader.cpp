#include "ModLoader.h"
#include "Common/Logger.h"
#include "Common/Settings.h"
#include "Lua/LuaCommandQueue.h"
#include "API/api.h"
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

    // Key: Injection point e.g "post_phase/B_SOUTH_FIELD_01.lua"
    // Value: List of full paths to the scripts from different mods
    static std::map<std::string, std::vector<std::string>> s_injectionScripts;
    static std::mutex s_injectionMutex;


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
    std::map<std::string, std::vector<std::string>> temp_injection_scripts;

    std::vector<std::string> mod_names;
    for (const auto& mod_entry : std::filesystem::directory_iterator(mods_root)) {
        if (mod_entry.is_directory()) {
            mod_names.push_back(mod_entry.path().filename().string());
        }
    }
    // Sort mods alphabetically to ensure a consistent load order for injections
    std::sort(mod_names.begin(), mod_names.end());

    for (const auto& mod_name : mod_names) {
        std::filesystem::path mod_path = std::filesystem::path(mods_root) / mod_name;
        Logger::Log(Verbose) << "Scanning mod: " << mod_name;

        if (!std::filesystem::exists(mod_path) || !std::filesystem::is_directory(mod_path)) continue;

        for (const auto& file_entry : std::filesystem::recursive_directory_iterator(mod_path)) {
            if (!file_entry.is_regular_file()) continue;

            std::string relative_path_str = std::filesystem::relative(file_entry.path(), mod_path).string();
            std::replace(relative_path_str.begin(), relative_path_str.end(), '\\', '/');

            if (relative_path_str.starts_with("inject/")) {
                std::string injection_point = relative_path_str.substr(7); // 7 is length of "inject/"
                temp_injection_scripts[injection_point].push_back(file_entry.path().string());
            }
            else {
                file_to_mods_map[relative_path_str].push_back(mod_name);
            }
        }
    }

    {
        const std::lock_guard<std::mutex> lock(s_injectionMutex);
        s_injectionScripts = std::move(temp_injection_scripts);
        for (const auto& [point, paths] : s_injectionScripts) {
            Logger::Log(Verbose) << "Found " << paths.size() << " injection script(s) for point '" << point << "'";
        }
    }


    const std::set<std::string> relevant_extensions = { ".dds", ".lua", ".settbll", ".settb" };

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

std::vector<std::vector<char>> GetInjectionScripts(const std::string& injectionPoint) {
    std::vector<std::vector<char>> scripts_data;

    std::vector<std::string> script_paths;
    {
        const std::lock_guard<std::mutex> lock(s_injectionMutex);
        auto it = s_injectionScripts.find(injectionPoint);
        if (it == s_injectionScripts.end()) {
            return scripts_data;
        }
        script_paths = it->second;
    }

    Logger::Log(Verbose) << "Loading " << script_paths.size() << " scripts for injection point: " << injectionPoint;

    for (const auto& path : script_paths) {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            scripts_data.emplace_back(
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()
            );
        }
        else {
            Logger::Log(Error) << "Could not open injection script for reading: " << path;
        }
    }

    return scripts_data;
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


    {
        const std::lock_guard<std::mutex> lock(s_cacheMutex);
        auto cache_it = s_fileCache.find(full_path);
        if (cache_it != s_fileCache.end()) {
            auto& cachedEntry = cache_it->second;
            cachedEntry.lastAccessTime = std::chrono::steady_clock::now();
            out_size = cachedEntry.data.size();
            return cachedEntry.data.data();
        }
    } 

    // Slow, but it doesnt block other threads from accessing the cache
    std::vector<char> fileData;
    {
        std::ifstream file(full_path, std::ios::binary);
        if (!file.is_open()) {
            Logger::Log(Error) << "Error: Could not open resolved file: " << full_path;
            out_size = 0;
            return nullptr;
        }
        fileData.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }


    {
        const std::lock_guard<std::mutex> lock(s_cacheMutex);

        auto cache_it = s_fileCache.find(full_path);
        if (cache_it != s_fileCache.end()) {
            // Another thread beat us to it. Discard our data and use the existing entry.
            auto& cachedEntry = cache_it->second;
            cachedEntry.lastAccessTime = std::chrono::steady_clock::now();
            out_size = cachedEntry.data.size();
            return cachedEntry.data.data();
        }

        CachedFile newFile{
            std::move(fileData), // Move our local data into the cache to avoid a copy
            std::chrono::steady_clock::now()
        };

        auto [inserted_it, success] = s_fileCache.emplace(std::move(full_path), std::move(newFile));

        auto& newEntry = inserted_it->second;
        out_size = newEntry.data.size();
        return newEntry.data.data();
    }
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

void LoadPlugins() {
    const std::string mods_root = "LunarTear/mods/";
    if (!std::filesystem::exists(mods_root) || !std::filesystem::is_directory(mods_root)) {
        return;
    }

    Logger::Log(Info) << "Scanning for plugins...";

    for (const auto& mod_entry : std::filesystem::directory_iterator(mods_root)) {
        if (!mod_entry.is_directory()) continue;

        std::string mod_name = mod_entry.path().filename().string();

        for (const auto& file_entry : std::filesystem::directory_iterator(mod_entry.path())) {
            if (file_entry.is_regular_file() && file_entry.path().extension() == ".dll") {
                std::string plugin_path_str = file_entry.path().string();
                Logger::Log(Info) << "Attempting to load plugin: " << plugin_path_str;

                HMODULE plugin_module = LoadLibraryA(plugin_path_str.c_str());
                if (plugin_module) {
                    Logger::Log(Info) << "Successfully loaded plugin: " << plugin_path_str;
                    API::InitializePlugin(mod_name, plugin_module);
                }
                else {
                    DWORD error = GetLastError();
                    LPSTR messageBuffer = nullptr;
                    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
                    std::string message(messageBuffer, size);
                    LocalFree(messageBuffer);

                    Logger::Log(Error) << "Failed to load plugin '" << plugin_path_str << "'. Error " << error << ": " << message;
                }
            }
        }
    }
    Logger::Log(Info) << "Plugin scan complete.";
} 