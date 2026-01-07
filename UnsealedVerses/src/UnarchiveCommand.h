#include <future>
#include <mutex>

class UnarchiveCommand : public Command {
    std::mutex console_mutex; 

	bool chara_only = false;

public:
    UnarchiveCommand(std::vector<std::string> args) : Command(std::move(args)) {}

    int execute() override {
        if (m_args.size() != 2 && m_args.size() != 3) {
            std::cerr << "Error: unarchive mode requires <info.arc> <output_folder> [options]\n";
            return 1;
        }
        const std::filesystem::path index_path(m_args[0]);
        const std::filesystem::path output_folder_base(m_args[1]);
        const std::filesystem::path archive_dir = index_path.parent_path();

    
        if (m_args.size() == 3) {
            if (m_args[2] == "--chara-only") {
                chara_only = true;
            }
            else {
                std::cerr << "Error: Unknown option '" << m_args[2] << "'\n";
                return 1;
            }

        }

        std::cout << "Unpacking archives using index: " << index_path << "\n";

        auto compressed_index = unwrap(replicant::ReadFile(index_path), "Failed to read index");
        auto dSize = unwrap(replicant::archive::GetDecompressedSize(compressed_index), "Bad index ZSTD");
        auto bxon_data = unwrap(replicant::archive::Decompress(compressed_index, dSize), "Index decompress failed");

        auto [info, payload] = unwrap(replicant::ParseBxon(bxon_data), "Index BXON parse failed");
        if (info.assetType != "tpArchiveFileParam") throw std::runtime_error("Invalid asset type");

        auto params = unwrap(replicant::TpArchiveFileParam::Deserialize(payload), "Param deserialize failed");

        std::vector<std::vector<const replicant::FileEntry*>> files_per_archive(params.archiveEntries.size());

        for (const auto& file : params.fileEntries) {
            if (chara_only && file.name.starts_with("chara/") == false) {
                continue;
			}
            if (file.archiveIndex < files_per_archive.size()) {
                files_per_archive[file.archiveIndex].push_back(&file);
            }
        }

        std::vector<std::future<void>> futures;

        for (size_t i = 0; i < params.archiveEntries.size(); ++i) {
            const auto& arc_entry = params.archiveEntries[i];
            const auto& file_list = files_per_archive[i];

            if (file_list.empty()) continue;

            futures.push_back(std::async(std::launch::async,
                [this,
                i,
                arc_entry = std::cref(arc_entry),
                file_list = std::cref(file_list),
                archive_dir,
                output_folder_base]()
                {
                    try {
                        processArchive(
                            i,
                            arc_entry.get(),
                            file_list.get(),
                            archive_dir,
                            output_folder_base
                        );
                    }
                    catch (const std::exception& e) {
                        std::lock_guard<std::mutex> lock(console_mutex);
                        std::cerr << "Error in archive '"
                            << arc_entry.get().filename
                            << "': " << e.what() << "\n";
                    }
                }
            ));
        }

        for (auto& f : futures) {
            f.wait();
        }

        std::cout << "\nUnpacking complete.\n";
        return 0;
    }

private:
    void processArchive(
        size_t index,
        const replicant::ArchiveEntry& arc_meta,
        const std::vector<const replicant::FileEntry*>& files,
        const std::filesystem::path& arc_dir,
        const std::filesystem::path& out_base
    ) {
        std::filesystem::path arc_path = arc_dir / arc_meta.filename;

        {
            std::lock_guard<std::mutex> lock(console_mutex);
            std::cout << "Processing " << arc_meta.filename << " (" << files.size() << " files)...\n";
        }

        // PRELOAD 
        if (arc_meta.loadType == replicant::ArchiveLoadType::PRELOAD_DECOMPRESS) {
            auto arc_data = unwrap(replicant::ReadFile(arc_path), "Failed to read " + arc_meta.filename);

            // Decompress the ENTIRE archive to memory - the largest vanilla preload arc is 9MB
            auto dSize = unwrap(replicant::archive::GetDecompressedSize(arc_data), "Bad ZSTD in " + arc_meta.filename);
            auto blob = unwrap(replicant::archive::Decompress(arc_data, dSize), "Decompress failed " + arc_meta.filename);

            for (const auto* file : files) {
                
                std::filesystem::path out_path = out_base / file->name;

                // For Type 0: rawOffset is offset into DECOMPRESSED blob
                // size is the uncompressed size
                if (file->rawOffset + file->size > blob.size()) {
                    throw std::runtime_error("File " + file->name + " out of bounds in " + arc_meta.filename);
                }

                std::span<const std::byte> file_span(blob.data() + file->rawOffset, file->size);
                unwrap(replicant::WriteFile(out_path, file_span), "Write failed " + file->name);
            }
        }
        // STREAM
        else {
            std::ifstream arc_stream(arc_path, std::ios::binary);
            if (!arc_stream) throw std::runtime_error("Could not open " + arc_path.string());

            std::vector<std::byte> compressed_buf;

            for (const auto* file : files) {

                std::filesystem::path out_path = out_base / file->name;

                arc_stream.seekg(file->rawOffset);
                if (!arc_stream) throw std::runtime_error("Seek failed for " + file->name);

                if (compressed_buf.size() < file->size) {
                    compressed_buf.resize(file->size);
                }

                if (!arc_stream.read(reinterpret_cast<char*>(compressed_buf.data()), file->size)) {
                    throw std::runtime_error("Read failed for " + file->name);
                }

                std::span<const std::byte> chunk_span(compressed_buf.data(), file->size);

                auto dSize = unwrap(replicant::archive::GetDecompressedSize(chunk_span), "Bad ZSTD for " + file->name);
                auto decompressed = unwrap(replicant::archive::Decompress(chunk_span, dSize), "Decompress failed " + file->name);

                unwrap(replicant::WriteFile(out_path, decompressed), "Write failed " + file->name);
            }
        }

        {
            std::lock_guard<std::mutex> lock(console_mutex);
            std::cout << "Finished " << arc_meta.filename << ".\n";
        }
    }
};