#include "parser.h"
#include <iostream>
#include <fstream>
#include <sstream>


// ===================================== PRIVATE =====================================

//void ForensicExtractor::saveToFile(const std::string& data, const std::string& filePath) {
//    std::ofstream file(filePath, std::ios::out);
//
//    if (!file.is_open()) {
//        std::cerr << "[-] Failed to open file: " << filePath << std::endl;
//        return;
//    }
//
//    file << data;
//    file.close();
//}

std::string PARSER::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r,");

    if (start == std::string::npos) return "";
    return str.substr(start, end - start + 1);
}

std::string PARSER::formatEpochMillis(const std::string& millisStr) {
    if (millisStr.empty()) return "";

    try {
        long long millis = std::stoll(millisStr);
        std::time_t seconds = millis / 1000;

        std::tm tm_time;
#ifdef _WIN32
        localtime_s(&tm_time, &seconds);
#else
        localtime_r(&seconds, &tm_time);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm_time, "%d %b %Y %I:%M %p");
        return oss.str();
    }
    catch (...) {
        return millisStr; // fallback if conversion fails
    }
}

std::map<std::string, std::string> PARSER::parseRow(const std::string& line) {
    std::map<std::string, std::string> row;

    // Find start after "Row: X "
    size_t startPos = line.find("Row:");
    if (startPos == std::string::npos) return row;

    // Move past "Row: <num> "
    startPos = line.find(' ', startPos + 4);
    if (startPos == std::string::npos) return row;
    std::string content = line.substr(startPos + 1);
    std::vector<size_t> keyPositions;

    // Find all positions where a key starts (pattern: word=)
    for (size_t i = 0; i < content.size(); ++i) {
        if (content[i] == '=') {
            // walk backwards to find key start
            size_t j = i;
            while (j > 0 && std::isalnum(content[j - 1]) || content[j - 1] == '_') {
                --j;
            }
            keyPositions.push_back(j);
        }
    }
    // Extract key-value pairs
    for (size_t i = 0; i < keyPositions.size(); ++i) {
        size_t keyStart = keyPositions[i];
        size_t eqPos = content.find('=', keyStart);

        if (eqPos == std::string::npos) continue;

        std::string key = content.substr(keyStart, eqPos - keyStart);

        size_t valueStart = eqPos + 1;
        size_t valueEnd;

        if (i + 1 < keyPositions.size()) {
            valueEnd = keyPositions[i + 1];
        }
        else {
            valueEnd = content.size();
        }

        std::string value = content.substr(valueStart, valueEnd - valueStart);

        row[trim(key)] = trim(value);
    }
    return row;
}

nlohmann::json PARSER::parseADBOutputToJSON(const std::string& output, const std::string& rowPrefix) {
    std::istringstream stream(output);
    std::string line;

    nlohmann::json jsonArray = nlohmann::json::array();

    while (std::getline(stream, line)) {
        if (line.find(rowPrefix) != std::string::npos) {
            auto parsed = parseRow(line);

            if (!parsed.empty()) {
                nlohmann::json obj;

                for (const auto& kv : parsed) {
                    obj[kv.first] = kv.second;
                }

                jsonArray.push_back(obj);
            }
        }
    }

    return jsonArray;
}

std::vector<std::map<std::string, std::string>> PARSER::extractRows(const std::string& input) {
    std::istringstream stream(input);
    std::string line;
    std::vector<std::map<std::string, std::string>> rows;

    while (std::getline(stream, line)) {
        if (line.find("Row:") != std::string::npos) {
            auto parsed = parseRow(line);
            if (!parsed.empty()) {
                rows.push_back(parsed);
            }
        }
    }
    return rows;
}

nlohmann::json PARSER::parseSMS(const std::string& artifactRawData) {
    auto rows = extractRows(artifactRawData);
    nlohmann::json result = nlohmann::json::array();

    for (const auto& row : rows) {
        nlohmann::json obj;

        obj["type"] = "sms";
        obj["address"] = row.count("address") ? row.at("address") : "";
        obj["message"] = row.count("body") ? row.at("body") : "";
        //obj["timestamp"] = row.count("date") ? row.at("date") : "";   // This store unix timestamp
        obj["timestamp"] = row.count("date")                            // Convert unix to human readable timestamp
            ? PARSER::formatEpochMillis(row.at("date"))
            : "";

        // SMS type mapping
        if (row.count("type")) {
            std::string t = row.at("type");
            if (t == "1") obj["direction"] = "inbox";
            else if (t == "2") obj["direction"] = "sent";
            else obj["direction"] = "unknown";
        }

        result.push_back(obj);
    }
    return result;
}

