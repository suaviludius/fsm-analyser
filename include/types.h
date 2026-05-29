#pragma once
#include <string>
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>

namespace fsm {

// Формат: "2026-01-12 09:07:02.345" (23 символа)
using Timestamp = std::string;

// Составной ключ для FSM: имя + id
struct MachineKey {
    std::string name;
    uint64_t id;

    bool operator==(const MachineKey& other) const {
        return name == other.name && id == other.id;
    }
};

struct MachineKeyHash {
    size_t operator()(const MachineKey& k) const {
        return std::hash<std::string>()(k.name) ^ (std::hash<uint64_t>()(k.id) << 1);
    }
};

// Информация о FSM
struct MachineInfo {
    std::string currentState;
    std::string lastMessage;
    Timestamp lastUpdate;
    bool isTerminal = false;
};

// Аномалия
struct Anomaly {
    Timestamp timestamp;
    std::string machineName;
    uint64_t machineId;
    std::string state;
    std::string lastMessage;
    std::chrono::milliseconds duration;  // миллисекунды для точности
};

} // namespace fsm
