#pragma once
#include <Windows.h>
#include <map>
#include <string>
#include <mutex>
#include <vector>


void ScanModsAndResolveConflicts();
void* LoadLooseFile(const char* filename, size_t& out_size);

std::vector<std::vector<char>> GetInjectionScripts(const std::string& injectionPoint);

void StartCacheCleanupThread();
void StopCacheCleanupThread();

void LoadPlugins();