nlohmann::json PARSER::parseCallLogs(const std::string& artifactRawData) {
    auto rows = extractRows(artifactRawData);
    nlohmann::json result = nlohmann::json::array();
    for (const auto& row : rows) {
        nlohmann::json obj;

        obj["type"] = "call";
        obj["number"] = row.count("number") ? row.at("number") : "";
        obj["duration"] = row.count("duration") ? row.at("duration") : "";
       // obj["timestamp"] = row.count("date") ? row.at("date") : "";   // This store unix timestamp
        obj["timestamp"] = row.count("date")                            // Convert unix to human readable timestamp
            ? PARSER::formatEpochMillis(row.at("date"))
            : "";

        // Call type mapping
        if (row.count("type")) {
            std::string t = row.at("type");
            if (t == "1") obj["call_type"] = "incoming";
            else if (t == "2") obj["call_type"] = "outgoing";
            else if (t == "3") obj["call_type"] = "missed";
            else obj["call_type"] = "unknown";
        }

        result.push_back(obj);
    }
    return result;
}

nlohmann::json PARSER::parseContacts(const std::string& artifactRawData) {
    auto rows = extractRows(artifactRawData); // your existing row extractor
    nlohmann::json result = nlohmann::json::array();

    // Mapping numeric type to string
    std::unordered_map<std::string, std::string> phoneTypeMap = {
        {"1", "HOME"},
        {"2", "MOBILE"},
        {"3", "WORK"},
        {"4", "FAX_WORK"},
        {"5", "FAX_HOME"},
        {"6", "PAGER"},
        {"7", "OTHER"},
        {"8", "CALLBACK"},
        {"9", "CAR"},
        {"10", "COMPANY_MAIN"},
        {"11", "ISDN"},
        {"12", "MAIN"},
        {"13", "OTHER_FAX"},
        {"14", "RADIO"},
        {"15", "TELEX"},
        {"16", "TTY_TDD"},
        {"17", "WORK_MOBILE"},
        {"18", "WORK_PAGER"},
        {"19", "ASSISTANT"},
        {"20", "MMS"}
    };

    for (const auto& row : rows) {
        nlohmann::json obj;
        obj["type"] = "contact";

        // Name: if missing, use empty string
        obj["name"] = row.count("display_name") && !row.at("display_name").empty()
            ? row.at("display_name")
            : "";

        // Phone: if missing, skip the entry for forensic cleanliness
        if (row.count("number") && !row.at("number").empty()) {
            obj["phone"] = row.at("number");
        }
        else {
            continue; // skip rows without phone numbers
        }

        // Map numeric type to string, default to OTHER
        if (row.count("type") && !row.at("type").empty() && phoneTypeMap.count(row.at("type"))) {
            obj["phone_type"] = phoneTypeMap[row.at("type")];
        }
        else {
            obj["phone_type"] = "OTHER";
        }

        // Use contact_id if present, otherwise use person
        if (row.count("contact_id") && !row.at("contact_id").empty()) {
            obj["contact_id"] = row.at("contact_id");
        }
        else if (row.count("person") && !row.at("person").empty()) {
            obj["contact_id"] = row.at("person");
        }
        else {
            obj["contact_id"] = ""; // fallback empty
        }

        result.push_back(obj);
    }

    return result;
}

nlohmann::json PARSER::parseMedia(const std::string& artifactRawData) {
    auto rows = extractRows(artifactRawData);
    nlohmann::json result = nlohmann::json::array();
    for (const auto& row : rows) {
        nlohmann::json obj;
        obj["type"] = "media";
        obj["path"] = row.count("_data") ? row.at("_data") : "";
        obj["mime_type"] = row.count("mime_type") ? row.at("mime_type") : "";
        obj["size"] = row.count("size") ? row.at("size") : "";
        obj["timestamp"] = row.count("date_added") ? row.at("date_added") : "";

        result.push_back(obj);
    }
    return result;
}

