class UnpackCommand : public Command {
public:
    UnpackCommand(std::vector<std::string> args) : Command(std::move(args)) {}
    int execute() override {
        if (m_args.size() != 2) {
            std::cerr << "Error: unpack mode requires <input.xap> <output_folder>\n";
            return 1;
        }
        const std::filesystem::path input_path(m_args[0]);
        const std::filesystem::path output_folder_path(m_args[1]);

        std::cout << "Unpacking PACK file\n";
        std::cout << "Input:  " << input_path << "\n";
        std::cout << "Output: " << output_folder_path << "\n\n";

        std::filesystem::create_directories(output_folder_path);

        auto pack_data = unwrap(replicant::ReadFile(input_path), "Failed to read PACK file");

        auto pack = unwrap(replicant::Pack::Deserialize(pack_data), "Failed to parse PACK file");

        for (const auto& entry : pack.files) {
            std::filesystem::path out_path = output_folder_path / entry.name;
            std::filesystem::create_directories(out_path.parent_path());

            std::vector<std::byte> file_content = entry.serializedData;
            if (entry.hasResource()) {
                file_content.insert(file_content.end(), entry.resourceData.begin(), entry.resourceData.end());
            }

            unwrap(replicant::WriteFile(out_path, file_content), "Failed to write output file " + entry.name);
            std::cout << "Extracted: " << entry.name << "\n";
        }

        return 0;
    }
};