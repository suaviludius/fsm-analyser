/*
 * @license
 * (C) PROTEI protei.ru
 */
#include "Analyzer.h"
#include "Reporter.h"

#include <iostream>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0]
                  << " <log_file> <end_states_file> [output_file]"
                  << std::endl;
        return 1;
    }

    std::string logFile = argv[1];
    std::string endStatesFile = argv[2];
    std::string outputFile = (argc > 3) ? argv[3] : "report.csv";

    try {
        auto startTime = std::chrono::steady_clock::now();

        fsm::Analyzer analyzer;

        // Загружаем конечные состояния
        analyzer.loadEndStates(endStatesFile);

        // Анализируем лог
        analyzer.analyze(logFile);

        // Получаем аномалии
        const auto& anomalies = analyzer.getAnomalies();

        // Сохраняем отчет
        fsm::Reporter::saveToFile(anomalies, outputFile);

        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        std::cout << "\n\nAnalysis complete!" << std::endl;
        std::cout << "Found anomalies: " << anomalies.size() << std::endl;
        std::cout << "Report saved to: " << outputFile << std::endl;
        std::cout << "Time taken: " << duration.count() << " ms" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
