#include "replicant/kpk.h"

class UnpackKPKCommand : public Command {
public:
    UnpackKPKCommand(std::vector<std::string> args) : Command(std::move(args)) {}
    int execute() override {
        if (m_args.size() != 2) {
            std::cout << "Error: unpack KPK mode requires <input> <output_folder>\n";
            return 1;
        }
        const std::filesystem::path input_path(m_args[0]);
        const std::filesystem::path output_folder_path(m_args[1]);

        std::cout << "Unpacking KPK file\n";
        std::cout << "Input:  " << input_path << "\n";
        std::cout << "Output: " << output_folder_path << "\n\n";

        std::filesystem::create_directories(output_folder_path);

        auto kpk_data = unwrap(replicant::ReadFile(input_path), "Failed to read KPK file");

        auto kpk = unwrap(replicant::KpkFile::Deserialize(kpk_data), "Failed to parse KPK file");

        for (const auto& entry : kpk.entries) {
            std::filesystem::path out_path = output_folder_path / entry.name;
            std::filesystem::create_directories(out_path.parent_path());

            unwrap(replicant::WriteFile(out_path, entry.data), "Failed to write output file " + entry.name);
            std::cout << "Extracted: " << entry.name << "\n";
        }

        return 0;
    }
};