#include "Parser.h"

#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string_view>

namespace fsm {

// Быстрый парсинг входящего сообщения
// Формат: "2026-01-12 09:07:02.345 ... FSM: Sg.SIP.UA id: 2621; > St: 5 INCOMING_DIALOG_PROCEEDING Pr: 38418:4 SIP_UA_ALERTING_REQ ()"
bool LogParser::parseMessageFast(const std::string& line, ParseResult& result) {
    // 1. Проверяем минимальную длину (timestamp = 23 символа)
    if (line.size() < 23) return false;
    
    // 2. Парсим timestamp (первые 23 символа)
    result.timestamp = parseTimestamp(line.substr(0, 23));
    
    // 3. Ищем "FSM: "
    auto fsmPos = line.find("FSM: ");
    if (fsmPos == std::string::npos) return false;
    
    // 4. Извлекаем machineName (до " id: ")
    auto idPos = line.find(" id: ", fsmPos);
    if (idPos == std::string::npos) return false;
    result.machineName = line.substr(fsmPos + 5, idPos - fsmPos - 5);
    
    // 5. Извлекаем machineId
    auto semicolonPos = line.find(";", idPos);
    if (semicolonPos == std::string::npos) return false;
    try {
        result.machineId = std::stoull(line.substr(idPos + 5, semicolonPos - idPos - 5));
    } catch (...) {
        return false;
    }
    
    // 6. Ищем "> St: " (признак входящего сообщения)
    auto stPos = line.find("> St: ", semicolonPos);
    if (stPos == std::string::npos) return false;
    
    // 7. Извлекаем текущее состояние (число + имя)
    auto stateNumEnd = line.find(' ', stPos + 6);
    if (stateNumEnd == std::string::npos) return false;
    
    auto stateNameStart = stateNumEnd + 1;
    auto stateNameEnd = line.find(' ', stateNameStart);
    if (stateNameEnd == std::string::npos) return false;
    result.currentState = line.substr(stateNameStart, stateNameEnd - stateNameStart);
    
    // 8. Ищем " Pr: " (приоритет)
    auto prPos = line.find(" Pr: ", stateNameEnd);
    if (prPos == std::string::npos) return false;
    
    // 9. Пропускаем "Pr: X:Y " (приоритет в формате число:число)
    auto priorityColon1 = line.find(':', prPos);
    if (priorityColon1 == std::string::npos) return false;
    auto priorityColon2 = line.find(':', priorityColon1 + 1);
    if (priorityColon2 == std::string::npos) return false;
    auto priorityEnd = line.find(' ', priorityColon2);
    if (priorityEnd == std::string::npos) return false;
    
    // 10. Извлекаем incoming message (после приоритета, до "(")
    auto msgStart = priorityEnd + 1;
    auto msgEnd = line.find(" (", msgStart);
    if (msgEnd == std::string::npos) msgEnd = line.size();
    result.incomingMessage = line.substr(msgStart, msgEnd - msgStart);
    
    result.type = ParseResult::Type::MESSAGE;
    return true;
}

// Быстрый парсинг смены состояния
// Формат: "2026-01-12 09:07:02.345 ... FSM: Sg.SIP.UA id: 2621; < St: 9 WAITING_PRACK"
bool LogParser::parseStateChangeFast(const std::string& line, ParseResult& result) {
    // 1. Проверяем минимальную длину
    if (line.size() < 23) return false;
    
    // 2. Парсим timestamp
    result.timestamp = parseTimestamp(line.substr(0, 23));
    
    // 3. Ищем "FSM: "
    auto fsmPos = line.find("FSM: ");
    if (fsmPos == std::string::npos) return false;
    
    // 4. Извлекаем machineName
    auto idPos = line.find(" id: ", fsmPos);
    if (idPos == std::string::npos) return false;
    result.machineName = line.substr(fsmPos + 5, idPos - fsmPos - 5);
    
    // 5. Извлекаем machineId
    auto semicolonPos = line.find(";", idPos);
    if (semicolonPos == std::string::npos) return false;
    try {
        result.machineId = std::stoull(line.substr(idPos + 5, semicolonPos - idPos - 5));
    } catch (...) {
        return false;
    }
    
    // 6. Ищем "< St: " (признак смены состояния)
    auto stPos = line.find("< St: ", semicolonPos);
    if (stPos == std::string::npos) return false;
    
    // 7. Извлекаем новое состояние (число + имя)
    auto stateNumEnd = line.find(' ', stPos + 6);
    if (stateNumEnd == std::string::npos) return false;
    
    auto stateNameStart = stateNumEnd + 1;
    auto stateNameEnd = line.find(' ', stateNameStart);
    if (stateNameEnd == std::string::npos) {
        result.newState = line.substr(stateNameStart);
    } else {
        result.newState = line.substr(stateNameStart, stateNameEnd - stateNameStart);
    }
    
    result.type = ParseResult::Type::STATE_CHANGE;
    return true;
}

Timestamp LogParser::parseTimestamp(const std::string& timestampStr) {
    std::tm tm = {};
    int milliseconds = 0;

    // Формат: "2026-01-12 09:07:02.345"
    std::istringstream ss(timestampStr);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

    // Извлекаем миллисекунды
    auto dotPos = timestampStr.find('.');
    if (dotPos != std::string::npos) {
        std::string msStr = timestampStr.substr(dotPos + 1);
        milliseconds = std::stoi(msStr);
    }

    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    tp += std::chrono::milliseconds(milliseconds);

    return tp;
}

std::string LogParser::extractMachineName(const std::string& line) {
    auto fsmPos = line.find("FSM: ");
    if (fsmPos == std::string::npos) return "";
    
    auto idPos = line.find(" id: ", fsmPos);
    if (idPos == std::string::npos) return "";
    
    return line.substr(fsmPos + 5, idPos - fsmPos - 5);
}

uint64_t LogParser::extractMachineId(const std::string& line) {
    auto idPos = line.find(" id: ");
    if (idPos == std::string::npos) return 0;
    
    auto semicolonPos = line.find(";", idPos);
    if (semicolonPos == std::string::npos) return 0;
    
    try {
        return std::stoull(line.substr(idPos + 5, semicolonPos - idPos - 5));
    } catch (...) {
        return 0;
    }
}

ParseResult LogParser::parse(const std::string& line) {
    ParseResult result;
    
    // Пробуем как входящее сообщение (содержит больше информации)
    if (parseMessageFast(line, result)) {
        return result;
    }

    // Пробуем как смену состояния
    if (parseStateChangeFast(line, result)) {
        return result;
    }
    
    result.type = ParseResult::Type::INVALID;
    return result;
}

void LogParser::loadEndStates(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        auto colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string machineName = line.substr(0, colonPos);
            std::string endState = line.substr(colonPos + 1);

            // Убираем пробелы
            machineName.erase(0, machineName.find_first_not_of(" \t"));
            machineName.erase(machineName.find_last_not_of(" \t") + 1);
            endState.erase(0, endState.find_first_not_of(" \t"));
            endState.erase(endState.find_last_not_of(" \t") + 1);

            m_endStates[machineName].insert(endState);
        }
    }
}

bool LogParser::isTerminalState(const std::string& state, const std::string& machineName) {
    auto it = m_endStates.find(machineName);
    if (it == m_endStates.end()) {
        return false;
    }
    return it->second.find(state) != it->second.end();
}

} // namespace fsm
