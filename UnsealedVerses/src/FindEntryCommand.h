#pragma once
#include "Common.h"
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include "replicant/core/io.h"
#include "replicant/pack.h"
#include "replicant/arc.h"

class FindEntryCommand : public Command {
public:
    FindEntryCommand(std::vector<std::string> args) : Command(std::move(args)) {}
    int execute() override {
        if (m_args.size() != 2) {
            std::cerr << "Error: find-entry mode requires <search_folder> <entry_name_to_find>\n";
            return 1;
        }

        const std::filesystem::path search_path(m_args[0]);
        const std::string entry_name = m_args[1];

        if (!std::filesystem::is_directory(search_path)) {
            std::cerr << "Error: Provided path is not a directory: " << search_path << "\n";
            return 1;
        }

        std::cout << "Searching for entry '" << entry_name << "' in directory '" << search_path.string() << "'...\n";
        int found_count = 0;

        for (const auto& dir_entry : std::filesystem::recursive_directory_iterator(search_path)) {
            if (dir_entry.is_regular_file()) {
                const auto& current_file_path = dir_entry.path();

                // TODO: Right now we have to parse the entire PACK file to check for the entry - a regression in the new libreplicant api


                auto pack_result = replicant::Pack::DeserializeNoResources(current_file_path);
                if (!pack_result) continue;

                if (pack_result->findFile(entry_name)) {
                    std::filesystem::path relative_path = std::filesystem::relative(current_file_path, search_path);
                    std::cout << "Found in: " << relative_path.generic_string() << "\n";
                    found_count++;
                }
            }
        }

        std::cout << "Search complete. Found " << found_count << " occurrence(s).\n";
        return 0;
    }
};
