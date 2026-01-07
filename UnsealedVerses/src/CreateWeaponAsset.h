#pragma once
#include "Common.h"
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include "replicant/core/io.h"
#include "replicant/pack.h"
#include "replicant/arc.h"
#include "replicant/tpXonAssetHeader.h"
#include "replicant/tpActorAssetParam.h"
#include "replicant/bxon.h"


class CreateWeaponAsset : public Command {
public:

	CreateWeaponAsset(std::vector<std::string> args) : Command(std::move(args)) {}

	int execute() override {
		if (m_args.size() != 2) {
			std::cerr << "Error: create-weapon-asset mode requires <assets_local_mesh_path> <output_weapon_asset>\n";
			return 1;
		}

		const std::filesystem::path mesh_path(m_args[0]);
		const std::filesystem::path output_path(m_args[1]);
		const std::string weapon_name = output_path.stem().string();

		replicant::Pack weapon_pack;

		weapon_pack.info.version = 4;
		weapon_pack.imports.push_back(replicant::ImportEntry{
			.pathHash = replicant::fnv1_32(mesh_path.string()), // TODO: I should not have to do this manually...
			.path = mesh_path.string()
			
			});


		replicant::AssetPackageEntry weapon_asset_package;
		weapon_asset_package.name = weapon_name + ".xap";
		weapon_asset_package.nameHash = replicant::fnv1_32(weapon_asset_package.name); // TODO: I should not have to do this manually

		replicant::XonAssetEntry weapon_xon_asset_entry;
		weapon_xon_asset_entry.assetTypeHash = 0x659FBAD7;

		replicant::ActorAssetEntry actor_asset_entry;
		actor_asset_entry.u32_00 = 1198150135; //idk what this is
		actor_asset_entry.assetPath = mesh_path.string();
		actor_asset_entry.u32_0c = 0;
		actor_asset_entry.u32_10 = 0;

		std::vector<std::byte> actor_asset_data = unwrap(
			replicant::buildActorAssetParam(std::span<const replicant::ActorAssetEntry>{ &actor_asset_entry, 1 }),
			 "Failed to build actor asset param"
		);

		weapon_xon_asset_entry.assetData = actor_asset_data;

		std::vector<std::byte> xon_asset_data = unwrap(
			replicant::buildXonAssetHeader(std::span<const replicant::XonAssetEntry>{ &weapon_xon_asset_entry, 1 }, {}),
			"Failed to build XON asset header"
		);

		auto bxon_data = unwrap(replicant::BuildBxon("tpXonAssetHeader", 3, 955984368, xon_asset_data), "Failed to build bxon");

		weapon_asset_package.content = bxon_data;

		weapon_pack.assetPackages.push_back(weapon_asset_package);

		std::vector<std::byte> serialized_pack = unwrap(
			weapon_pack.Serialize(),
			"Failed to serialize weapon asset pack"
		);

		unwrap(
			replicant::WriteFile(output_path.string(), serialized_pack),
			"Failed to write weapon asset pack to file"
		);

		return 0;

	}
};