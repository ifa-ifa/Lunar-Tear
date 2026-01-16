#include "Weapons.h"
#include "Common/Logger.h"
#include "Common/Save.h"
#include "Game/Globals.h"
#include "ModLoader.h"
#include <replicant/weapon.h>
#include <unordered_map>
#include <string>
#include <replicant/core/reader.h>
#include "Game/Strings.h"
#include <replicant/core/writer.h>
#include <replicant/weapon.h>
#include <nlohmann/json.hpp>
#include <unordered_set>

using json = nlohmann::json;
using enum Logger::LogCategory;

namespace {

	std::unordered_map<int, std::string> weaponRegistry;
	std::unordered_set<int> usedListOrders;

	bool registerWeapon(const std::string& weaponStringID) {

		replicant::raw::RawWeaponBody** bodies = (replicant::raw::RawWeaponBody**)weaponSpecBodies;

		for (int i = 0; i < 64; i++) {
			if (bodies[i]) { continue; }
			if (weaponRegistry.find(i) != weaponRegistry.end()) { continue; }
			weaponRegistry.insert({ i , weaponStringID });
			Logger::Log(Info) << "Registered weapon `" << weaponStringID << "` for id " << i;
			return true;
		}

		Logger::Log(Logger::LogCategory::Error) << "Registry full - could not register wepon: " << weaponStringID;
		return false;


	}

	std::optional<int> findKeyByValue(const std::unordered_map<int, std::string>& m,
		const std::string& value)
	{
		for (const auto& [key, val] : m)
		{
			if (val == value)
				return key;
		}
		return std::nullopt;
	}


	void saveCustomWeapons() {

		size_t weaponCount = weaponRegistry.size();
		if (weaponCount == 0) return;

		replicant::Writer writer;
		replicant::StringPool strPool;

		writer.write(weaponCount);

		for (auto& [id, name] : weaponRegistry) {

			writer.write(static_cast<char>(playerSaveData->weaponLevels[id]));
			playerSaveData->weaponLevels[id] = static_cast<char>(-1);

			strPool.add(name, writer.reserveOffset());
		}

		strPool.flush(writer);

		Save::setBinary("_weapons", writer.buffer());
	}

	void loadCustomWeapons() {

		auto dataRes = Save::getBinary("_weapons");
		if (!dataRes) return;

		std::vector<std::byte> data = *dataRes;

		replicant::Reader reader(data);

		const size_t count = *reader.view<size_t>();
		
		for (size_t i = 0; i < count; i++) {

			const char level = *reader.view<char>();
			std::string name = reader.readStringRelative(*reader.view<uint32_t>());

			auto id = findKeyByValue(weaponRegistry, name);
			if (!id) continue;

			playerSaveData->weaponLevels[*id] = level;
			
		}
	}

	std::vector<std::byte> s_weaponBuf;

	
}

replicant::weapon::WeaponEntry jsonToWeapon(json jdata) {
	replicant::weapon::WeaponEntry entry;
	if (!jdata.is_object()) {
		throw std::string("top level weapon json is not an object");
	}

	// We need to ensure there are no duplicate listOrder values
	replicant::raw::RawWeaponBody** bodies = (replicant::raw::RawWeaponBody**)weaponSpecBodies;
	int newListOrder = 0;

	for (int i = 0; i < 64; i++) {
		if (usedListOrders.count(i) == 1) continue;
		newListOrder = i;
		break;
	}
	usedListOrders.insert(newListOrder);
	entry.listOrder = newListOrder;

	


	entry.internalNameBody = jdata.at("asset_name").get<std::string>();
	entry.nameStringID = getLTStringId(jdata.at("display_name").get<std::string>());
	entry.descStringID = getLTStringId(jdata.at("display_desc").get<std::string>());
	
	entry.story1StringID = getLTStringId(jdata.at("display_story1").get<std::string>());
	entry.story2StringID = getLTStringId(jdata.at("display_story2").get<std::string>());
	entry.story3StringID = getLTStringId(jdata.at("display_story3").get<std::string>());
	entry.story4StringID = getLTStringId(jdata.at("display_story4").get<std::string>());
	entry.HeightOnBack = jdata.at("displacment_on_back").get<float>();
	entry.InHandDisplacment = jdata.at("displacment_in_hand").get<float>();
	entry.shopPrice = jdata.at("shop_price").get<uint32_t>();
	entry.knockbackPercent = jdata.at("knockback_precent").get<float>();
	entry.weaponType = jdata.at("weapon_type").get<uint8_t>();
	entry.excludeFromCompletion = jdata.at("exclude_from_completion").get<bool>() ? 1 : 0;
	std::vector<uint32_t> attack = jdata.at("attack_power").get<std::vector<uint32_t>>();
	std::vector<uint32_t> magic = jdata.at("magic_power").get<std::vector<uint32_t>>();
	std::vector<uint32_t> guard = jdata.at("guard_break").get<std::vector<uint32_t>>();
	std::vector<uint32_t> armor = jdata.at("armour_break").get<std::vector<uint32_t>>();
	std::vector<uint32_t> weight = jdata.at("weight").get<std::vector<uint32_t>>();

	auto fillStats = [&](replicant::weapon::WeaponStats& stats, size_t index) {
		if (index >= attack.size() || index >= magic.size() ||
			index >= guard.size() || index >= armor.size() || index >= weight.size()) {
			throw std::out_of_range("Stat arrays in JSON are shorter than required 4 levels");
		}
		stats.attack = attack[index];
		stats.magicPower = magic[index];
		stats.guardBreak = guard[index];
		stats.armourBreak = armor[index];
		stats.weight = weight[index];
		};

	auto parseRecipe = [&](replicant::weapon::WeaponUpgradeRecipe& recipe, const json& rJson) {
		recipe.upgradeCost = rJson.at("cost").get<uint32_t>();

		std::vector<int32_t> ingredients = rJson.at("ingredients").get<std::vector<int32_t>>();
		std::vector<uint32_t> counts = rJson.at("count").get<std::vector<uint32_t>>();

		if (ingredients.size() != counts.size()) {
			throw std::runtime_error("Recipe ingredient list size does not match count list size");
		}

		if (ingredients.size() > 0) { recipe.ingredientId1 = ingredients[0]; recipe.ingredientCount1 = counts[0]; }
		if (ingredients.size() > 1) { recipe.ingredientId2 = ingredients[1]; recipe.ingredientCount2 = counts[1]; }
		if (ingredients.size() > 2) { recipe.ingredientId3 = ingredients[2]; recipe.ingredientCount3 = counts[2]; }
		};


	fillStats(entry.level1Stats, 0);
	fillStats(entry.level2Stats, 1);
	fillStats(entry.level3Stats, 2);
	fillStats(entry.level4Stats, 3);

	const auto& recipes = jdata.at("recipes");
	if (recipes.size() < 3) {
		throw std::string("Recipes array must contain at least 3 entries (for levels 2, 3, 4)");
	}

	parseRecipe(entry.level2Recipe, recipes.at(0));
	parseRecipe(entry.level3Recipe, recipes.at(1));
	parseRecipe(entry.level4Recipe, recipes.at(2));

	return entry;

}


