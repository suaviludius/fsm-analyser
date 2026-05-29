#pragma once
#include "types.h"
#include "Parser.h"

#include <unordered_map>
#include <fstream>
#include <iostream>

namespace fsm {

class Analyzer {
public:
    Analyzer();

    void loadEndStates(const std::string& filename);
    void analyze(const std::string& logFile);
    const std::vector<Anomaly>& getAnomalies() const { return m_anomalies; }
    int getProgress() const { return m_progress; }
    void printProgress() const;

private:
    void processStateChange(const ParseResult& result);
    void processMessage(const ParseResult& result);
    void detectStuckMachines();

    LogParser m_parser;
    std::unordered_map<MachineKey, MachineInfo, MachineKeyHash> m_machines;
    std::vector<Anomaly> m_anomalies;

    size_t m_totalLines = 0;
    size_t m_processedLines = 0;
    int m_progress = 0;
    std::chrono::steady_clock::time_point m_lastProgressUpdate;
};

} // namespace fsm
