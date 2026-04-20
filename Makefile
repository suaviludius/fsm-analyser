# Конфигурация
BUILD_TYPE := Release
CONFIG_TYPE := ${BUILD_TYPE}
BUILD_DIR := build_${BUILD_TYPE}
TARGET_EXEC := fsm_analyzer

# Общая команда
all: config build

# Конфигурация сборки CMake
# ! Изменил CMakeLists -> выполни config !
config:
		@rm -rf ${BUILD_DIR}/CMakeCache.txt ${BUILD_DIR}/CMakeFiles
		@mkdir -p ${BUILD_DIR}
		@cd ${BUILD_DIR} && cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE}

# Сборка проекта по чертежу в build directory
# ! Изменил .h или .cpp -> выполни build !
build:
		@cd ${BUILD_DIR} && cmake --build . --config ${CONFIG_TYPE}

# Запуск с кастомными аргументами
# Использование: make run LOG=data/my.log END=data/my_end.txt OUT=report.csv
run:
	@if [ -z "$(LOG)" ] || [ -z "$(END)" ]; then \
		echo "Ошибка: укажите LOG и END параметры!"; \
		echo "Пример: make run LOG=data/test.log END=data/end_states.txt OUT=report.csv"; \
		exit 1; \
	fi
	@./${BUILD_DIR}/fsm_analyzer $(LOG) $(END) $(OUT)

# Запуск приложения c тестовыми данными
run-t1:
		@./${BUILD_DIR}/fsm_analyzer data/test_ds1/in.log data/test_ds1/end_states.txt report.csv

run-t2:
		@./${BUILD_DIR}/fsm_analyzer data/test_ds2/in.log data/test_ds2/end_states.txt report.csv

# Запуск приложения c датасетами
run-d11:
		@./${BUILD_DIR}/fsm_analyzer data/datasets/1/in1.log data/datasets/1/end_states.txt report.csv

run-d12:
		@./${BUILD_DIR}/fsm_analyzer data/datasets/1/in2.log data/datasets/1/end_states.txt report.csv

run-d13:
		@./${BUILD_DIR}/fsm_analyzer data/datasets/1/in3.log data/datasets/1/end_states.txt report.csv

run-d2:
		@./${BUILD_DIR}/fsm_analyzer data/datasets/2/in.log data/datasets/2/end_states.txt report.csv

run-d3:
		@./${BUILD_DIR}/fsm_analyzer data/datasets/3/in.log data/datasets/3/end_states.txt report.csv

# Очистка директории сборки
clean:
		@rm -rf ${BUILD_DIR}

# Отладка (сборка в Debug режиме)
debug:
		@$(MAKE) config BUILD_TYPE=Debug
		@$(MAKE) build BUILD_TYPE=Debug

# Запуск собранных тестов
test:
		@if [ ! -f "${BUILD_DIR}/run_tests" ]; then \
			echo "Тесты не найдены. Соберите проект с параметром BUILD_TESTS=ON!"; \
			exit 1; \
		fi
		@./${BUILD_DIR}/run_tests

# Инструкция
help:
	@echo "  Доступные make команды"
	@echo ""
	@echo "  make           - конфигурация и сборка проекта"
	@echo "  make config    - создание конфигурации сборки CMake"
	@echo "  make build     - сборка проекта"
	@echo "  make clean     - очистка директории сборки"
	@echo "  make debug     - сборка в Debug режиме"
	@echo ""
	@echo "  make run-t1    - тестовый датасет 1"
	@echo "  make run-t2    - тестовый датасет 2"
	@echo ""
	@echo "  make run-d11   - датасет 1, файл in1.log"
	@echo "  make run-d12   - датасет 1, файл in2.log"
	@echo "  make run-d13   - датасет 1, файл in3.log"
	@echo "  make run-d2    - датасет 2"
	@echo "  make run-d3    - датасет 3"
	@echo ""
	@echo "  make run LOG=<file> END=<file> OUT=<file>"
	@echo "  Пример: make run LOG=data/my.log END=data/end.txt OUT=res.csv"
	@echo ""
	@echo "  make test      - запуск тестов"
	@echo "  make help      - показать эту справку"