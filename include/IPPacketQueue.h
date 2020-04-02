#pragma once

#include <cstdint>
#include <unordered_map>
#include <queue>

#include "Net.h"

class IPPacketQueue
{
public:
    class Entry 
    {
    public:
        Entry() { }
        Entry(const Interface& interface, const packet_t packet) :
            interface(interface), packet(packet) { }

        const Interface& GetInterface() const { return interface; }
        packet_t& GetPacket() { return packet; }

    private:
        Interface interface;
        packet_t packet;
    };

    void Add(const IP& destIP, const Interface& interface, const packet_t& packet);
    std::queue<Entry>& GetQueue(const IP& destIP);
    void SendQueuedPackets(const IP& destIP, const MAC& destMAC);

private:
    std::unordered_map<uint32_t, std::queue<Entry>> entries;
};