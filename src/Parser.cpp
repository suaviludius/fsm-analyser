#include "Parser.h"

#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>

namespace fsm {

LogParser::LogParser() {
    // Регулярное выражение для смены состояния: "< St: XXX"
    // Пример: "FSM: Sg.SIP.UA id: 2621; < St: 9 WAITING_PRACK"
    m_stateChangeRegex = std::regex(
        R"((\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}\.\d{3}).*FSM:\s+([^\s]+)\s+id:\s+(\d+);\s+<\s+St:\s+\d+\s+(\w+))"
    );

    // Регулярное выражение для входящего сообщения: "> St: XXX Pr: YYY"
    // Пример: "FSM: Sg.SIP.UA id: 2621; > St: 5 INCOMING_DIALOG_PROCEEDING Pr: 38418:4 SIP_UA_ALERTING_REQ ()"
    m_messageRegex = std::regex(
        R"((\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}\.\d{3}).*FSM:\s+([^\s]+)\s+id:\s+(\d+);\s+>\s+St:\s+\d+\s+(\w+)\s+Pr:\s+\d+:\d+\s+(\w+)\s*\([^)]*\))"
    );
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

ParseResult LogParser::parse(const std::string& line) {
    ParseResult result;

    // Пробуем как входящее сообщение (должно быть первым, так как содержит больше информации)
    std::smatch match;
    if (std::regex_search(line, match, m_messageRegex)) {
        result.type = ParseResult::Type::MESSAGE;
        result.timestamp = parseTimestamp(match[1]);
        result.machineName = match[2];
        result.machineId = std::stoull(match[3]);
        result.currentState = match[4];      // Текущее состояние
        result.incomingMessage = match[5];   // Входящее сообщение
        return result;
    }

    // Пробуем как смену состояния
    if (std::regex_search(line, match, m_stateChangeRegex)) {
        result.type = ParseResult::Type::STATE_CHANGE;
        result.timestamp = parseTimestamp(match[1]);
        result.machineName = match[2];
        result.machineId = std::stoull(match[3]);
        result.newState = match[4];          // Новое состояние
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
