#include "Save.h"
#include "Logger.h"
#include "Json.h"
#include "base16.h"
#include <mutex>
#include <deque>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using enum Logger::LogCategory;

namespace {

	const std::filesystem::path saveFilePath("LunarTear/Gamedata/LTGamedata.json");

	std::deque<std::function<void()>> preSaveCallbacks;
	std::deque<std::function<void()>> postSaveCallbacks;
	std::deque<std::function<void()>> postLoadCallbacks;
	std::mutex saveCallbacksMutex;

	json activeSave;
	std::mutex activeSaveMutex;
	std::mutex saveFileMutex;

	std::string timestamp() {
		using namespace std::chrono;

		auto now = system_clock::now();

		auto secs = time_point_cast<seconds>(now);
		auto ms = duration_cast<milliseconds>(now - secs).count();

		std::time_t t = system_clock::to_time_t(secs);
		std::tm tm{};
		localtime_s(&tm, &t);

		std::ostringstream out;
		out << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S")
			<< '.' << std::setw(3) << std::setfill('0') << ms;

		return out.str();
	}





}

namespace Save {

	void saveActiveDataToDisk(uint32_t slot) {
		if (slot > 6) {
			Logger::Log(Error) << "Tried to write save data to (0-indexed) slot " << slot;
			return;
		}


		if (!std::filesystem::exists(saveFilePath)) {
			std::ofstream out(saveFilePath);
			if (!out) {
				Logger::Log(Error) << "Failed to create save file: " << saveFilePath;
				return;
			}
		}

		std::lock_guard saveFilelock(saveFileMutex);

		auto jdataRes = readJSON(saveFilePath);
		json jdata;

		if (!jdataRes) {
			Logger::Log(Warning) << "Could not read existing save file during write operation. Creating new file structure.";

			// If the file exists but is unreadable, maybe try to back it up here too?
			// Otherwise, just start fresh to ensure we save the users current progress
			jdata = json::array();
		}
		else {
			jdata = *jdataRes;
		}


		if (!jdata.is_array()) {
			Logger::Log(Warning) << "Save file root was not an array. Resetting.";
			jdata = json::array();
		}

		while (jdata.size() < 7)
			jdata.push_back(json::object());

		{
			std::lock_guard activeDataLock(activeSaveMutex);
			jdata[slot] = activeSave;
		}

		auto tmpFilePath = saveFilePath.parent_path() / "LunarTearSave.tmp";
		bool res = saveJSON(tmpFilePath, jdata);
		if (!res) return;

		std::filesystem::path backup = saveFilePath;
		backup += ".rollback";

		std::error_code ec;
		std::filesystem::remove(backup, ec);
		std::filesystem::rename(saveFilePath, backup, ec);
		std::filesystem::rename(tmpFilePath, saveFilePath, ec);

		if (ec) {
			Logger::Log(Error) << "Save replace failed: " << ec.message();
			// Try to restore
			std::filesystem::rename(backup, saveFilePath, ec);
			return;
		}

		std::filesystem::remove(backup, ec);

	}

	void loadActiveDataFromDisk(uint32_t slot) {

		if (slot > 6) {
			Logger::Log(Error) << "Tried to load save data from (0-indexed) slot " << slot;
			return;
		}

		if (!std::filesystem::exists(saveFilePath.parent_path())) {
			std::filesystem::create_directories(saveFilePath.parent_path());
		}
		if (!std::filesystem::exists(saveFilePath)) {
			Logger::Log(Warning) << "No save file found at: " << saveFilePath << ". Creating...";
			bool res = saveJSON(saveFilePath, json::array());
			if (!res) return;
		}

		std::lock_guard saveFilelock(saveFileMutex);

		{
			std::lock_guard activeDataLock(activeSaveMutex);
			activeSave = json::array();
		}

		auto jdataRes = readJSON(saveFilePath);
		json jdata;

		if (!jdataRes) {

			auto corruptedPath = saveFilePath.parent_path() / ("corrupted_" + timestamp());
			std::error_code ec;
			std::filesystem::rename(saveFilePath, corruptedPath, ec);
			if (ec) {
				Logger::Log(Error) << "Failed to preserve corrupted file. Returning";
				return;
			};
			Logger::Log(Info) << "Corrupt LT save backup created at: " << corruptedPath;
			std::filesystem::remove(saveFilePath);
			jdata = json::array();

		}
		else {
			jdata = *jdataRes;
		}

		if (!jdata.is_array())
			jdata = json::array();

		while (jdata.size() < 7)
			jdata.push_back(json::object());
		
		std::lock_guard activeDataLock(activeSaveMutex);
		activeSave = jdata[slot];
		

	}

