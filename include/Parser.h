#pragma once
#include "types.h"

#include <string>
#include <optional>
#include <string>
#include <unordered_set>

namespace fsm {

// Результат парсинга строки лога
struct ParseResult {

    static constexpr uint8_t INVALID = 0;
    static constexpr uint8_t MESSAGE = 1;
    static constexpr uint8_t STATE_CHANGE = 2;

    // MESSAGE = "> St: ", STATE_CHANGE = "< St: "

    // 0= INVALID, 1= MESSAGE, 2= STATE_CHANGE
    uint8_t type = INVALID;

    std::string timestamp;
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

    // Парсит одну строку лога (любого вида)
    ParseResult parse(const std::string& line);

    // Проверяет, является ли состояние терминальным
    bool isTerminalState(const std::string& state, const std::string& machineName);

    // Устанавливает конечные состояния из файла
    // Читает один раз, так что сильно оптимизировать не стоит
    void loadEndStates(const std::string& filename);

    // Парсинг времени std::string -> std::chrono.count()
    uint64_t parseTimestamp(const std::string& timestampStr);

private:

    // Состояния парсера
    enum class State {
        START,
        TIMESTAMP,
        FSM_PREFIX,
        MACHINE_NAME,
        MACHINE_ID,
        ARROW_TYPE,      // > или <
        STATE_NUMBER,
        STATE_NAME,
        PRIORITY_PREFIX,
        PRIORITY_VALUE,
        MESSAGE_TEXT,
        DONE,
        ERROR
    };

    // Кэш конечных состояний для каждого типа FSM
    std::unordered_map<std::string, std::string> m_endStates;
};

} // namespace fsm
