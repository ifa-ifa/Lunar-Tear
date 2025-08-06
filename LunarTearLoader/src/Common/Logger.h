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