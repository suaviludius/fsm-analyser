#pragma once
#include "types.h"
#include "Parser.h"

#include <unordered_map>
#include <fstream>
#include <iostream>

namespace fsm {

class Analyzer {
public:
    Analyzer() = default;

    void loadEndStates(const std::string& filename);
    void analyze(const std::string& logFile);
    const std::vector<Anomaly>& getAnomalies() const { return m_anomalies; }

private:
    void processStateChange(const ParseResult& result);
    void processMessage(const ParseResult& result);
    void detectStuckMachines();

    LogParser m_parser;
    std::unordered_map<MachineKey, MachineInfo, MachineKeyHash> m_machines;
    std::vector<Anomaly> m_anomalies;
    std::string m_lastTimestamp;

    size_t m_totalLines = 0;
    size_t m_processedLines = 0;
    int m_progress = 0;
};

} // namespace fsm
