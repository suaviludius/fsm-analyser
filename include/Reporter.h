#pragma once
#include "types.h"

#include <vector>
#include <string>

namespace fsm {

class Reporter {
public:
    static std::string generateCsv(const std::vector<Anomaly>& anomalies);
    static void saveToFile(const std::vector<Anomaly>& anomalies, const std::string& filename);

private:
    static std::string formatDuration(std::chrono::milliseconds duration);
};

} // namespace fsm