nlohmann::json PARSER::parseInstalledApps(const std::string& artifactRawData) {

    nlohmann::json result = nlohmann::json::array();
    std::istringstream stream(artifactRawData);
    std::string line;
    while (std::getline(stream, line))
    {
        if (line.empty())
            continue;

        const std::string prefix = "package:";
        if (line.find(prefix) == 0)
        {
            std::string packageName = line.substr(prefix.length());
            nlohmann::json obj;
            obj["type"] = "package";
            obj["package_name"] = packageName;
            result.push_back(obj);
        }
    }
    return result;
}

nlohmann::json PARSER::parseCalendar(const std::string& artifactRawData) {
    auto rows = extractRows(artifactRawData);
    nlohmann::json result = nlohmann::json::array();

    for (const auto& row : rows) {
        nlohmann::json obj;
        obj["type"] = "calendar";
        obj["calendar_id"] = row.count("_id") ? row.at("_id") : "";
        obj["display_name"] = row.count("calendar_displayName")
            ? row.at("calendar_displayName")
            : "";
        obj["account"] = {
            { "name", row.count("account_name") ? row.at("account_name") : "" },
            { "type", row.count("account_type") ? row.at("account_type") : "" },
            { "owner", row.count("ownerAccount") ? row.at("ownerAccount") : "" }
        };
        obj["settings"] = {
            { "timezone", row.count("calendar_timezone") ? row.at("calendar_timezone") : "" },
            { "visible", row.count("visible") ? row.at("visible") == "1" : false },
            { "sync_enabled", row.count("sync_events") ? row.at("sync_events") == "1" : false },
            { "is_primary", row.count("isPrimary") ? row.at("isPrimary") == "1" : false },
            { "access_level", row.count("calendar_access_level")
                                ? row.at("calendar_access_level")
                                : "" }
        };
        obj["metadata"] = {
            { "color", row.count("calendar_color") ? row.at("calendar_color") : "" },
            { "deleted", row.count("deleted") ? row.at("deleted") == "1" : false }
        };
        result.push_back(obj);
    }
    return result;
}

nlohmann::json PARSER::parseCalendarEvents(const std::string& artifactRawData) {
    auto rows = extractRows(artifactRawData);
    nlohmann::json result = nlohmann::json::array();

    for (const auto& row : rows) {
        nlohmann::json obj;
        obj["type"] = "calendar_event";
        obj["event_id"] = row.count("_id") ? row.at("_id") : "";
        obj["calendar_id"] = row.count("calendar_id") ? row.at("calendar_id") : "";
        obj["title"] = row.count("title") ? row.at("title") : "";
        obj["description"] = row.count("description") ? row.at("description") : "";
        obj["location"] = row.count("eventLocation") ? row.at("eventLocation") : "";
        obj["time"] = {
            { "start", row.count("dtstart") ? row.at("dtstart") : "" },
            { "end", row.count("dtend") ? row.at("dtend") : "" },
            { "timezone", row.count("eventTimezone") ? row.at("eventTimezone") : "" },
            { "all_day", row.count("allDay") ? row.at("allDay") == "1" : false }
        };
        obj["recurrence"] = {
            { "rule", row.count("rrule") ? row.at("rrule") : "" },
            { "last_date", row.count("lastDate") ? row.at("lastDate") : "" },
            { "original_id", row.count("original_id") ? row.at("original_id") : "" },
            { "original_instance_time",
                row.count("originalInstanceTime")
                    ? row.at("originalInstanceTime")
                    : "" }
        };
        obj["ownership"] = {
            { "organizer", row.count("organizer") ? row.at("organizer") : "" },
            { "has_attendee_data",
                row.count("hasAttendeeData")
                    ? row.at("hasAttendeeData") == "1"
                    : false },
            { "guests_can_modify",
                row.count("guestsCanModify")
                    ? row.at("guestsCanModify") == "1"
                    : false }
        };
        obj["alerts"] = {
            { "has_alarm",
                row.count("hasAlarm")
                    ? row.at("hasAlarm") == "1"
                    : false }
        };
        obj["metadata"] = {
            { "availability", row.count("availability") ? row.at("availability") : "" },
            { "access_level", row.count("accessLevel") ? row.at("accessLevel") : "" },
            { "status", row.count("status") ? row.at("status") : "" },
            { "deleted", row.count("deleted") ? row.at("deleted") == "1" : false },
            { "dirty", row.count("dirty") ? row.at("dirty") == "1" : false },
            { "sync_id", row.count("_sync_id") ? row.at("_sync_id") : "" }
        };
        result.push_back(obj);
    }
    return result;
}

