#include "ARPTable.h"

// Implementare cu unordered_map -> lookup in O(1)

void ARPTable::Add(const IP& ip, const MAC& mac)
{
    entries[static_cast<uint32_t>(ip)] = mac;
}

bool ARPTable::GetMAC(const IP& ip, MAC& mac)
{
    auto search = entries.find(static_cast<uint32_t>(ip));
    if (search != entries.end()) {
        mac = search->second;
        return 1;
    }
    return 0;
}
