#pragma once
#include <optional>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "Logger.h"

using json = nlohmann::json;
using enum Logger::LogCategory;

inline std::optional<json> readJSON(const std::filesystem::path& path) {

	json jdata;

	std::ifstream f(path, std::ios::binary);
	if (!f.is_open()) {
		Logger::Log(Error) << "Failed to open json file: " << path;
		return std::nullopt;
	}
	try {
		f >> jdata;
	}
	catch (const json::parse_error& ex) {
		Logger::Log(Error) << "Save file parsing failed - data is possibly corrupted (although recoverable?). Error: " << ex.what();
		return std::nullopt;

	}

	return jdata;


}
inline bool saveJSON(const std::filesystem::path& path, const json& jdata) {

	if (!std::filesystem::exists(path.parent_path())) {
		std::filesystem::create_directories(path.parent_path());
	}

	std::ofstream f;
	f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
	try {
		f.open(path, std::ios::binary);
	}
	catch (const std::ios_base::failure& e) {
		Logger::Log(Error) << "Could not open json file: "
			<< path
			<< " (" << e.what() << ")";
		return false;
	}

	try {
		std::string data = jdata.dump(4);
		f.write(data.data(), data.size());
		f.flush();
	}
	catch (const std::ios_base::failure& e) {
		Logger::Log(Error) << "Failed to write to json file " << path << " (" << e.what() << ")";
		return false;
	}
	return true;

}