nlohmann::json PARSER::parseCalendarWhen(const std::string& artifactRawData) {
    auto rows = extractRows(artifactRawData);
    nlohmann::json result = nlohmann::json::array();

    for (const auto& row : rows) {
        nlohmann::json obj;
        obj["type"] = "calendar_instance";
        obj["event_id"] = row.count("event_id") ? row.at("event_id") : "";
        obj["begin"] = row.count("begin") ? row.at("begin") : "";
        obj["end"] = row.count("end") ? row.at("end") : "";
        obj["title"] = row.count("title") ? row.at("title") : "";
        obj["location"] = row.count("eventLocation") ? row.at("eventLocation") : "";
        obj["all_day"] = row.count("allDay")
            ? row.at("allDay") == "1"
            : false;

        obj["status"] = row.count("status") ? row.at("status") : "";
        obj["availability"] = row.count("availability")
            ? row.at("availability")
            : "";

        obj["instance"] = {
            { "start_day", row.count("startDay") ? row.at("startDay") : "" },
            { "end_day", row.count("endDay") ? row.at("endDay") : "" },
            { "start_minute",
                row.count("startMinute")
                    ? row.at("startMinute")
                    : "" },
            { "end_minute",
                row.count("endMinute")
                    ? row.at("endMinute")
                    : "" }
        };
        result.push_back(obj);
    }
    return result;
}

nlohmann::json PARSER::parseCalendarAttendees(const std::string& artifactRawData) {
    auto rows = extractRows(artifactRawData);
    nlohmann::json result = nlohmann::json::array();

    for (const auto& row : rows) {
        nlohmann::json obj;
        obj["type"] = "calendar_attendee";
        obj["event_id"] = row.count("event_id") ? row.at("event_id") : "";
        obj["attendee_name"] = row.count("attendeeName") ? row.at("attendeeName") : "";
        obj["attendee_email"] = row.count("attendeeEmail") ? row.at("attendeeEmail") : "";
        obj["attendee_type"] = row.count("attendeeType") ? row.at("attendeeType") : "";
        obj["attendee_status"] = row.count("attendeeStatus") ? row.at("attendeeStatus") : "";
        obj["is_organizer"] = row.count("isOrganizer") ? row.at("isOrganizer") == "1" : false;
        obj["metadata"] = {
            { "relationship", row.count("relationship") ? row.at("relationship") : "" },
            { "deleted", row.count("deleted") ? row.at("deleted") == "1" : false }
        };
        result.push_back(obj);
    }
    return result;
}

nlohmann::json PARSER::parseCalendarReminders(const std::string& artifactRawData) {
    auto rows = extractRows(artifactRawData);
    nlohmann::json result = nlohmann::json::array();
    for (const auto& row : rows) {
        nlohmann::json obj;
        obj["type"] = "calendar_reminder";
        obj["event_id"] = row.count("event_id") ? row.at("event_id") : "";
        obj["minutes_before"] = row.count("minutes") ? row.at("minutes") : "";
        obj["method"] = row.count("method") ? row.at("method") : ""; // e.g., alert, email, etc.
        obj["metadata"] = {
            { "deleted", row.count("deleted") ? row.at("deleted") == "1" : false },
            { "created", row.count("created") ? row.at("created") : "" },
            { "modified", row.count("modified") ? row.at("modified") : "" }
        };
        result.push_back(obj);
    }
    return result;
}

nlohmann::json PARSER::parseCalendarExtendedProperties(const std::string& artifactRawData) {
    auto rows = extractRows(artifactRawData);
    nlohmann::json result = nlohmann::json::array();

    for (const auto& row : rows) {
        nlohmann::json obj;
        obj["type"] = "calendar_extended_property";
        obj["event_id"] = row.count("event_id") ? row.at("event_id") : "";
        obj["name"] = row.count("name") ? row.at("name") : "";
        obj["value"] = row.count("value") ? row.at("value") : "";
        obj["metadata"] = {
            { "deleted", row.count("deleted") ? row.at("deleted") == "1" : false },
            { "_id", row.count("_id") ? row.at("_id") : "" }
        };
        result.push_back(obj);
    }
    return result;
}



