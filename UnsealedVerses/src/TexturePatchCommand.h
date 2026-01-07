#pragma once
#include "Common.h"
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include "replicant/core/io.h"
#include "replicant/dds.h"
#include "replicant/bxon.h"


class TexturePatchCommand : public Command {
public:
    TexturePatchCommand(std::vector<std::string> args) : Command(std::move(args)) {}
    int execute() override {
        if (m_args.size() != 3) {
            std::cerr << "Error: texture-patch requires <output.xap> <input.xap> <dds_folder>\n";
            return 1;
        }
        const std::filesystem::path output_path(m_args[0]);
        const std::filesystem::path input_path(m_args[1]);
        const std::filesystem::path dds_folder_path(m_args[2]);

        std::cout << "Patching textures in PACK file\n";
        std::cout << "Input PACK:     " << input_path << "\n";
        std::cout << "DDS Folder:     " << dds_folder_path << "\n";
        std::cout << "Output PACK:    " << output_path << "\n\n";

        if (!std::filesystem::is_directory(dds_folder_path)) {
            std::cerr << "Error: Provided patch path is not a directory.\n";
            return 1;
        }

        auto pack_data = unwrap(replicant::ReadFile(input_path), "Failed to read input PACK file");
        auto pack = unwrap(replicant::Pack::Deserialize(pack_data), "Failed to parse PACK file");

        int patch_count = 0;
        for (const auto& dir_entry : std::filesystem::recursive_directory_iterator(dds_folder_path)) {
            if (dir_entry.is_regular_file() && dir_entry.path().extension() == ".dds") {
                const auto& dds_path = dir_entry.path();
                // Match dds filename (e.g., "tex_abc") to pack entry name (e.g., "tex_abc.rtex")
                std::string entry_name = std::filesystem::relative(dds_path, dds_folder_path).replace_extension(".rtex").generic_string();

                replicant::PackFileEntry* entry_to_patch = pack.findFile(entry_name);

                if (entry_to_patch) {
                    std::cout << "Patching '" << entry_name << "' with '" << dds_path.string() << "'...\n";

                    auto dds_file = unwrap(replicant::dds::DDSFile::Load(dds_path), "Failed to load patch DDS " + dds_path.string());

                    auto [header, pixel_data] = dds_file.ToGameFormat();

                    auto raw_tex_header_res = replicant::SerializeTexHead(header);
                    if (!raw_tex_header_res) {
                        std::cerr << "Error: Failed to serialize texture header for " << entry_name << ": " << raw_tex_header_res.error().toString() << "\n";
                        continue;
					}
					std::vector<std::byte> raw_tex_header = *raw_tex_header_res;

                    auto bxon_raw = replicant::BuildBxon(
                        "tpGxTexHead",
                        3,
                        782713094,
                        raw_tex_header
                    );
                    if (!bxon_raw) {
                        std::cerr << "Error: Failed to build BXON for " << entry_name << ": " << bxon_raw.error().toString() << "\n";
                        continue;
					}
					entry_to_patch->serializedData = *bxon_raw;

                    entry_to_patch->resourceData = std::move(pixel_data);

                    std::cout << "  + Successfully patched.\n";
                    patch_count++;
                }
            }
        }

        if (patch_count == 0) {
            std::cout << "\nNo matching textures found to patch. Output file will be identical to input.\n";
        }

        std::cout << "\nRebuilding PACK file...\n";
        auto rebuilt_pack_data_res = pack.Serialize();
        if (!rebuilt_pack_data_res) {
            std::cerr << "Error: Failed to serialize rebuilt PACK file: " << rebuilt_pack_data_res.error().toString() << "\n";
            return 1;
		}
		std::vector<std::byte> rebuilt_pack_data = *rebuilt_pack_data_res;
        unwrap(replicant::WriteFile(output_path, rebuilt_pack_data), "Failed to write rebuilt PACK file");
        return 0;
    }
};