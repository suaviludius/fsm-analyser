#pragma once
#include "types.h"
#include "Parser.h"

#include <unordered_map>
#include <fstream>
#include <atomic>
#include <memory>

namespace fsm {

class Analyzer {
public:
    Analyzer() = default;

    // Выполнение анализа состояний лог файла
    void analyze(const std::string& logFile, const std::string& endStatesFile, const std::string& outputFile);

    const auto& getMachines() const { return m_machines; }
    const auto& getAnomaliesCount() const { return m_anomaliesCount; }

private:
    // Структура для обработки машинных состояний внутри разных потоков
    struct ThreadResult {
        std::unordered_map<MachineKey, MachineInfo, MachineKeyHash> machines;
        std::string lastTimestamp;
    };

    // Разделение файла на чанки
    std::vector<std::pair<const char*, size_t>> splitIntoChunks(const char* data, size_t fileSize, int numChunks);

    // Обработка файлового чанка в отдельном потоке
    ThreadResult processChunk(const char* data, size_t size, size_t chunkId, std::atomic<size_t>& globalProcessedBytes);

    void processStateChange(const ParseResult& result, std::unordered_map<MachineKey, MachineInfo, MachineKeyHash>& machines);
    void processMessage(const ParseResult& result, std::unordered_map<MachineKey, MachineInfo, MachineKeyHash>& machines);

    // Объединение результатов поиска машинных состояний из нескольких потоков
    void mergeResults(std::vector<ThreadResult>& results);

    // Определитель аномалий в m_machines (RVO)
    void detectStuckMachines(const std::string& outputFile);

    std::unordered_map<MachineKey, MachineInfo, MachineKeyHash> m_machines;
    size_t m_anomaliesCount = 0;
    LogParser m_parser;

    std::string m_lastTimestamp;
};

} // namespace fsm
