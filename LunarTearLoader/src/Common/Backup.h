#pragma once
#include <filesystem>
#include <string>
#include <chrono>
#include <windows.h>
#include <shlobj.h>
#include <format> 
#include <vector>
#include <algorithm>
#include "Common/Settings.h"
#include "Common/Logger.h"

inline void CreateSaveBackups() {

    PWSTR widePath = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &widePath);

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to get Documents directory");
    }
    std::filesystem::path documentsPath = widePath;
    CoTaskMemFree(widePath);

	auto saveDir = 
            std::filesystem::path(documentsPath) /
            "My Games/NieR Replicant ver.1.22474487139/Steam/";


    if (!std::filesystem::exists(saveDir)) return;

    for (const std::filesystem::directory_entry& steamIdEntry : std::filesystem::directory_iterator(saveDir)) {

        if (!steamIdEntry.is_directory()) continue;

        if (!std::filesystem::exists(steamIdEntry.path() / "GAMEDATA")) {
            Logger::Log(Logger::LogCategory::Verbose) << "No save file found in: " << steamIdEntry.path().string();
			continue;
        }

        auto timestamp = std::format("{:%Y-%m-%d_%H-%M-%S}", std::chrono::system_clock::now());

        auto steamIdBackupDir = std::filesystem::path("LunarTear/backups") / steamIdEntry.path().filename();
        auto backupPath = steamIdBackupDir / ("GAMEDATA_" + timestamp);

        std::filesystem::create_directories(backupPath.parent_path());

        try {
            std::filesystem::copy_file(steamIdEntry.path() / "GAMEDATA", backupPath);
        }
        catch (const std::exception& e) {
            Logger::Log(Logger::LogCategory::Error) << "Failed to back up save file: " << e.what();
            continue;
		}

        Logger::Log(Logger::LogCategory::Info) << "Backed up save to: " << backupPath.string();
        
        
        int maxBackups = Settings::Instance().maxBackups;

        // If maxBackups is 0 or less treat as unlimited to prevent accidental wipe
        if (maxBackups > 0) {

            std::vector<std::filesystem::path> backupFiles;

            for (const auto& entry : std::filesystem::directory_iterator(steamIdBackupDir)) {
                if (entry.is_symlink()) continue;
                if (entry.is_regular_file()) {
                    std::string fname = entry.path().filename().string();

                    if (fname.starts_with("GAMEDATA_")) {
                        backupFiles.push_back(entry.path());
                    }
                }
            }

            if (backupFiles.size() > (size_t)maxBackups) {

                std::sort(backupFiles.begin(), backupFiles.end());

                size_t filesToDelete = backupFiles.size() - maxBackups;

                for (size_t i = 0; i < filesToDelete; ++i) {
                    try {

                        auto safeBase = std::filesystem::canonical(steamIdBackupDir);
                        auto target = std::filesystem::canonical(backupFiles[i]);

                        std::wstring baseStr = safeBase.native();
                        if (!baseStr.empty() && baseStr.back() != std::filesystem::path::preferred_separator) {
                            baseStr += std::filesystem::path::preferred_separator;
                        }
                        if (!target.native().starts_with(baseStr)) {
                            Logger::Log(Logger::LogCategory::Warning)
                                << "Refusing to delete file resolving outside backup directory: "
                                << target.string();
                            continue;
                        }

                        std::filesystem::remove(backupFiles[i]);
                        Logger::Log(Logger::LogCategory::Verbose) << "Deleted old backup " << backupFiles[i].filename().string();
                    }
                    catch (const std::exception& e) {
                        Logger::Log(Logger::LogCategory::Warning) << "Failed to delete old backup: " << e.what();
                    }
                }
            }
        }
    }
}