// ===================================== PUBLIC =====================================



nlohmann::json PARSER::mergeCalendarArtifacts(
    const nlohmann::json& calendars_json,
    const nlohmann::json& events_json,
    const nlohmann::json& calendarWhen_json,
    const nlohmann::json& attendees_json,
    const nlohmann::json& reminders_json,
    const nlohmann::json& extendedproperties_json) {

    nlohmann::json merged = nlohmann::json::array();

    // Index events by calendar_id for fast lookup
    std::unordered_map<std::string, std::vector<nlohmann::json>> calendarEventsMap;
    for (const auto& event : events_json) {
        std::string cal_id = event.value("calendar_id", "");
        calendarEventsMap[cal_id].push_back(event);
    }

    // Index instances by event_id
    std::unordered_map<std::string, std::vector<nlohmann::json>> instancesMap;
    for (const auto& instance : calendarWhen_json) {
        std::string event_id = instance.value("event_id", "");
        instancesMap[event_id].push_back(instance);
    }

    // Index reminders by event_id
    std::unordered_map<std::string, std::vector<nlohmann::json>> remindersMap;
    for (const auto& reminder : reminders_json) {
        std::string event_id = reminder.value("event_id", "");
        remindersMap[event_id].push_back(reminder);
    }

    // Index extended properties by event_id
    std::unordered_map<std::string, std::vector<nlohmann::json>> extendedMap;
    for (const auto& ext : extendedproperties_json) {
        std::string event_id = ext.value("event_id", "");
        extendedMap[event_id].push_back(ext);
    }

    // Index attendees by event_id
    std::unordered_map<std::string, std::vector<nlohmann::json>> attendeesMap;
    for (const auto& attendee : attendees_json) {
        std::string event_id = attendee.value("event_id", "");
        attendeesMap[event_id].push_back(attendee);
    }

    // Merge everything under calendars
    for (const auto& calendar : calendars_json) {
        nlohmann::json calObj = calendar;
        calObj["events"] = nlohmann::json::array();

        std::string cal_id = calendar.value("calendar_id", "");

        if (calendarEventsMap.count(cal_id)) {
            for (auto event : calendarEventsMap[cal_id]) {
                std::string event_id = event.value("event_id", "");

                // Attach instances
                if (instancesMap.count(event_id))
                    event["instances"] = instancesMap[event_id];
                else
                    event["instances"] = nlohmann::json::array();

                // Attach reminders
                if (remindersMap.count(event_id))
                    event["reminders"] = remindersMap[event_id];
                else
                    event["reminders"] = nlohmann::json::array();

                // Attach extended properties
                if (extendedMap.count(event_id))
                    event["extended_properties"] = extendedMap[event_id];
                else
                    event["extended_properties"] = nlohmann::json::array();

                // Attach attendees
                if (attendeesMap.count(event_id))
                    event["attendees"] = attendeesMap[event_id];
                else
                    event["attendees"] = nlohmann::json::array();

                calObj["events"].push_back(event);
            }
        }

        merged.push_back(calObj);
    }

    return merged;
}


nlohmann::json PARSER::parseAdbDeviceInfo(const std::string& deviceInfoFromAdb) {

    std::istringstream stream(deviceInfoFromAdb);
    std::string line;
    nlohmann::json deviceInfo;

    while (std::getline(stream, line)) {
        // Expected format: [key]: [value]
        size_t keyStart = line.find('[');
        size_t keyEnd = line.find(']');
        size_t valueStart = line.find('[', keyEnd);
        size_t valueEnd = line.find(']', valueStart);
        if (keyStart == std::string::npos || keyEnd == std::string::npos ||
            valueStart == std::string::npos || valueEnd == std::string::npos) {
            continue;
        }
        std::string key = line.substr(keyStart + 1, keyEnd - keyStart - 1);
        std::string value = line.substr(valueStart + 1, valueEnd - valueStart - 1);
        deviceInfo[key] = value;
    }
    return deviceInfo;
}

void PARSER::saveJSONToFile(const nlohmann::json& data, const std::string& outputFile) {
    std::ofstream file(outputFile);
    if (!file.is_open()) {
        std::cerr << "[-] Failed to open file: " << outputFile << std::endl;
        return;
    }
    file << data.dump(4); // pretty print
    file.close();
}

