#include "Logger.h"
#include <map>
#include "settings.h"
#include <atomic>

namespace
{
    bool s_log_to_console = true;
    bool s_log_to_file = true;


    std::map<Logger::LogCategory, bool> s_category_enabled;


    std::ofstream s_log_file;
    std::mutex s_log_mutex;
    std::atomic<bool> s_logger_initialized = false; 


    std::string CategoryToString(Logger::LogCategory category) {
        switch (category) {
        case Logger::LogCategory::Info:     return "[Info]    ";
        case Logger::LogCategory::Verbose:  return "[Verbose] ";
        case Logger::LogCategory::Warning:  return "[Warning] ";
        case Logger::LogCategory::Error:    return "[Error]   ";
        case Logger::LogCategory::FileInfo: return "[FileInfo]";
        default:                            return "[Unknown] ";
        }
    }


    void Write(Logger::LogCategory category, const std::string& message)
    {
        std::lock_guard<std::mutex> lock(s_log_mutex);

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf;
        localtime_s(&tm_buf, &time_t);

        std::string final_message = std::format("[{:02}:{:02}:{:02}] [Lunar Tear] {} {}",
            tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, CategoryToString(category), message);

        if (s_log_to_console) {
            std::cout << final_message << std::endl;
        }
        if (s_log_to_file && s_log_file.is_open()) {
            s_log_file << final_message << std::endl;
        }
    }
}


void Logger::Init()
{

    Settings& settings = Settings::Instance();

    s_category_enabled = {
        {LogCategory::Info, settings.LogInfo},
        {LogCategory::Verbose, settings.LogVerbose},
        {LogCategory::Warning, settings.LogWarning},
        {LogCategory::Error, settings.LogError},
        {LogCategory::FileInfo, settings.LogOriginalFileInfo}
    };


    if (settings.LogToConsole) {
        AllocConsole();
        FILE* console;
        freopen_s(&console, "CONOUT$", "w", stdout);
    }
    if (settings.LogToFile) {
        s_log_file.open("LunarTear/lunartear.log", std::ios::out | std::ios::trunc);
    }

    s_logger_initialized = true;
}

Logger::LogStream::LogStream(LogCategory category, bool active)
    : m_category(category), m_is_active(active) {}

Logger::LogStream::~LogStream()
{
    if (m_is_active && m_buffer.tellp() > 0) {
        Write(m_category, m_buffer.str());
    }
}

bool Logger::IsActive(LogCategory category) {
    return s_logger_initialized && (s_category_enabled.count(category) ? s_category_enabled.at(category) : false);
}

Logger::LogStream Logger::Log(LogCategory category)
{
    return LogStream{ category, IsActive(category)};
}