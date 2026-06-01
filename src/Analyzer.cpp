#include "Analyzer.h"

#include <sys/mman.h>
#include <sys/stat.h> // fstat
#include <fcntl.h>  // open
#include <unistd.h> // close
#include <chrono>
#include <thread>
#include <iomanip>
#include <string.h>

namespace fsm {

Analyzer::Analyzer() {
    m_lastProgressUpdate = std::chrono::steady_clock::now();
}

void Analyzer::loadEndStates(const std::string& filename) {
    m_parser.loadEndStates(filename);
}

void Analyzer::analyze(const std::string& logFile) {
    // системный вызов для открывания файла (чисто для Unix/Linux)
    int fd = open(logFile.c_str(), O_RDONLY);
    if (fd == -1) {
        throw std::runtime_error("Cannot open file: " + logFile);
    }

    // Получаем размер этого файла
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        close(fd);
        throw std::runtime_error("Cannot get file size");
    }
    size_t fileSize = sb.st_size;

    // Мапим файл в память (СУПЕР ОЧЕНЬ БЫСТРО)
    char* data = (char*)mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (data == MAP_FAILED) {
        throw std::runtime_error("mmap failed");
    }

    // Анализируем напрямую в памяти
    const char* start = data;
    const char* end = data + fileSize;
    const char* lineStart = start;
    const char* p = start;

    auto startTime = std::chrono::steady_clock::now();

    while (p < end){
        // Ищем следующий '\n' от текущей позиции до конца буфера
        const char* newline = (const char*)memchr(p, '\n', end - p);
        // нет больше переводов строк
        if (!newline) break;

        std::string_view line(lineStart, newline - lineStart);
        ++m_processedLines;

        // Обработка строки (без копирования)
        auto result = m_parser.parse(line);

        if (result.type == ParseResult::STATE_CHANGE) {
            processStateChange(result);
        } else if (result.type == ParseResult::MESSAGE) {
            processMessage(result);
        }

        // Идем к указателю на следующий символ после '\n'
        lineStart = newline + 1;
        p = newline + 1;

        // Обновляем прогресс при изменении процента
        size_t bytesRead = p - start;
        int newProgress = static_cast<int>(100.0 * bytesRead / fileSize);
        if (newProgress > m_progress) {
            m_progress = newProgress;
            std::cout << "\rProgress: " << m_progress << "%" << std::flush;
        }

    }

    //Обработка последней строки (если нет \n в конце)
    // Пока пусть будет так
    if (lineStart < end) {
        std::string_view line(lineStart, end - lineStart);
        auto result = m_parser.parse(line);

        if (result.type == ParseResult::STATE_CHANGE) {
            processStateChange(result);
        } else if (result.type == ParseResult::MESSAGE) {
            processMessage(result);
        }

        // Обновляем прогресс последний раз
        std::cout << "\rProgress: 100%" << std::flush;
    }

    // Говорим ОС, что выделеные странички памяти можно освободить
    munmap(data, fileSize);

    auto endTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    std::cout << "\nData processed:"
              << "\nlines: "<< m_processedLines
              << "\nseconds: " << elapsed.count()
              << "\nspeed: " << fileSize/(1024 * 1024)/std::max(1,(int)elapsed.count()) << "MB/s";

    // Финальная проверка на зависшие FSM
    detectStuckMachines();

    std::cout << std::endl;
}

void Analyzer::processStateChange(const ParseResult& result) {
    MachineKey key{std::string(result.machineName), result.machineId};

    auto& info = m_machines[key];
    info.currentState = std::string(result.newState);
    info.lastUpdate = std::string(result.timestamp);
    info.isTerminal = m_parser.isTerminalState(info.currentState, key.name);
}

void Analyzer::processMessage(const ParseResult& result) {
    MachineKey key{std::string(result.machineName), result.machineId};

    auto& info = m_machines[key];
    info.currentState = std::string(result.currentState);
    info.lastMessage = std::string(result.incomingMessage);
    info.lastUpdate = std::string(result.timestamp);
    info.isTerminal = m_parser.isTerminalState(info.currentState, key.name);
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
