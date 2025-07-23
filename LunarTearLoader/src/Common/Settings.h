#pragma once
#include <Windows.h>
#include <INIReader.h>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <type_traits>



struct Settings;


    class ISetting {
    public:
        virtual ~ISetting() = default;

        virtual const std::string& getKey() const = 0;
        virtual const std::string& getComment() const = 0;
        virtual std::string getDefaultValueAsString() const = 0;

        virtual void load(INIReader& reader, Settings& settings) const = 0;
        virtual void applyDefault(Settings& settings) const = 0;
    };

    template<typename T>
    class Setting : public ISetting {
    public:

        Setting(std::string key, T defaultValue, T Settings::* memberPtr, std::string comment)
            : m_key(std::move(key)),
            m_defaultValue(defaultValue),
            m_memberPtr(memberPtr),
            m_comment(std::move(comment)) {
        }

        const std::string& getKey() const override { return m_key; }
        const std::string& getComment() const override { return m_comment; }

        std::string getDefaultValueAsString() const override {
            if constexpr (std::is_same_v<T, bool>) {
                return m_defaultValue ? "true" : "false";
            }
            else {
                return std::to_string(m_defaultValue);
            }
        }

        void load(INIReader& reader, Settings& settings) const override {
            if constexpr (std::is_same_v<T, bool>) {
                settings.*m_memberPtr = reader.GetBoolean("main", m_key, m_defaultValue);
            }
            else if constexpr (std::is_floating_point_v<T>) {
                settings.*m_memberPtr = reader.GetReal("main", m_key, m_defaultValue);
            }
            else if constexpr (std::is_integral_v<T>) {
                settings.*m_memberPtr = reader.GetInteger("main", m_key, m_defaultValue);
            }
        }

        void applyDefault(Settings& settings) const override {
            settings.*m_memberPtr = m_defaultValue;
        }

    private:
        std::string m_key;
        T m_defaultValue;
        T Settings::* m_memberPtr;
        std::string m_comment;
    };



class Settings {
public:


    bool LogInfo;
    bool LogVerbose;
    bool LogWarning;
    bool LogError;
    bool LogOriginalFileInfo;
    
    bool LogToConsole;
    bool LogToFile;

    bool DumpTextures;
    bool DumpScripts;
    bool DumpTables;



    static Settings& Instance() {
        static std::unique_ptr<Settings> instance = [] {
            auto s = std::make_unique<Settings>();
            return s;
            }();
        return *instance;
    }

    Settings() {


        registerSetting("LogInfo", true, &Settings::LogInfo, "");
        registerSetting("LogVerbose", false, &Settings::LogVerbose, "");
        registerSetting("LogWarning", true, &Settings::LogWarning, "");
        registerSetting("LogError", true, &Settings::LogError, "");
        registerSetting("LogOriginalFileInfo", false, &Settings::LogOriginalFileInfo, "");

        registerSetting("LogToConsole", false, &Settings::LogToConsole, "");
        registerSetting("LogToFile", true, &Settings::LogToFile, "");

        registerSetting("DumpTextures", false, &Settings::DumpTextures, "");
        registerSetting("DumpScripts", false, &Settings::DumpScripts, "");
        registerSetting("DumpTables", false, &Settings::DumpTables, "");
   



    }

    // Returns: 0 = success, 1 = used defaults (corrupt file), 2 = used defaults (created new file)
    int LoadFromFile(const std::string& path = "LunarTear/LunarTear.ini") {

        std::ifstream fileCheck(path);
        if (!fileCheck.good()) {
            // File does not exist. Create it.
            WriteDefaultIni(path);
            for (const auto& setting : m_settings) {
                setting->applyDefault(*this);
            }
            return 2;
        }
        fileCheck.close();

        // File exists, now try to parse it.
        INIReader reader(path);
        if (reader.ParseError() != 0) {
            MessageBoxW(0,
                L"The INI file is corrupt or contains an error.\n\n"
                L"Default settings will be used for this session.\n\n"
                L"Either correct the error or delete the INI file and a new INI will be generated on the next game launch.",
                L"Lunar Tear Config Error", 0);

            for (const auto& setting : m_settings) {
                setting->applyDefault(*this);
            }
            return 1;
        }

        // File exists and is valid
        for (const auto& setting : m_settings) {
            setting->load(reader, *this);
        }
        return 0;
    }

private:

    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    template<typename T>
    void registerSetting(const std::string& key, T defaultValue, T Settings::* memberPtr, const std::string& comment) {
        m_settings.push_back(std::make_unique<Setting<T>>(key, defaultValue, memberPtr, comment));
    }

    void WriteDefaultIni(const std::string& path) const {
        std::ofstream file(path);
        if (!file.is_open()) {
            return;
        }

        file << "[main]\n\n";
        for (const auto& setting : m_settings) {
            if (setting->getComment() != "") {
                file << "# " << setting->getComment() << "\n";
            }
            file << setting->getKey() << " = " << setting->getDefaultValueAsString() << "\n\n";
        }
    }

    std::vector<std::unique_ptr<ISetting>> m_settings;
};