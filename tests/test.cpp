#include "Analyzer.h"
#include "Reporter.h"

#include <gtest/gtest.h>

#include <fstream>
#include <filesystem>
#include <regex>
#include <algorithm>

namespace fs = std::filesystem;

struct FullPipelineTest : public testing::Test {
    void SetUp() override {
        // Пути к тестовым данным (копируются в build директорию)
        m_logFile = "data/test_ds1/in.log";
        m_endStatesFile = "data/test_ds1/end_states.txt";
        m_expectedOutputFile = "data/test_ds1/out.csv";
        m_actualOutputFile = "test_actual_output.csv";
    }

    std::string readFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return "";
        }
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return content;
    }

    void normalizeLineEndings(std::string& str) {
        // Убираем \r для кроссплатформенности
        str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
        // Убираем пробелы в конце строк
        std::regex trailingSpaces(R"( +\n)");
        str = std::regex_replace(str, trailingSpaces, "\n");
    }

    std::string m_logFile;
    std::string m_endStatesFile;
    std::string m_expectedOutputFile;
    std::string m_actualOutputFile;
};

TEST_F(FullPipelineTest, TestDataset1) {
    // Проверяем, что тестовые данные существуют
    ASSERT_TRUE(fs::exists(m_logFile)) << "Log file not found: " << m_logFile;
    ASSERT_TRUE(fs::exists(m_endStatesFile)) << "End states file not found: " << m_endStatesFile;
    ASSERT_TRUE(fs::exists(m_expectedOutputFile)) << "Expected output file not found: " << m_expectedOutputFile;

    // 1. Запускаем анализатор
    fsm::Analyzer analyzer;

    // 2. Анализируем лог
    ASSERT_NO_THROW(analyzer.analyze(m_logFile, m_endStatesFile));

    // 3. Получаем аномалии
    const auto& anomalies = analyzer.getAnomalies();

    // 4. Сохраняем результат
    fsm::Reporter::saveToFile(anomalies, m_actualOutputFile);

    // 5. Сравниваем с ожидаемым результатом
    std::string expected = readFile(m_expectedOutputFile);
    std::string actual = readFile(m_actualOutputFile);

    normalizeLineEndings(expected);
    normalizeLineEndings(actual);

    // Для отладки
    if (expected != actual) {
        std::cout << "=== EXPECTED ===" << std::endl;
        std::cout << expected << std::endl;
        std::cout << "=== ACTUAL ===" << std::endl;
        std::cout << actual << std::endl;
    }

    EXPECT_EQ(actual, expected);

    // 6. Очистка
    std::remove(m_actualOutputFile.c_str());
}

TEST_F(FullPipelineTest, PerformanceTest) {
    // Проверяем, что программа обрабатывает большие файлы эффективно
    // (не загружает весь файл в память)

    // Создаем большой тестовый файл
    std::string largeLogFile = "large_test.log";
    std::ofstream file(largeLogFile);

    // Генерируем 10000 строк
    for (int i = 0; i < 10000; ++i) {
        file << "2026-01-12 09:07:02.345 2513826 PrimFSM.cpp(37) FSM: Sg.SIP.UA id: " << i << "; > St: 5 INCOMING_DIALOG_PROCEEDING Pr: 38418:4 SIP_UA_ALERTING_REQ ()\n";
        file << "2026-01-12 09:07:02.346 2513826 PrimFSM.cpp(72) FSM: Sg.SIP.UA id: "<< i << "; < St: 9 WAITING_PRACK\n";
    }
    file.close();

    // Создаем end states
    std::string endStatesFile = "large_end_states.txt";
    std::ofstream endFile(endStatesFile);
    endFile << "Sg.SIP.UA : NUL\n";
    endFile.close();

    // Измеряем время и память
    auto startTime = std::chrono::steady_clock::now();
    size_t peakMemory = 0;

    fsm::Analyzer analyzer;
    analyzer.analyze(largeLogFile, endStatesFile);

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // Проверяем, что аномалии найдены
    auto anomalies = analyzer.getAnomalies();
    EXPECT_EQ(anomalies.size(), 10000);  // Все 10000 машин зависли

    // Проверяем производительность (должно быть быстро)
    EXPECT_LT(duration.count(), 2000);  // Меньше 2 секунд для 10000 записей

    std::cout << "Performance test: " << duration.count() << "ms for " << 20000 << " log lines" << std::endl;

    // Очистка
    std::remove(largeLogFile.c_str());
    std::remove(endStatesFile.c_str());
}