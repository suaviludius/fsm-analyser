#include "Analyzer.h"

#include <sys/mman.h>
#include <sys/stat.h> // fstat
#include <fcntl.h>  // open
#include <unistd.h> // close
#include <chrono>
#include <thread>
#include <iomanip>
#include <string.h>
#include <iostream>

namespace fsm {


std::vector<std::pair<const char*, size_t>> Analyzer::splitIntoChunks(
        const char* data, size_t fileSize, int numChunks){

    std::vector<std::pair<const char*, size_t>> chunks;

    if (numChunks <= 1 || fileSize < 500 * 1024 * 1024) {
        // Для маленьких файлов - один чанк
        chunks.emplace_back(data, fileSize);
        return chunks;
    }

    // Размер чанка
    size_t idealChunkSize = fileSize / numChunks;
    size_t minChunkSize = idealChunkSize / 2;

    const char* currentStart = data;

    for (int i = 0; i < numChunks-1; ++i) {
        const char* chunkEnd = data + (i + 1) * idealChunkSize;

        // Ищем следующий '\n' (не реже минимального размера)
        const char* newline = chunkEnd;
        if (size_t(chunkEnd - currentStart) < minChunkSize) {
            newline = currentStart + minChunkSize;
        }

        // Ищем \n вперед
        while (newline < data + fileSize && *newline != '\n') {
            ++newline;
        }

        // Включаем \n в следующий чанк
        if (newline < data + fileSize) {
            ++newline;
        }

        size_t chunkSize = newline - currentStart;
        if (chunkSize > 0) {
            chunks.emplace_back(currentStart, chunkSize);
            currentStart = newline;
        }
    }

    // Последний чанк - остатки сладки
    if (currentStart < data + fileSize) {
        chunks.emplace_back(currentStart, data + fileSize - currentStart);
    }

    return chunks;
}


Analyzer::ThreadResult Analyzer::processChunk(
    const char* data, size_t size, size_t chunkId, std::atomic<size_t>& globalProcessedBytes) {
    ThreadResult result;

    // Счетчик байт для статус бара
    size_t processedBytes = 0;
    const size_t UPDATE_INTERVAL_BYTES = 1024 * 1024; // 1MB

    const char* p = data;
    const char* end = data + size;
    const char* lineStart = data;

    // Локальная мапа для этого потока (избегаем блокировок)
    result.machines.reserve(10000);

    while (p < end){
        // Ищем следующий '\n' от текущей позиции до конца буфера
        const char* newline = (const char*)memchr(p,'\n', end - p);
        // нет больше переводов строк
        if (!newline) break;

        std::string_view line(lineStart, newline - lineStart);

        // Обработка строки (без копирования)
        auto parseResult = m_parser.parse(line);

        if(parseResult.type != ParseResult::INVALID){
            result.lastTimestamp = std::string(parseResult.timestamp);
            if (parseResult.type == ParseResult::STATE_CHANGE) {
                processStateChange(parseResult, result.machines);
            } else if (parseResult.type == ParseResult::MESSAGE) {
                processMessage(parseResult, result.machines);
            }
        }

        // Считаем, сколько байт обработали
        size_t bytesDone = (newline + 1 - p);

        // Идем к указателю на следующий символ после '\n'
        lineStart = newline + 1;
        p = newline + 1;

        // Можно и по слипу
        processedBytes += bytesDone;
        if (processedBytes >= UPDATE_INTERVAL_BYTES) {
            globalProcessedBytes += processedBytes;
            processedBytes = 0;
        }
    }

    globalProcessedBytes += processedBytes;

    return result;
}

// Мержить результаты будем после отработки потоков
void Analyzer::mergeResults(std::vector<ThreadResult>& results) {
    // Выделяем память заранее
    size_t totalSize = 0;
    for (auto& result : results) {
        totalSize += result.machines.size();
    }
    m_machines.reserve(m_machines.size() + totalSize);

    // Результаты идут в последовательном порядке чтения из файлов
    // то есть вначале будут те результаты, что соответсвуют началу файла, дальше - ниже
    for (auto& result : results) {
        // Последний актуальный таймштамп запоминаем для поиска аномалий
        if(m_lastTimestamp < result.lastTimestamp) {
            m_lastTimestamp = result.lastTimestamp;
        }

        // Объединение мап- для каждого потока
        for (auto& [key, info] : result.machines) {
            m_machines[key] = std::move(info);
        }
    }
}


void Analyzer::analyze(const std::string& logFile, const std::string& endStatesFile, const std::string& outputFile) {
    // Загружаем конечные состояния
    m_parser.loadEndStates(endStatesFile);

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

    //auto start = std::chrono::steady_clock::now();

    // Мапим файл в память (СУПЕР ОЧЕНЬ БЫСТРО)
    char* data = (char*)mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
    // Этот системный вызов говорит ОС, что мы будем читать ВЕСЬ файл последовательно
    // и ОС должна читать его в фоне
    madvise(data, fileSize, MADV_WILLNEED | MADV_SEQUENTIAL);
    close(fd);

    if (data == MAP_FAILED) {
        throw std::runtime_error("mmap failed");
    }

    // Принудительно трогаем каждую страницу (для теста)
    //for (size_t i = 0; i < fileSize; i += 4096) {
    //    volatile char c = data[i];
    //    (void)c;
    //}
    //auto loadTime = std::chrono::steady_clock::now();

    //std::cout << "Load to page cache: " 
    //        << std::chrono::duration_cast<std::chrono::milliseconds>(loadTime - start).count()
    //        << " ms\n";

    // Если у нас файл > 500 Мб, то делим, иначе играемся в одном
    int numThreads = 0;
    if (fileSize < 500 * 1024 * 1024) {
        numThreads = 1;
    } else {
        // Оптимальное число потоков для I/O bound файла > 1Гб ~ (ядер - 1)
        numThreads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1);
    }

