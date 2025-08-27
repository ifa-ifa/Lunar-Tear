#pragma once
#include <Windows.h>
#include <map>
#include <string>
#include <optional>
#include <mutex>
#include <vector>
#include <chrono>


extern bool VFS_ready;

void ScanModsAndResolveConflicts();
void* LoadLooseFile(const char* filename, size_t& out_size);

std::optional<std::string> GetModPath(const std::string& mod_id);
std::vector<std::vector<char>> GetInjectionScripts(const std::string& injectionPoint);

void StartCacheCleanupThread();
void StopCacheCleanupThread();

void LoadPlugins();