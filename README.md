# FSM Anomaly Detector


Анализатор переходов конечных автоматов (Finite State Machine) для выявления аномальных паттернов поведения в логах телекоммуникационных систем.

> *Проект выполнен в рамках хакатона по C++ «Выход на связь»*  
> *18 апреля 2026 · Протей БЦ «Телеком» · Санкт-Петербург, Б. Сампсониевский пр., 60*


## Быстрый старт

```bash
# Клонирование репозитория
git clone https://github.com/suaviludius/fsm-analyzer.git
cd fsm-analyzer
# Сборка через Cmake
make
# Выполните запуск приложения по следующему формату:
./build_Release/fsm_analyzer <log_file> <end_states_file> [output_file]
```
**Зависимости**: C++17, CMake 3.15+, Linux

- `log_file` - путь к файлу с логами FSM
- `end_states_file` - путь к файлу с конечными состояниями
- `output_file`- путь для сохранения CSV отчета (необязательный)

## Формат входных данных

### log_file

Строки лога содержат два типа событий:

#### 1. Смена состояния (State Change)

```bash
2026-01-12 09:07:02.346 2513826 PrimFSM.cpp(72) FSM: Sg.SIP.UA id: 2621; < St: 9 WAITING_PRACK
```

**Поля:**
- `2026-01-12 09:07:02.346` - timestamp
- `Sg.SIP.UA` - имя FSM
- `2621` - ID экземпляра FSM
- `WAITING_PRACK` - новое состояние

#### 2. Входящее сообщение (Incoming Message)

```bash
2026-01-12 09:07:02.345 2513826 PrimFSM.cpp(37) FSM: Sg.SIP.UA id: 2621; > St: 5 INCOMING_DIALOG_PROCEEDING Pr: 38418:4 SIP_UA_ALERTING_REQ ()
```

**Поля:**
- `2026-01-12 09:07:02.345` - timestamp
- `Sg.SIP.UA` - имя FSM
- `2621` - ID экземпляра FSM
- `INCOMING_DIALOG_PROCEEDING` - текущее состояние
- `SIP_UA_ALERTING_REQ` - входящее сообщение

### end_states_file

Содержит перечень терминальных состояний для каждого типа FSM:
```bash
Sg.SIP.UA : NUL
```

**Поля:**
- `Sg.SIP.UA` - имя конечного автомата
- `NUL`- состояние, считающееся нормальным завершением

## Формат выходных данных

### output_file

```bash
# Пример строки из отчета
2026-01-12 09:07:02.346 Sg.SIP.UA 2621 WAITING_PRACK SIP_UA_ALERTING_REQ 00:00:00.000
```

- `2026-01-12 09:07:02.346` - время последнего изменения состояния
- `Sg.SIP.UA` - имя FSM
- `2621` - идентификатор экземпляра FSM
- `WAITING_PRACK` - текущее состояние FSM
- `SIP_UA_ALERTING_REQ` - последнее входящее сообщение
- `00:00:00.000` - время нахождения в данном состоянии

## Make команды

```bash
# Сборка
make                 # конфигурация и сборка
make config          # только конфигурация CMake
make build           # только сборка
make debug           # сборка в Debug режиме
make clean           # очистка сборки

# Запуск на тестовых даннных
make run-t1          # тестовый датасет 1
make run-t2          # тестовый датасет 2
make run-d11         # датасет 1, in1.log
make run-d12         # датасет 1, in2.log
make run-d13         # датасет 1, in3.log
make run-d2          # датасет 2
make run-d3          # датасет 3

# Свой запуск
make run LOG=<file> END=<file> OUT=<file>
# Пример: make run LOG=data/my.log END=data/end.txt OUT=res.csv

# Тесты и справка
make test            # запуск тестов
make help            # показать справку
```

## Тестирование

Проект содержит один тест для проверки корректности работы программы на тестовом датасете из дирректории data/test_ds1

```bash
# Запуск тестов через make
make test

# Пример ручного запуск
./build_Release/run_tests
```