    // Сколько потоков, столько и будет чанков
    auto chunks = splitIntoChunks(data, fileSize, numThreads);

    std::vector<std::thread> threads;
    std::vector<ThreadResult> results(chunks.size());

    // Количество обработанных строк для статусбара
    std::atomic<size_t> processedBytes{0};
    std::atomic<bool> stopProgress{false};

    // Отдельный поток для прогресса
    std::thread progressThread([&processedBytes, fileSize, &stopProgress]() {
        size_t lastBytes = 0;
        while (!stopProgress) {
            size_t currentBytes = processedBytes.load();
            if (currentBytes != lastBytes) {
                int progress = static_cast<int>(100.0 * currentBytes / fileSize);
                std::cout << "\rProgress: " << progress << "%" << std::flush;
                lastBytes = currentBytes;
            }
        }
        std::cout << "\rProgress: 100%" << std::flush << "\n";
    });

    // Время пошло!
    auto startTime = std::chrono::steady_clock::now();

    for (size_t i = 0; i < chunks.size(); ++i) {
        threads.emplace_back([this, &results, &chunks, i, &processedBytes]() {
            results[i] = this->processChunk(chunks[i].first, chunks[i].second, i, processedBytes);
        });
    }

    // Ждем потоки
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    // Прогресс тоже останавливаем
    stopProgress = true;
    progressThread.join();

    // Объединяем результаты
    mergeResults(results);

    // Говорим ОС, что выделеные странички памяти можно освободить
    munmap(data, fileSize);

    // Время стоп!
    auto endTime = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    std::cout << "\nParsing processed\n"
              << "\n  bytes: "<< processedBytes.load()
              << "\n  seconds: " << duration.count()
              << "\n  speed: " << fileSize/(1024 * 1024)/std::max(1,(int)duration.count()) << "MB/s" << std::endl;;


    // Проверка на зависшие FSM
    detectStuckMachines(outputFile);
}

void Analyzer::processStateChange(const ParseResult& result,
    std::unordered_map<MachineKey, MachineInfo, MachineKeyHash>& machines) {
    MachineKey key{std::string(result.machineName), result.machineId};

    auto& info = machines[key];
    info.currentState = std::string(result.newState);
    info.lastUpdate = std::string(result.timestamp);
    // Только читает файл, поэтому потокобесзопасно!
    info.isTerminal = m_parser.isTerminalState(info.currentState, key.name);
}

void Analyzer::processMessage(const ParseResult& result, std::unordered_map<MachineKey, MachineInfo, MachineKeyHash>& machines) {
    MachineKey key{std::string(result.machineName), result.machineId};

    auto& info = machines[key];
    info.currentState = std::string(result.currentState);
    info.lastMessage = std::string(result.incomingMessage);
    info.lastUpdate = std::string(result.timestamp);
    info.isTerminal = m_parser.isTerminalState(info.currentState, key.name);
}

void Analyzer::detectStuckMachines(const std::string& outputFile) {
    std::ofstream file(outputFile, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create output file: " + outputFile);
    }

    // Устанавливаем буффер для файлового потока в user-space
    // Это поможет избежать многозатратных системных вызовов
    const size_t FILE_BUFFER_SIZE = 64 * 1024 * 1024; // 64 MB
    std::vector<char> fileBuffer(FILE_BUFFER_SIZE);
    file.rdbuf()->pubsetbuf(fileBuffer.data(), fileBuffer.size());

    // Есть ли хоть один timestamp?
    if (m_machines.empty() || m_lastTimestamp.empty()) {
        return;
    }

    // Аномалии делаем строками чтобы сразу выводить их в файл
    std::string anomalies;
    // Делаем локальный буффер не для всех записей, а для части, чтобы выводить по частям
    anomalies.reserve(128 * 1024); // 128 KB

    // Парсим последний timestamp один раз
    uint64_t lastTimestampMs = m_parser.parseTimestamp(m_lastTimestamp);

    for (const auto& [key, info] : m_machines) {
        if (!info.isTerminal) {
            // Парсим lastUpdate ТОЛЬКО для аномалии
            uint64_t lastUpdateMs = m_parser.parseTimestamp(info.lastUpdate);
            std::chrono::milliseconds duration = std::chrono::milliseconds(lastTimestampMs - lastUpdateMs);

            anomalies += info.lastUpdate;
            anomalies += ',';
            anomalies += key.name;
            anomalies += ',';
            // machineId идет целиком
            anomalies += std::to_string(key.id);
            anomalies += ',';
            anomalies += info.currentState;
            anomalies += ',';
            anomalies += info.lastMessage;
            anomalies += ',';
            LogParser::formatDuration(duration, anomalies);
            anomalies += '\n';

            ++m_anomaliesCount;

            // Сброс при заполнении
            if (anomalies.size() > 1000000) { // 1 MB
                file << anomalies;
                anomalies.clear();
            }
        }
    }

    // Финальный сброс
    if (!anomalies.empty()) {
        file << anomalies;
    }
}

} // namespace fsm
