#include "Reporter.h"

#include <sstream>
#include <iomanip>
#include <fstream>
#include <chrono>

namespace fsm {

std::string Reporter::formatDuration(std::chrono::milliseconds duration) {
    auto ms = duration.count();

    auto hours = ms / 3600000;
    ms %= 3600000;
    auto minutes = ms / 60000;
    ms %= 60000;
    auto seconds = ms / 1000;
    auto millis = ms % 1000;

    // Предварительно выделяем буфер
    // Точный размер: 2+1+2+1+2+1+3 =12
    std::string result;
    result.reserve(12);

    // Часы
    result += (hours < 10 ? "0" : "") + std::to_string(hours);
    result += ':';

    // Минуты
    result += (minutes < 10 ? "0" : "") + std::to_string(minutes);
    result += ':';

    // Секунды
    result += (seconds < 10 ? "0" : "") + std::to_string(seconds);
    // Тут внимательно! Идет господин точка!
    result += '.';

    // Миллисекунды (всегда 3 цифры)
    if (millis < 100) result += '0';
    if (millis < 10) result += '0';
    result += std::to_string(millis);

    return result;
}

std::string Reporter::generateCsv(const std::vector<Anomaly>& anomalies) {
    std::ostringstream oss;

    for (const auto& anomaly : anomalies) {
        oss << anomaly.timestamp << ","
            << anomaly.machineName << ","
            << anomaly.machineId << ","
            << anomaly.state << ","
            << anomaly.lastMessage << ","
            << formatDuration(anomaly.duration) << "\n";
    }

    return oss.str();
}

void Reporter::saveToFile(const std::vector<Anomaly>& anomalies,
                         const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create output file: " + filename);
    }

    file << generateCsv(anomalies);
}

} // namespace fsm