	void copySlot(uint32_t sourceSlot, uint32_t destSlot) {
		std::lock_guard saveFilelock(saveFileMutex);

		auto jdataRes = readJSON(saveFilePath);
		if (!jdataRes) return;
		json jdata = *jdataRes;

		if (!jdata.is_array()) jdata = json::array();
		while (jdata.size() < 7)  jdata.push_back(json::object());


		jdata[destSlot] = jdata[sourceSlot];

		auto tmpFilePath = saveFilePath.parent_path() / "LunarTearSave.tmp";
		bool res = saveJSON(tmpFilePath, jdata);
		if (!res) return;

		std::filesystem::path backup = saveFilePath;
		backup += ".rollback";

		std::error_code ec;
		std::filesystem::remove(backup, ec);
		std::filesystem::rename(saveFilePath, backup, ec);
		std::filesystem::rename(tmpFilePath, saveFilePath, ec);

		if (ec) {
			Logger::Log(Error) << "Save replace failed: " << ec.message();
			// Try to restore
			std::filesystem::rename(backup, saveFilePath, ec);
			return;
		}
		std::filesystem::remove(backup, ec);

	}

	void clearSlot(uint32_t slot) {
		std::lock_guard saveFilelock(saveFileMutex);

		auto jdataRes = readJSON(saveFilePath);
		if (!jdataRes) return;
		json jdata = *jdataRes;

		if (!jdata.is_array()) jdata = json::array();
		while (jdata.size() < 7) jdata.push_back(json::object());


		jdata[slot] = json::object();

		auto tmpFilePath = saveFilePath.parent_path() / "LunarTearSave.tmp";
		bool res = saveJSON(tmpFilePath, jdata);
		if (!res) return;

		std::filesystem::path backup = saveFilePath;
		backup += ".rollback";

		std::error_code ec;
		std::filesystem::remove(backup, ec);
		std::filesystem::rename(saveFilePath, backup, ec);
		std::filesystem::rename(tmpFilePath, saveFilePath, ec);

		if (ec) {
			Logger::Log(Error) << "Save clear failed: " << ec.message();
			// Try to restore
			std::filesystem::rename(backup, saveFilePath, ec);
			return;
		}
		std::filesystem::remove(backup, ec);

	}

	void setString(const std::string& key, const std::string& val) {
		std::lock_guard lock(activeSaveMutex);

		activeSave[key] = val;
	}
	std::optional<std::string> getString(const std::string& key) {
		std::lock_guard lock(activeSaveMutex);

		if (activeSave.contains(key) && activeSave[key].is_string()) {
			return activeSave[key];
		}
		Logger::Log(Warning) << "Save data key: `" << key << "` does not exist";
		return std::nullopt;
	}

	void setBinary(const std::string& key, std::span<const std::byte> val) {
		std::lock_guard lock(activeSaveMutex);

		activeSave[key] = encodeBase16(val);
	}

	std::optional<std::vector<std::byte>> getBinary(const std::string& key) {
		std::lock_guard lock(activeSaveMutex);

		if (activeSave.contains(key) && activeSave[key].is_string()) {
			return decodeBase16(activeSave[key]);
		}
		Logger::Log(Warning) << "Save data key: `" << key << "` does not exist";
		return std::nullopt;
	}




	void RegisterPreSavePlayerDataCallback(std::function<void()>func) {
		std::lock_guard lock(saveCallbacksMutex);
		preSaveCallbacks.push_back(func);
	}
	void RegisterPostSavePlayerDataCallback(std::function<void()>func) {
		std::lock_guard lock(saveCallbacksMutex);
		postSaveCallbacks.push_back(func);
	}
	void RegisterLoadPlayerDataCallback(std::function<void()>func) {
		std::lock_guard lock(saveCallbacksMutex);
		postLoadCallbacks.push_back(func);
	}

	void exePreSaveCallbacks() {

		std::vector<std::function<void()>> snapshot;
		{
			std::lock_guard lock(saveCallbacksMutex);
			snapshot.assign(preSaveCallbacks.begin(), preSaveCallbacks.end());
		}
		for (const auto& func : snapshot) {
			try {
				func();
			}
			catch (const std::exception& e) {
				Logger::Log(Error) << "Exception executing pre save callback: " << e.what();
			}
			catch (...) {
				Logger::Log(Error) << "Unknown exception executing pre save callback";
			}
		}

	}
	void exePostSaveCallbacks() {
		std::vector<std::function<void()>> snapshot;
		{
			std::lock_guard lock(saveCallbacksMutex);
			snapshot.assign(postSaveCallbacks.begin(), postSaveCallbacks.end());
		}
		for (const auto& func : snapshot) {
			try {
				func();
			}
			catch (const std::exception& e) {
				Logger::Log(Error) << "Exception executing post save callback: " << e.what();
			}
			catch (...) {
				Logger::Log(Error) << "Unknown exception executing post save callback";
			}
		}
	}
	void exePostLoadCallbacks() {
		std::vector<std::function<void()>> snapshot;
		{
			std::lock_guard lock(saveCallbacksMutex);
			snapshot.assign(postLoadCallbacks.begin(), postLoadCallbacks.end());
		}
		for (const auto& func : snapshot) {
			try {
				func();
			}
			catch (const std::exception& e) {
				Logger::Log(Error) << "Exception executing post load callback: " << e.what();
			}
			catch (...) {
				Logger::Log(Error) << "Unknown exception executing post load callback";
			}
		}
	}


}