#include "IPPacketQueue.h"
#include "skel.h"

// Implementare cu unordered_map -> lookup in O(1)

void IPPacketQueue::Add(const IP& destIP, const Interface& interface, const packet_t& packet)
{
    entries[static_cast<uint32_t>(destIP)].emplace(interface, packet);
}

std::queue<IPPacketQueue::Entry>& IPPacketQueue::GetQueue(const IP& destIP)
{
    return entries[static_cast<uint32_t>(destIP)];
}

void IPPacketQueue::SendQueuedPackets(const IP& destIP, const MAC& destMAC)
{
    auto& queue = GetQueue(destIP);
    while (!queue.empty()) {
        auto& entry = queue.front();
        struct ether_header *eth_hdr = reinterpret_cast<struct ether_header *>(entry.GetPacket().payload);

        destMAC.CopyTo(eth_hdr->ether_dhost);

        send_packet(entry.GetInterface().GetIndex(), &entry.GetPacket());
        queue.pop();
    }
}
