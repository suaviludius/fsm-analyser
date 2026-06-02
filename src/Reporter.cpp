#include "Reporter.h"

#include <sstream>
#include <iomanip>
#include <fstream>
#include <chrono>

namespace fsm {

void Reporter::formatDuration(std::chrono::milliseconds duration, std::string& result) {
    auto ms = duration.count();
    auto hours = ms / 3600000;
    ms %= 3600000;
    auto minutes = ms / 60000;
    ms %= 60000;
    auto seconds = ms / 1000;
    auto millis = ms % 1000;

    result.push_back('0' + (hours / 10));
    result.push_back('0' + (hours % 10));
    result.push_back(':');
    result.push_back('0' + (minutes / 10));
    result.push_back('0' + (minutes % 10));
    result.push_back(':');
    result.push_back('0' + (seconds / 10));
    result.push_back('0' + (seconds % 10));
    // Тут внимательно! Идет господин точка!
    result.push_back('.');
    result.push_back('0' + (millis / 100));
    result.push_back('0' + ((millis / 10) % 10));
    result.push_back('0' + (millis % 10));
}

std::string Reporter::generateCsv(const std::vector<Anomaly>& anomalies) {
    if (anomalies.empty()) return {};

    size_t estimatedSize = 0;

    for (const auto& a : anomalies) {
        estimatedSize += a.timestamp.size() + a.machineName.size()+a.state.size() + a.lastMessage.size() + 35;
    }

    std::string result;
    result.reserve(estimatedSize);

    char numBuffer[32];

    for (const auto& a : anomalies) {
        result += a.timestamp;
        result += ',';
        result += a.machineName;
        result += ',';
        // machineId идет целиком
        result += std::to_string(a.machineId);
        result += ',';
        result += a.state;
        result += ',';
        result += a.lastMessage;
        result += ',';

        formatDuration(a.duration, result);
        result += '\n';
    }

    return result;
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