void fixEquippedWeapon() {

	if (!((replicant::raw::RawWeaponBody**)weaponSpecBodies)[playerSaveData->currentWeapon]) {

		playerSaveData->currentWeapon = 0;
	}

}


// We just fill the registry blindly and patch weapon specs loadCustomWeapons will fill the players
//   inventory based on unique string ids in the registry, the registry itself is volatile and 
//   does not need to be saved

void InitWeaponSystem() {

	replicant::raw::RawWeaponBody** bodies = (replicant::raw::RawWeaponBody**)weaponSpecBodies;
	for (int i = 0; i < 64; i++) {
		if (!bodies[i]) continue;
		usedListOrders.insert(bodies[i]->listOrder);
	}

	std::vector<json> jweapons = GetCustomWeapons();

	if (jweapons.empty()) {
		Logger::Log(Verbose) << "No custom weapons found. Skipping weapon system init.";
		Save::RegisterLoadPlayerDataCallback(fixEquippedWeapon);
		return;
	}

	std::vector<replicant::weapon::WeaponEntry> weapons;
	std::vector<int> weaponID;

	for (size_t i = 0; i < jweapons.size(); i++) {

		try {
			bool res = registerWeapon(jweapons[i].at("unique_name").get<std::string>());
			if (!res) break;
			weapons.push_back(jsonToWeapon(jweapons[i]));

		}
		catch (const std::string& e) {
			Logger::Log(Error) << "Error parsing weapon: " << e;
		}
		catch (const json::type_error& e) {
			Logger::Log(Error) << "Json type error (parsing weapons): " << e.what();
		}
		catch (const json::out_of_range& e) {
			Logger::Log(Error) << "Json out-of-bounds error (parsing weapons): " << e.what();
		}

	}

	auto weaponBufRes = replicant::weapon::serialiseWeaponSpecs(weapons);
	if (!weaponBufRes) {
		Logger::Log(Error) << "Error serialising weapons: " << weaponBufRes.error().toString();
	}

	replicant::Reader reader(*weaponBufRes);
	
	auto head = reader.view<replicant::raw::RawWeaponHeader>();
	reader.seek(reader.getOffsetPtr(head->offsetToDataStart));
	s_weaponBuf = std::vector<std::byte>(
		reinterpret_cast<const std::byte*>(reader.getOffsetPtr(head->offsetToDataStart)), 
		reinterpret_cast<const std::byte*>(reader.getOffsetPtr(head->offsetToDataStart))+reader.remaining()
	);


	for (size_t i = 0; i < weapons.size(); i++) {
		std::string weaponustr = jweapons[i].at("unique_name").get<std::string>();
		auto res = findKeyByValue(weaponRegistry, weaponustr);
		if (!res) {
			Logger::Log(Error) << "No id for weapon: " << weaponustr;
			continue;
		}

		int gameID = *res;

		size_t entryStride = sizeof(replicant::raw::RawWeaponEntry);
		size_t entryOffset = i * entryStride;

		size_t bodyOffset = entryOffset + offsetof(replicant::raw::RawWeaponEntry, body);

		std::byte* bodyAddress = s_weaponBuf.data() + bodyOffset;
		((replicant::raw::RawWeaponBody**)weaponSpecBodies)[gameID] =
			reinterpret_cast<replicant::raw::RawWeaponBody*>(bodyAddress);

		Logger::Log(Info) << "Patched weapon table index " << gameID << " (" << weaponustr << ")";
	}

	Save::RegisterPreSavePlayerDataCallback(saveCustomWeapons);
	Save::RegisterPostSavePlayerDataCallback(loadCustomWeapons);
	Save::RegisterLoadPlayerDataCallback(loadCustomWeapons);

	Save::RegisterLoadPlayerDataCallback(fixEquippedWeapon);

}



int getIdFromName(const std::string& name) {
	auto res = findKeyByValue(weaponRegistry, name);
	if (!res) {
		return -1;
	}
	return *res;
}