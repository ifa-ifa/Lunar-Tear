#include "Logger.h"
#include <map>

namespace
{
    bool s_log_to_console = true;
    bool s_log_to_file = true;

    std::map<Logger::LogCategory, bool> s_category_enabled;

    // Helper to associate enum with INI key and default value.
    const std::map<Logger::LogCategory, std::pair<const char*, bool>> c_category_info = {
        { Logger::LogCategory::Info,     { "LogInfo", true } },
        { Logger::LogCategory::Verbose,  { "LogVerbose", false } },
        { Logger::LogCategory::Warning,  { "LogWarnings", true } },
        { Logger::LogCategory::Error,    { "LogErrors", true } },
        { Logger::LogCategory::FileInfo, { "LogOriginalFileInfo", false } }
    };

    std::ofstream s_log_file;
    std::mutex s_log_mutex;


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
    const char* ini_path = "LunarTear/LunarTear.ini";
    const char* log_dir = "LunarTear";
    std::filesystem::create_directory(log_dir);

    if (!std::filesystem::exists(ini_path)) {
        std::ofstream default_ini(ini_path);
        if (default_ini.is_open()) {
            default_ini << "; Lunar tear config file - please delete this file upgrading version and a new one will be created for you\n";
            default_ini << "[Logging]\n";
            default_ini << "; Set to 1 to enable a category, 0 to disable.\n\n";
            default_ini << "LogInfo=1    ; General information \n";
            default_ini << "LogVerbose=0    ; Detailed Information\n";
            default_ini << "LogWarnings=1    ; Non-critical problems\n";
            default_ini << "LogErrors=1    ; Critical errors that might cause issues\n";
            default_ini << "LogOriginalFileInfo=0    ; For mod makers\n";
            default_ini << "\n; Destinations\n";
            default_ini << "LogToConsole=0\n";
            default_ini << "LogToFile=1\n";
        }
    }

    for (const auto& pair : c_category_info) {
        Logger::LogCategory cat = pair.first;
        const char* key = pair.second.first;
        bool default_val = pair.second.second;
        s_category_enabled[cat] = (GetPrivateProfileIntA("Logging", key, default_val, ini_path) != 0);
    }

    s_log_to_console = (GetPrivateProfileIntA("Logging", "LogToConsole", 0, ini_path) != 0);
    s_log_to_file = (GetPrivateProfileIntA("Logging", "LogToFile", 1, ini_path) != 0);

    if (s_log_to_console) {
        AllocConsole();
        FILE* console;
        freopen_s(&console, "CONOUT$", "w", stdout);
    }
    if (s_log_to_file) {
        s_log_file.open("LunarTear/lunartear.log", std::ios::out | std::ios::trunc);
    }
}

Logger::LogStream::LogStream(LogCategory category, bool active)
    : m_category(category), m_is_active(active) {
}

Logger::LogStream::~LogStream()
{
    if (m_is_active && m_buffer.tellp() > 0) {
        Write(m_category, m_buffer.str());
    }
}

Logger::LogStream Logger::Log(LogCategory category)
{
    bool is_active = s_category_enabled.count(category) ? s_category_enabled.at(category) : false;
    return LogStream{ category, is_active };
}