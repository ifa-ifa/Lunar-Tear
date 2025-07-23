#pragma once
#include <map>
#include <string>
#include <mutex>



void ScanModsAndResolveConflicts();
void* LoadLooseFile(const char* filename, size_t& out_size);