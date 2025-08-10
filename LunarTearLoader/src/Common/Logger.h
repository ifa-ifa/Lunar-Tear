#pragma once

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <format>
#include <mutex>
#include <filesystem>
#include <sstream>

namespace Logger
{
    enum class LogCategory {
        Info,       
        Verbose,     
        Warning,     
        Error,       
        FileInfo,
        Lua
    };

    class LogStream
    {
    public:
        LogStream(LogCategory category, bool active, std::string pluginName = "");
        ~LogStream();

        template <typename T>
        LogStream& operator<<(const T& value) {

            if (m_is_active) {
                m_buffer << value;
            }
            return *this;
        }

        LogStream& operator<<(const std::wstring& value) {
            if (m_is_active && !value.empty()) {
                // Convert the wstring (UTF-16) to a UTF-8 string for our stream
                int size_needed = WideCharToMultiByte(CP_UTF8, 0, &value[0], (int)value.size(), NULL, 0, NULL, NULL);
                std::string utf8_str(size_needed, 0);
                WideCharToMultiByte(CP_UTF8, 0, &value[0], (int)value.size(), &utf8_str[0], size_needed, NULL, NULL);
                m_buffer << utf8_str;
            }
            return *this;
        }

        LogStream& operator<<(const std::u8string& value) {
            if (m_is_active) {
                // Convert the u8string to a string before passing it to the stringstream
                m_buffer << std::string(reinterpret_cast<const char*>(value.c_str()), value.length());
            }
            return *this;
        }

    private:
        friend LogStream Log(LogCategory category, const std::string& pluginName);
        std::stringstream m_buffer;
        LogCategory m_category;
        bool m_is_active;
        std::string m_pluginName;
    };

    void Init();

    bool IsActive(LogCategory category);

    LogStream Log(LogCategory category, const std::string& pluginName = "");
}