#pragma once
#include "types.h"

#include <vector>
#include <string>

namespace fsm {

class Reporter {
public:
    static std::string generateCsv(const std::vector<Anomaly>& anomalies, Timestamp lastTimestamp);
    static void saveToFile(const std::vector<Anomaly>& anomalies,
                          Timestamp lastTimestamp,
                          const std::string& filename);

private:
    static std::string formatTimestamp(Timestamp ts);
    static std::string formatDuration(std::chrono::milliseconds duration);
};

} // namespace fsm
