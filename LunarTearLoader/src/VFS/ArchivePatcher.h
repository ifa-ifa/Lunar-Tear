#pragma once
#include <thread>
#include <vector>
#include <string>
#include <mutex>

struct ModArchive {
    std::string id;
    std::string archivePath;
    std::string indexPath;
};

// Global list of mods that provide an info.arc, populated by the mod scanner.
extern std::vector<ModArchive> g_patched_archive_mods;

// Starts the background thread that performs the archive patching
void StartArchivePatchingThread();

// Signals the patching thread to stop, if it's running
void StopArchivePatchingThread();