#pragma once
#include <mutex>
#include <unordered_map>
#include <deque>

inline constexpr int LTStringRangeMin = 700074000;
inline constexpr int LTStringRangeMax = 900740000;


int getLTStringId(const char* str);
int getLTStringId(const std::string& str);

const char* getLTString(int id);