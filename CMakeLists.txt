cmake_minimum_required(VERSION 3.15)
project(FSMAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Основной исполняемый файл
add_executable(fsm_analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Analyzer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Reporter.cpp
)

target_include_directories(fsm_analyzer PUBLIC include)

# Google Test
option(BUILD_TEST "Build tests" ON)

message(STATUS "Build test: " BUILD_TEST)

if(BUILD_TEST)
    enable_testing()

    # Скачиваем Google Test
    # include(FetchContent)
    # FetchContent_Declare(
    #     googletest
    #    GIT_REPOSITORY https://github.com/google/googletest.git
    #    GIT_TAG v1.14.0
    # )
    #FetchContent_MakeAvailable(googletest)

    find_package(GTest REQUIRED)

    add_executable(run_tests tests/test.cpp)
    target_sources(run_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src/Analyzer.cpp
        ${CMAKE_SOURCE_DIR}/src/Parser.cpp
        ${CMAKE_SOURCE_DIR}/src/Analyzer.cpp
        ${CMAKE_SOURCE_DIR}/src/Reporter.cpp
    )
    target_link_libraries(run_tests PRIVATE GTest::gtest_main)
    target_include_directories(run_tests PUBLIC
        include
        data/test_ds1
        data/test_ds2
    )
    add_test(NAME FsmAnalyzerTests COMMAND run_tests)
endif()