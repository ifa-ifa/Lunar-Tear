#pragma once

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <format>
#include <mutex>
#include <filesystem>
#include <sstream>

template <typename T>
concept Streamable = requires(std::ostream & os, const T & value) {
    { os << value } -> std::same_as<std::ostream&>;
};


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

        template <Streamable T>
        LogStream& operator<<(const T& value) {

            m_buffer << value;
            
            return *this;
        }

        template <typename E>
            requires std::is_enum_v<E>
        LogStream& operator<<(const E& value) {
                m_buffer << static_cast<std::underlying_type_t<E>>(value);
            
            return *this;
        }

        LogStream& operator<<(const std::wstring& value) {
            if (!value.empty()) {
                int size_needed = WideCharToMultiByte(CP_UTF8, 0, &value[0], (int)value.size(), NULL, 0, NULL, NULL);
                std::string utf8_str(size_needed, 0);
                WideCharToMultiByte(CP_UTF8, 0, &value[0], (int)value.size(), &utf8_str[0], size_needed, NULL, NULL);
                m_buffer << utf8_str;
            }
            return *this;
        }

        LogStream& operator<<(const std::u8string& value) {
                m_buffer << std::string(reinterpret_cast<const char*>(value.c_str()), value.length());
            
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