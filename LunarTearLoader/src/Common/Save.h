#pragma once
#include <string>
#include <optional>
#include <vector>
#include <span>
#include <functional>


namespace Save {


	void saveActiveDataToDisk(uint32_t slot);
	void loadActiveDataFromDisk(uint32_t slot);
	void copySlot(uint32_t srcSlot, uint32_t dstSlot);
	void clearSlot(uint32_t slot);

	// Save to and fetch from the active Lunar Tear save data
	// Keys must be unique, you should prefix with your author name or mod name to avoid collisions


	void setString(const std::string& key, const std::string& val);
	std::optional<std::string> getString(const std::string& key);

	// Will be automatically encoded and decoded in base16
	void setBinary(const std::string& key, std::span<const std::byte> data);
	std::optional<std::vector<std::byte>> getBinary(const std::string& key);


	// The below functions are to help make sure that all saves are vanilla compatible. 
	//  Any modded state should be removed from the main playerData and saved using the above functions, 
	//  then restored on save load.
	// Callbacks registered with any of the below functions are not called when loading from/saving 
	//   into the volatile backup.
	

	// Called just before active save data is saved into GAMEDATA
	void RegisterPreSavePlayerDataCallback(std::function<void()>func);
	// Called just after active save data is saved into GAMEDATA
	void RegisterPostSavePlayerDataCallback(std::function<void()>func);

	// Called just after an data slot from GAMEDATA is loaded into active data
	void RegisterLoadPlayerDataCallback(std::function<void()>func);


	void exePreSaveCallbacks();
	void exePostSaveCallbacks();
	void exePostLoadCallbacks();

	

}