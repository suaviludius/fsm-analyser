#include "Parser.h"

#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <charconv> // для быстрого парсинга чисел!!!

namespace fsm {

// TODO: сделать optional будет хорошая идея?
ParseResult LogParser::parse(std::string_view line) {
    ParseResult result;

    // Предварительно выделяем память для строк
    result.timestamp.reserve(24);
    result.machineName.reserve(32);
    result.currentState.reserve(32);
    // Вот этих двух оставить тут или перенести в switch?
    //result.newState.reserve(32);
    //result.incomingMessage.reserve(128);

    State state = State::START;
    size_t pos = 0;
    size_t startPos = 0;

    // Временные переменные
    result.type = ParseResult::MESSAGE;
    int priorityDepth = 0;

    // один раз посчитали и хватит
    const size_t lineSize = line.size();

    while (pos <= lineSize && state != State::DONE && state != State::ERROR) {

        switch (state) {
            case State::START:
                if (pos == 0 && line.size() >= 23) {
                    result.timestamp = std::string(line.substr(0, 23));
                    pos = 23;  // После timestamp устанавливаем сами
                    state = State::TIMESTAMP;
                } else {
                    state = State::ERROR;
                }
                break;

            case State::TIMESTAMP:
                // Ищем " FSM: "
                if (lineSize >= pos + 6 && line.substr(pos, 6) == " FSM: ") {
                    pos += 6;
                    state = State::FSM_PREFIX;
                } else {
                    pos++;
                }
                break;

            case State::FSM_PREFIX:
                startPos = pos;
                state = State::MACHINE_NAME;
                pos++;
                break;

            case State::MACHINE_NAME:
                // Ищем " id: "
                if (pos + 4 <= lineSize && line.substr(pos, 4) == " id:") {
                    result.machineName = std::string(line.substr(startPos, pos - startPos));
                    pos += 4;
                    state = State::MACHINE_ID;
                } else {
                    pos++;
                }
                break;

            case State::MACHINE_ID:
                // Пропускаем пробелы после "id:"
                while (pos < lineSize && line[pos] == ' ') pos++;

                startPos = pos;

                // Парсим число до ';'
                while (pos < lineSize && line[pos] != ';') pos++;
                if (pos > startPos) {
                    uint64_t id = 0;
                    // Функция работает быстрее, чем std::stoull !!!
                    std::from_chars(line.data() + startPos, line.data() + pos, id);
                    result.machineId = id;
                    pos++; // пропускаем ';'
                    state = State::ARROW_TYPE;
                } else {
                    state = State::ERROR;
                }
                break;

            case State::ARROW_TYPE:
                // Пропускаем пробелы
                while (pos < lineSize && line[pos] == ' ') pos++;

                if (pos + 4 <= lineSize) {
                    if (line.substr(pos, 4) == "> St") {
                        result.type = ParseResult::MESSAGE;
                        pos += 4;
                        state = State::STATE_NUMBER;
                    } else if (line.substr(pos, 4) == "< St") {
                        result.type = ParseResult::STATE_CHANGE;
                        pos += 4;
                        state = State::STATE_NUMBER;
                    } else {
                        state = State::ERROR;
                    }
                } else {
                    state = State::ERROR;
                }
                break;

            // TODO: в идеале иметь стат мапу с номерами и названиями, чтобы читать только номера
            // но если заранее таблицы соответсвий нет, то делать её не будем
            // (просто сейчас в end_states даются состояния без номеров, поэтому оставим на будущее)

            // Сейчас скипаем номер и читаем только названия
            case State::STATE_NUMBER:
                // Пропускаем ": "
                if (pos + 2 <= lineSize && line.substr(pos, 2) == ": ") {
                    pos += 2;
                }
                // Пропускаем число
                while (pos < lineSize && std::isdigit(line[pos])) {
                    pos++;
                }
                // Пропускаем пробелы после числа
                while (pos < lineSize && line[pos] == ' ') {
                    pos++;
                }
                state = State::STATE_NAME;
                break;

            case State::STATE_NAME:
                startPos = pos;
                // Читаем имя состояния до пробела или конца строки
                while (pos < lineSize && line[pos] != ' ') pos++;

                if (result.type == ParseResult::MESSAGE) {
                    result.currentState = std::string(line.substr(startPos, pos - startPos));
                    state = State::PRIORITY_PREFIX;
                } else {
                    result.newState = std::string(line.substr(startPos, pos - startPos));
                    state = State::DONE;  // STATE_CHANGE завершён
                }
                break;

            // Эту информацию скипаем
            case State::PRIORITY_PREFIX:
                // Ищем " Pr: "
                if (pos + 4 <= lineSize && line.substr(pos, 4) == " Pr:") {
                    pos += 4;
                    state = State::PRIORITY_VALUE;
                } else {
                    pos++;
                }
                break;

            // Эту информацию опять скипаем
            case State::PRIORITY_VALUE:
                // Пропускаем "X:Y "
                while (pos < lineSize && line[pos] == ' ') pos++;

                priorityDepth = 0;
                while (pos < lineSize && line[pos] != ' ') {
                    if (line[pos] == ':') priorityDepth++;
                    pos++;
                }
                if (priorityDepth >= 1) {
                    state = State::MESSAGE_TEXT;
                } else {
                    state = State::ERROR;
                }
                break;

            case State::MESSAGE_TEXT:
                // Пропускаем пробелы
                while (pos < lineSize && line[pos] == ' ') pos++;

                startPos = pos;

                // Читаем сообщение до " (" или конца строки
                while (pos < lineSize) {
                    if (pos + 2 <= lineSize && line.substr(pos, 2) == " (") {
                        break;
                    }
                    pos++;
                }
                result.incomingMessage = std::string(line.substr(startPos, pos - startPos));
                state = State::DONE;
                break;

            default:
                state = State::ERROR;
                break;
        }

    }

    if(state != State::DONE) result.type = ParseResult::INVALID;

    return result;
}


uint64_t LogParser::parseTimestamp(const std::string& ts) {
    // Конвертируем строку "2026-01-12 09:07:02.345" в миллисекунды
    // Используем mktime для точности - вызываем ТОЛЬКО для аномалий
    std::tm tm = {};
    std::istringstream ss(ts);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

    auto dotPos = ts.find('.');
    int ms = 0;
    if (dotPos != std::string::npos) {
        ms = std::stoi(ts.substr(dotPos + 1));
    }

    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    tp += std::chrono::milliseconds(ms);

    return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
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

            m_endStates[std::move(machineName)] = std::move(endState);
        }
    }
}

bool LogParser::isTerminalState(const std::string& state, const std::string& machineName) {
    auto it = m_endStates.find(machineName);
    return it != m_endStates.end() && it->second == state;
}

} // namespace fsm
