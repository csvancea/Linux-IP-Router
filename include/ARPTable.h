#pragma once

#include <cstdint>
#include <unordered_map>

#include "Net.h"

class ARPTable
{
public:
    void Add(const IP& ip, const MAC& mac);
    bool GetMAC(const IP& ip, MAC& mac);

private:
    std::unordered_map<uint32_t, MAC> entries;
};