nlohmann::json PARSER::parseArtifact(const std::string& artifactRawData, DataType type) {
    switch (type) {
    case DataType::SMS:
        return parseSMS(artifactRawData);

    case DataType::CALL:
        return parseCallLogs(artifactRawData);

    case DataType::USER_INSTALLED_APPS:
        return parseInstalledApps(artifactRawData);

    case DataType::MEDIA:
        return parseMedia(artifactRawData);

    case DataType::CONTACTS:
        return parseContacts(artifactRawData);

    case DataType::CALENDAR:
        return parseCalendar(artifactRawData);

    case DataType::EVENTS:
        return parseCalendarEvents(artifactRawData);

    case DataType::WHEN:
        return parseCalendarWhen(artifactRawData);

    case DataType::ATTENDEES:
        return parseCalendarAttendees(artifactRawData);

    case DataType::REMINDERS:
        return parseCalendarReminders(artifactRawData);

    case DataType::EXTENDED_PROPERTIES:
        return parseCalendarExtendedProperties(artifactRawData);

    default:
        return nlohmann::json::array();
    }
}

nlohmann::json PARSER::flattenForensicCalendar(const nlohmann::json& mergedCalendars) {
    
    nlohmann::json forensicTimeline = nlohmann::json::array();
    for (const auto& calendar : mergedCalendars) {
        std::string calName = calendar.value("display_name", "");
        std::string calAccount = calendar["account"].value("name", "");
        if (!calendar.contains("events")) continue;
        for (const auto& event : calendar["events"]) {
            std::string title = event.value("title", "");
            std::string description = event.value("description", "");
            std::string location = event.value("location", "");

            if (event.contains("instances") && !event["instances"].empty()) {
                for (const auto& instance : event["instances"]) {
                    nlohmann::json artifact;

                    artifact["calendar_name"] = calName;
                    artifact["calendar_account"] = calAccount;
                    artifact["event_title"] = title;
                    artifact["description"] = description;
                    artifact["location"] = location;

                    // Convert epoch to human-readable
                    artifact["occurrence"] = {
                        { "start", formatEpochMillis(instance.value("begin", "")) },
                        { "end", formatEpochMillis(instance.value("end", "")) },
                        { "all_day", instance.value("all_day", false) }
                    };

                    // Add reminders
                    if (event.contains("reminders")) {
                        nlohmann::json reminderArr = nlohmann::json::array();
                        for (const auto& r : event["reminders"]) {
                            reminderArr.push_back({
                                { "minutes_before", r.value("minutes_before", "") },
                                { "method", r.value("method", "") }
                                });
                        }
                        artifact["reminders"] = reminderArr;
                    }

                    // Add extended properties
                    if (event.contains("extended_properties")) {
                        nlohmann::json extendedArr = nlohmann::json::array();
                        for (const auto& e : event["extended_properties"]) {
                            extendedArr.push_back({
                                { "name", e.value("name", "") },
                                { "value", e.value("value", "") }
                                });
                        }
                        artifact["extended_properties"] = extendedArr;
                    }

                    // Add attendees
                    if (event.contains("attendees")) {
                        nlohmann::json attendeeArr = nlohmann::json::array();
                        for (const auto& a : event["attendees"]) {
                            attendeeArr.push_back({
                                { "name", a.value("attendee_name", "") },
                                { "email", a.value("attendee_email", "") },
                                { "is_organizer", a.value("is_organizer", false) },
                                { "status", a.value("attendee_status", "") }
                                });
                        }
                        artifact["attendees"] = attendeeArr;
                    }

                    forensicTimeline.push_back(artifact);
                }
            }
            else {
                // No instances, just the event itself
                nlohmann::json artifact;
                artifact["calendar_name"] = calName;
                artifact["calendar_account"] = calAccount;
                artifact["event_title"] = title;
                artifact["description"] = description;
                artifact["location"] = location;
                artifact["occurrence"] = nullptr;
                artifact["reminders"] = nlohmann::json::array();
                artifact["extended_properties"] = nlohmann::json::array();
                artifact["attendees"] = nlohmann::json::array();

                forensicTimeline.push_back(artifact);
            }
        }
    }
    return forensicTimeline;
}
