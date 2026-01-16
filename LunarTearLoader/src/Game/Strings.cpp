#include "Strings.h"

namespace {
	std::mutex stringMapMutex;
	std::deque<std::string> stringArena;

	std::unordered_map<int, const char*> stringMap;
	int currentId = LTStringRangeMin;
}


int getLTStringId(const char* str) {

	std::lock_guard<std::mutex> lock(stringMapMutex);


	for (const auto& [cId, cStr] : stringMap) {
		if (strcmp(str, cStr) == 0) { return cId; }
	}

	if (currentId > LTStringRangeMax) { return -1; }

	stringArena.push_back(str);
	stringMap.insert({ currentId, stringArena.back().c_str()});
	currentId++;

	return currentId - 1;
}

int getLTStringId(const std::string& str)
{
	return getLTStringId(str.c_str());
}


const char* getLTString(int id) {

	std::lock_guard<std::mutex> lock(stringMapMutex);


	auto res = stringMap.find(id);

	if (res == stringMap.end()) {
		return "<LUNAR_TEAR_NO_TEXT>";
	}

	return res->second;

}