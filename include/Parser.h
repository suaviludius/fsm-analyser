#pragma once
#include "types.h"

#include <string>
#include <optional>
#include <string_view>
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
    LogParser() = default;

    // Парсит одну строку лога
    ParseResult parse(const std::string& line);

    // Проверяет, является ли состояние терминальным
    bool isTerminalState(const std::string& state, const std::string& machineName);

    // Устанавливает конечные состояния из файла
    void loadEndStates(const std::string& filename);

private:

    // Кэш конечных состояний для каждого типа FSM
    std::unordered_map<std::string, std::unordered_set<std::string>> m_endStates;

    Timestamp parseTimestamp(const std::string& timestampStr);
    std::string extractMachineName(const std::string& line);
    uint64_t extractMachineId(const std::string& line);
    
    // Оптимизированные методы парсинга
    bool parseMessageFast(const std::string& line, ParseResult& result);
    bool parseStateChangeFast(const std::string& line, ParseResult& result);
};

} // namespace fsm
