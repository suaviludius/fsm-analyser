#include "Analyzer.h"

#include <chrono>
#include <thread>
#include <iomanip>

namespace fsm {

Analyzer::Analyzer() {
    m_lastProgressUpdate = std::chrono::steady_clock::now();
}

void Analyzer::loadEndStates(const std::string& filename) {
    m_parser.loadEndStates(filename);
}

void Analyzer::analyze(const std::string& logFile) {
    std::ifstream file(logFile);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + logFile);
    }

    // Первый проход: считаем количество строк (для прогресса)
    std::string line;
    while (std::getline(file, line)) {
        ++m_totalLines;
    }

    // Возвращаемся в начало
    file.clear();
    file.seekg(0);

    // Второй проход: обработка
    m_processedLines = 0;
    while (std::getline(file, line)) {
        ++m_processedLines;

        auto result = m_parser.parse(line);

        if (result.type == ParseResult::STATE_CHANGE) {
            processStateChange(result);
        } else if (result.type == ParseResult::MESSAGE) {
            processMessage(result);
        }

        // Обновляем прогресс каждые 1000 строк
        if (m_processedLines % 100 == 0) {
            auto now = std::chrono::steady_clock::now();
            if (now - m_lastProgressUpdate > std::chrono::milliseconds(500)) {
                m_progress = static_cast<int>(100.0 * m_processedLines / m_totalLines);
                printProgress();
                m_lastProgressUpdate = now;
            }
        }
    }

    // Финальная проверка на зависшие FSM
    detectStuckMachines();

    // Завершаем прогресс
    m_progress = 100;
    printProgress();
    std::cout << std::endl;
}

void Analyzer::processStateChange(const ParseResult& result) {
    MachineKey key{result.machineName, result.machineId};

    auto& info = m_machines[key];
    info.currentState = result.newState;
    info.lastUpdate = result.timestamp;
    info.isTerminal = m_parser.isTerminalState(info.currentState, result.machineName);
}

// Комментарии поставлены в условие что терминальное состояние
// не требуется отслеживать от Incoming Message

void Analyzer::processMessage(const ParseResult& result) {
    MachineKey key{result.machineName, result.machineId};

    auto& info = m_machines[key];
    //info.currentState = result.currentState;
    info.lastMessage = result.incomingMessage;
    //info.lastUpdate = result.timestamp;
    //info.isTerminal = m_parser.isTerminalState(info.currentState, result.machineName);
}

void Analyzer::detectStuckMachines() {
    // Находим последний timestamp среди всех машин
    std::string lastTimestampStr;
    uint64_t lastTimestampMs = 0;

    for (const auto& [key, info] : m_machines) {
        uint64_t tsMs = m_parser.parseTimestamp(info.lastUpdate);
        if (tsMs > lastTimestampMs) {
            lastTimestampMs = tsMs;
            lastTimestampStr = info.lastUpdate;
        }
    }

    // Если машин нет, используем текущее время
    if (lastTimestampMs == 0) {
        return;
    }

    for (const auto& [key, info] : m_machines) {
        if (!info.isTerminal) {
            // Парсим lastUpdate ТОЛЬКО для аномалии
            uint64_t lastUpdateMs = m_parser.parseTimestamp(info.lastUpdate);

            Anomaly anomaly;
            anomaly.timestamp = info.lastUpdate;
            anomaly.machineName = key.name;
            anomaly.machineId = key.id;
            anomaly.state = info.currentState;
            anomaly.lastMessage = info.lastMessage;
            anomaly.duration = std::chrono::milliseconds(lastTimestampMs - lastUpdateMs);

            m_anomalies.push_back(anomaly);
        }
    }
}

void Analyzer::printProgress() const {
    std::cout << "\rProcessing logs: " << m_progress << "% complete" << std::flush;
}

} // namespace fsm
