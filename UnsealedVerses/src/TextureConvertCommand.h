
class TextureConvertCommand : public Command {
public:
    TextureConvertCommand(std::vector<std::string> args) : Command(std::move(args)) {}
    int execute() override {
        if (m_args.size() != 2) {
            std::cerr << "Error: texture-convert requires <input> <output>\n";
            std::cerr << "It will auto-detect conversion direction (.dds -> .rtex or .rtex -> .dds)\n";
            return 1;
        }
        const std::filesystem::path input_path(m_args[0]);
        const std::filesystem::path output_path(m_args[1]);

        if (input_path.extension() == ".dds") {
            return to_rtex(input_path, output_path);
        }
        else if (input_path.extension() == ".rtex") {
            return to_dds(input_path, output_path);
        }
        else {
            std::cerr << "Error: Input file must be .dds or .rtex\n";
            return 1;
        }
    }
private:
    int to_rtex(const std::filesystem::path& input_path, const std::filesystem::path& output_path) {
        std::cout << "Converting DDS to BXON Texture (.rtex)\n";
        std::cout << "Input:  " << input_path << "\n";
        std::cout << "Output: " << output_path << "\n";

        auto dds_file = unwrap(replicant::dds::DDSFile::Load(input_path), "Failed to load DDS file");
        auto [header, pixel_data] = dds_file.ToGameFormat();

        std::vector<std::byte> serialized_header = unwrap(replicant::SerializeTexHead(header), "Failed to serialise new tex header");

        std::vector<std::byte> payload;
        payload.insert(payload.end(), serialized_header.begin(), serialized_header.end());
        payload.insert(payload.end(), pixel_data.begin(), pixel_data.end());

        std::vector<std::byte> bxon_data = unwrap(replicant::BuildBxon("tpGxTexHead", 3, 0x2ea74106, payload), "Failed to build new rtex bxon");
        unwrap(replicant::WriteFile(output_path, bxon_data), "Failed to write output .rtex file");

        std::cout << "Successfully created " << output_path << "\n";
        return 0;
    }

    int to_dds(const std::filesystem::path& input_path, const std::filesystem::path& output_path) {
        std::cout << "Converting BXON Texture (.rtex) to DDS\n";
        std::cout << "Input:  " << input_path << "\n";
        std::cout << "Output: " << output_path << "\n";

        auto rtex_data = unwrap(replicant::ReadFile(input_path), "Failed to read .rtex file");
        auto [bxon_info, payload] = unwrap(replicant::ParseBxon(rtex_data), "Failed to parse BXON container");

        if (bxon_info.assetType != "tpGxTexHead") {
            std::cerr << "Error: Input file is not an rtex file.\n"; return 1;
        }

        auto header = unwrap(replicant::DeserializeTexHead(payload), "Failed to parse texture header");

        size_t header_size = unwrap(replicant::SerializeTexHead(header), "Failed to get Tex head size").size();
        std::span<const std::byte> pixel_data = payload.subspan(header_size);

        auto dds_file = unwrap(replicant::dds::DDSFile::CreateFromGameData(header, pixel_data), "Failed to create DDS from game data");
        unwrap(dds_file.Save(output_path), "Failed to save DDS file");

        std::cout << "Successfully converted to " << output_path << "\n";
        return 0;
    }
};
