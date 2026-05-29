#include "Reporter.h"

#include <sstream>
#include <iomanip>
#include <fstream>
#include <chrono>

namespace fsm {

std::string Reporter::formatDuration(std::chrono::milliseconds duration) {
    std::ostringstream oss;

    auto hours = duration / std::chrono::hours(1);
    duration %= std::chrono::hours(1);
    auto minutes = duration / std::chrono::minutes(1);
    duration %= std::chrono::minutes(1);
    auto seconds = duration / std::chrono::seconds(1);
    auto ms = duration % std::chrono::seconds(1);

    oss << std::setfill('0') << std::setw(2) << hours << ":"    // -> int
        << std::setfill('0') << std::setw(2) << minutes << ":"  // -> int
        << std::setfill('0') << std::setw(2) << seconds << "."  // -> int
        << std::setfill('0') << std::setw(3) << ms.count();     // -> chrono::milliseconds

    return oss.str();
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
