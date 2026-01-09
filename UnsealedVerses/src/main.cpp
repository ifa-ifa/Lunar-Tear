#include <iostream>
#include <vector>
#include <string>

#include "Common.h"
#include "FindEntryCommand.h"
#include "UnpackCommand.h"
#include "TexturePatchCommand.h"
#include "ArchiveCommand.h"
#include "UnarchiveCommand.h"
#include "TextureConvertCommand.h"
#include "CreateWeaponAsset.h"

void printUsage() {
    std::cout << "UnsealedVerses - A tool for editing Nier Replicant files.\n\n";
    std::cout << "Usage: UnsealedVerses <command> [options] <args...>\n\n";
    std::cout << "COMMANDS:\n\n";
    std::cout << "  archive <output.arc> <assets_path> [options]\n";
    std::cout << "    Builds a game archive from specified directory.\n";
    std::cout << "    Options:\n";
    std::cout << "      --index <path>      Generate a new compressed index file (e.g., info.arc).\n";
    std::cout << "      --patch <path>      Patch an existing compressed index file.\n";
    std::cout << "      --out <path>        Output path for the patched index. Overwrites original if not set.\n";
    std::cout << "      --load-type <0|1|2> Set load type (0: Preload, 1: Stream, 2: StreamOnDemand).\n\n";
    std::cout << "  unarchive <info.arc> <output_folder> [options]\n";
    std::cout << "    Extracts all files from archives referenced by the given index\n";
    std::cout << "    Options:\n";
    std::cout << "      --chara-only      Extract only the chara folder (character models and weapons) (saves ~20GB)\n\n";
    std::cout << "  texture-convert <input> <output>\n";
    std::cout << "    Converts a standalone texture between .dds and .rtex\n";
    std::cout << "    Note that here an rtex texture file is considered a header and pixel data concatenated\n\n";
    std::cout << "  unpack <input.xap> <output_folder>\n";
    std::cout << "    Extracts all files from a PACK file (.xap) into a specified folder.\n";
    std::cout << "    Note that this will append the resource data immediately after the serialised data\n\n";
    std::cout << "  texture-patch <out.xap> <in.xap> <dds_folder>\n";
    std::cout << "    Patches textures in a PACK file (.xap) using dds textures from a folder.\n";
    std::cout << "    The new files should have the same name as the ones being replcaed,\n";
    std::cout << "    just with a dds file extension instead of an rtex.\n\n";
    std::cout << "  find-entry <search_folder> <entry_name>\n";
    std::cout << "    Recursively searches a directory for PACK files containing an entry with the given name.\n\n";
    std::cout << "  create-weapon-asset <assets_local_mesh_path> <output_weapon_asset>\n";
    std::cout << "    Creates a weapon asset. The game will search for this to find the mesh for a weapon\n";
    std::cout << "    output_weapon_asset can be whatever you want but the last letter should be the level\n";
    std::cout << "    (end with 0 for level 1, end with 1 for level 2, etc.).\n";
    std::cout << "    assets_local_mesh_path should be local to game vfs.\n";
    std::cout << "    Your mesh can be wherever you want, i reccomend something like 'chara/weapon/[modname]/[weaponName]'.\n";
    std::cout << "    You will need ones of these weapon assets for every level, however they can all point to the same mesh.\n";

}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string command_name = argv[1];
    std::vector<std::string> command_args(argv + 2, argv + argc);
    std::unique_ptr<Command> command;

    if (command_name == "archive") {
        command = std::make_unique<ArchiveCommand>(command_args);
    }
    else if (command_name == "unarchive") {
        command = std::make_unique<UnarchiveCommand>(command_args);
    }
    else if (command_name == "texture-convert") {
        command = std::make_unique<TextureConvertCommand>(command_args);
    }
    else if (command_name == "texture-patch") {
        command = std::make_unique<TexturePatchCommand>(command_args);
    }
    else if (command_name == "find-entry") {
        command = std::make_unique<FindEntryCommand>(command_args);
    }
    else if (command_name == "unpack") {
        command = std::make_unique<UnpackCommand>(command_args);
    }
    else if (command_name == "create-weapon-asset") {
        command = std::make_unique<CreateWeaponAsset>(command_args);
    }
    else {
        std::cerr << "Error: Unknown command '" << command_name << "'\n\n";
        printUsage();
        return 1;
    }

    try {
        return command->execute();
    }
    catch (const std::exception& e) {
        std::cerr << "\nAn unexpected and fatal error occurred:\n" << e.what() << "\n";
        return 1;
    }
}