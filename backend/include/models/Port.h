#pragma once
#include <string>
#include <cstdint>

struct Port {
    long long id{};       // PK
    std::string name;     // унікальна назва
    std::string region;   // Europe/Asia/...
    double lat{};         // широта
    double lon{};         // довгота
};
