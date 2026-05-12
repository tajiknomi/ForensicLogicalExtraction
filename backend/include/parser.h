#pragma once

#include <map>
#include "json.h"


enum class DataType {
    SMS,
    CALL,
    MEDIA,
    CONTACTS,
    WHATSAPP,
    USER_INSTALLED_APPS,
    CALENDAR,
    EVENTS,
    WHEN,
    ATTENDEES,
    REMINDERS,
    EXTENDED_PROPERTIES
};


class PARSER {

private:
    //    void saveToFile(const std::string& data, const std::string& filePath);
    static std::map<std::string, std::string> parseRow(const std::string& line);
    static nlohmann::json parseADBOutputToJSON(const std::string& output, const std::string& rowPrefix = "Row:");
    static nlohmann::json parseSMS(const std::string& output);
    static nlohmann::json parseCallLogs(const std::string& output);
    static nlohmann::json parseMedia(const std::string& output);
    static nlohmann::json parseInstalledApps(const std::string& output);
    static nlohmann::json parseCalendar(const std::string& artifactRawData);
    static nlohmann::json parseCalendarEvents(const std::string& artifactRawData);
    static nlohmann::json parseCalendarWhen(const std::string& artifactRawData);
    static nlohmann::json parseCalendarAttendees(const std::string& artifactRawData);
    static nlohmann::json parseCalendarReminders(const std::string& artifactRawData);
    static nlohmann::json parseCalendarExtendedProperties(const std::string& artifactRawData);
    static std::string formatEpochMillis(const std::string& millisStr);
    static std::vector<std::map<std::string, std::string>> extractRows(const std::string& output);
    static std::string trim(const std::string& str);

public:
    static nlohmann::json parseAdbDeviceInfo(const std::string& deviceInfoFromAdb);
    static void saveJSONToFile(const nlohmann::json& data, const std::string& outputFile);
    static nlohmann::json parseArtifact(const std::string& artifactRawData, DataType type);
    static nlohmann::json mergeCalendarArtifacts(const nlohmann::json& calendars_json,
        const nlohmann::json& events_json,
        const nlohmann::json& calendarWhen_json,
        const nlohmann::json& attendees_json,
        const nlohmann::json& reminders_json,
        const nlohmann::json& extendedproperties_json);
    static nlohmann::json flattenForensicCalendar(const nlohmann::json& mergedCalendars);
};