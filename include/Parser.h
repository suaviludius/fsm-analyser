#pragma once
#include "types.h"

#include <string>
#include <optional>
#include <regex>
#include <unordered_set>

namespace fsm {

// Результат парсинга строки лога
struct ParseResult {
    enum class Type { STATE_CHANGE, MESSAGE, INVALID };

    Type type = Type::INVALID;
    Timestamp timestamp;
    std::string machineName;
    uint64_t machineId = 0;

    // Для STATE_CHANGE
    std::string newState;

    // Для MESSAGE
    std::string currentState;
    std::string incomingMessage;
};



class LogParser {
public:
    LogParser();

    // Парсит одну строку лога
    ParseResult parse(const std::string& line);

    // Проверяет, является ли состояние терминальным
    bool isTerminalState(const std::string& state, const std::string& machineName);

    // Устанавливает конечные состояния из файла
    void loadEndStates(const std::string& filename);

private:
    // Регулярные выражения для быстрого парсинга
    std::regex m_stateChangeRegex;    // < St: XXX
    std::regex m_messageRegex;        // > St: XXX Pr: YYY
    std::regex m_timestampRegex;      // 2026-01-12 09:04:02.048

    // Кэш конечных состояний для каждого типа FSM
    std::unordered_map<std::string, std::unordered_set<std::string>> m_endStates;

    Timestamp parseTimestamp(const std::string& timestampStr);
    std::string extractMachineName(const std::string& line);
    uint64_t extractMachineId(const std::string& line);
};

} // namespace fsm
