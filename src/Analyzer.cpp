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

        if (result.type == ParseResult::Type::STATE_CHANGE) {
            processStateChange(result);
        } else if (result.type == ParseResult::Type::MESSAGE) {
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
    // Используем последний известный timestamp из лога
    if (!m_lastTimestamp.has_value()) {
        m_lastTimestamp = std::chrono::system_clock::now();
    }
    detectStuckMachines(m_lastTimestamp.value());

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

    m_lastTimestamp = result.timestamp;
}

void Analyzer::processMessage(const ParseResult& result) {
    MachineKey key{result.machineName, result.machineId};

    auto& info = m_machines[key];
    info.currentState = result.currentState;
    info.lastMessage = result.incomingMessage;
    info.lastUpdate = result.timestamp;
    info.isTerminal = m_parser.isTerminalState(info.currentState, result.machineName);

    m_lastTimestamp = result.timestamp;
}

void Analyzer::detectStuckMachines(Timestamp lastTimestamp) {
    for (const auto& [key, info] : m_machines) {
        // Если не достигло конечного состояния - зависло
        if (!info.isTerminal) {
            Anomaly anomaly;
            anomaly.timestamp = info.lastUpdate;
            anomaly.machineName = key.name;
            anomaly.machineId = key.id;
            anomaly.state = info.currentState;
            anomaly.lastMessage = info.lastMessage;
            anomaly.duration = std::chrono::duration_cast<std::chrono::milliseconds>(lastTimestamp - info.lastUpdate);

            m_anomalies.push_back(anomaly);
        }
    }
}

void Analyzer::printProgress() const {
    std::cout << "\rProcessing logs: " << m_progress << "% complete" << std::flush;
}

} // namespace fsm
