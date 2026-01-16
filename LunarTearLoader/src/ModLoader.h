#pragma once
#include <vector>
#include <string>
#include <optional>
#include <filesystem>
#include <replicant/weapon.h>
#include <nlohmann/json.hpp>


void ScanModsAndResolveConflicts();
void LoadPlugins();

void* LoadLooseTable(const char* relativePath, size_t& out_size);
void* LoadLooseTexture(const char* relativePath, size_t& out_size);
std::vector<std::vector<char>> GetInjectionScripts(const std::string& injectionPoint);
std::vector<nlohmann::json> GetCustomWeapons();

std::optional<std::string> GetModPath(const std::string& mod_id);