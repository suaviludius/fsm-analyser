#pragma once
#include <string>
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>

namespace fsm {

// Составной ключ для FSM: имя + id
struct MachineKey {
    std::string name;
    uint64_t id;

    bool operator==(const MachineKey& other) const {
        return name == other.name && id == other.id;
    }
};

// Хэш-функция для MachineKey
struct MachineKeyHash {
    size_t operator()(const MachineKey& k) const {
        return std::hash<std::string>{}(k.name) * 31 + std::hash<uint64_t>{}(k.id);
    }
};

// Информация о FSM
struct MachineInfo {
    std::string currentState;
    std::string lastMessage;
    std::string lastUpdate;
    bool isTerminal = false;
};

} // namespace fsm
