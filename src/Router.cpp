#include "ARPTable.h"
#include "RoutingTable.h"
#include "IPPacketQueue.h"
#include "Net.h"
#include "skel.h"

class Router
{
public:
    Router()
    {
        init();
        DIE(!routingTable.ParseFile("rtable.txt"), "Can't parse routing table");
    }

    void Run()
    {
        while (1) {
            packet_t packet;
            DIE(get_packet(&packet) < 0, "get_message");

            struct ether_header *eth_hdr = reinterpret_cast<struct ether_header *>(packet.payload);
            Interface recvInterface (packet.interface);

            switch (ntohs(eth_hdr->ether_type)) {
            case ETHERTYPE_ARP:
                HandleARPPacket(packet, recvInterface);
                break;
            case ETHERTYPE_IP:
                HandleIPPacket(packet, recvInterface);
                break;
            }
        }
    }

private:
    void HandleARPPacket(packet_t& packet, const Interface& recvInterface)
    {
        struct ether_header *eth_hdr = reinterpret_cast<struct ether_header *>(packet.payload);
        struct ether_arp *arp_hdr = reinterpret_cast<struct ether_arp *>(packet.payload + sizeof(struct ether_header));

        MAC senderMAC (arp_hdr->arp_sha);
        IP senderIP (arp_hdr->arp_spa);
        IP targetIP (arp_hdr->arp_tpa);

        switch (ntohs(arp_hdr->ea_hdr.ar_op)) {
        case ARPOP_REQUEST:
            if (targetIP == recvInterface.GetIP()) {
                // ARP Request destinat mie; trimis raspuns

                MAC ethernetSrc (eth_hdr->ether_shost);
                ethernetSrc.CopyTo(eth_hdr->ether_dhost);
                recvInterface.GetMAC().CopyTo(eth_hdr->ether_shost);

                arp_hdr->ea_hdr.ar_op = htons(ARPOP_REPLY);
                recvInterface.GetMAC().CopyTo(arp_hdr->arp_sha);
                recvInterface.GetIP().CopyTo(arp_hdr->arp_spa);
                senderMAC.CopyTo(arp_hdr->arp_tha);
                senderIP.CopyTo(arp_hdr->arp_tpa);

                send_packet(recvInterface.GetIndex(), &packet);
            }
            break;
        case ARPOP_REPLY:
            arpTable.Add(senderIP, senderMAC);
            ipPacketQueue.SendQueuedPackets(senderIP, senderMAC);
            break;
        }
    }

    void HandleIPPacket(packet_t& packet, const Interface& recvInterface)
    {
        struct iphdr *ip_hdr = reinterpret_cast<struct iphdr *>(packet.payload + sizeof(struct ether_header));

        if (ip_hdr->daddr == recvInterface.GetIP())
            HandleOwnIPPacket(packet, recvInterface);
        else
            RouteIPPacket(packet);
    }

    void HandleOwnIPPacket(packet_t& packet, const Interface& recvInterface)
    {
        // Pachet IP destinat mie
        struct iphdr *ip_hdr = reinterpret_cast<struct iphdr *>(packet.payload + sizeof(struct ether_header));
        struct icmphdr *icmp_hdr = reinterpret_cast<struct icmphdr *>(packet.payload + sizeof(struct ether_header) + sizeof(struct iphdr));

        // Raspund doar la ICMP ping
        if (ip_hdr->protocol != IPPROTO_ICMP || icmp_hdr->type != ICMP_ECHO)
            return;

        void *icmpData = icmp_hdr + 1;
        uint16_t icmpDataSize = ntohs(ip_hdr->tot_len) - sizeof(struct iphdr) - sizeof(struct icmphdr);

        packet_t icmpPacket;
        icmpPacket.len = packet.len;

        ConstructIPPacket(icmpPacket, IPPROTO_ICMP, recvInterface.GetIP(), ip_hdr->saddr, ntohs(ip_hdr->tot_len));
        ConstructICMPPacket(icmpPacket, ICMP_ECHOREPLY, 0, ntohs(icmp_hdr->un.echo.id), ntohs(icmp_hdr->un.echo.sequence), icmpData, icmpDataSize);
        SendIPPacket(icmpPacket);
    }

    void RouteIPPacket(packet_t& packet)
    {
        struct iphdr *ip_hdr = reinterpret_cast<struct iphdr *>(packet.payload + sizeof(struct ether_header));

        if (ip_hdr->ttl <= 1) {
            // Verific daca am ruta inapoi la sursa (normal ar trebui) si anunt ca TTL a expirat
            RoutingTable::Entry route;
            if (routingTable.GetBestRoute(ip_hdr->saddr, route) == false)
                return;

            packet_t icmpPacket;
            uint16_t icmpDataSize = sizeof (struct iphdr) + 8;
            uint16_t ipPacketSize = sizeof (struct iphdr) + sizeof (struct icmphdr) + icmpDataSize;

            icmpPacket.len = sizeof (ether_header) + ipPacketSize;

            ConstructIPPacket(icmpPacket, IPPROTO_ICMP, route.GetInterface().GetIP(), ip_hdr->saddr, ipPacketSize);
            ConstructICMPPacket(icmpPacket, ICMP_TIME_EXCEEDED, ICMP_EXC_TTL, 0, 0, ip_hdr, icmpDataSize);
            SendIPPacket(icmpPacket);
            return;
        }

        if (Checksum(ip_hdr, sizeof(struct iphdr)) != 0) {
            return;
        }

        // Scad TTL si fac incremental update la checksum
        uint16_t oldValue = *reinterpret_cast<uint16_t *>(&ip_hdr->ttl);
        ip_hdr->ttl--;
        ip_hdr->check = IncrementalChecksum(ip_hdr->check, oldValue, *reinterpret_cast<uint16_t *>(&ip_hdr->ttl));

        SendIPPacket(packet);
    }

    void ConstructIPPacket(packet_t& packet, uint8_t protocol, const IP& srcIP, const IP& destIP, uint16_t length)
    {
        struct ether_header *eth_hdr = reinterpret_cast<struct ether_header *>(packet.payload);
        struct iphdr *ip_hdr = reinterpret_cast<struct iphdr *>(packet.payload + sizeof(struct ether_header));

        eth_hdr->ether_type = htons(ETHERTYPE_IP);

        ip_hdr->version = 4;
        ip_hdr->ihl = 5;
        ip_hdr->tos = 0;
        ip_hdr->tot_len = htons(length);
        ip_hdr->id = getpid();
        ip_hdr->frag_off = 0;
        ip_hdr->ttl = kTTL;
        ip_hdr->protocol = protocol;
        ip_hdr->daddr = destIP;
        ip_hdr->saddr = srcIP;
        ip_hdr->check = 0;
        ip_hdr->check = Checksum(ip_hdr, sizeof(struct iphdr));
    }

    void ConstructICMPPacket(packet_t& packet, uint8_t type, uint8_t code = 0, uint16_t id = 0, uint16_t seq = 0, const void *data = nullptr, uint16_t size = 0)
    {
        struct icmphdr *icmp_hdr = reinterpret_cast<struct icmphdr *>(packet.payload + sizeof(struct ether_header) + sizeof(struct iphdr));

        // Pachetele ICMP care anunta erori trebuie sa contina un payload care consta in:
        // header IP original + primii 8 bytes ai payload-ului original
        if (data != nullptr) {
            memcpy(icmp_hdr + 1, data, size);
        }

        icmp_hdr->code = code;
        icmp_hdr->type = type;
        switch (type) {
            case ICMP_ECHO:
            case ICMP_ECHOREPLY:
                icmp_hdr->un.echo.id = htons(id);
                icmp_hdr->un.echo.sequence = htons(seq);
                break;
        }

        // Checksum-ul ICMP nu se face doar pe header, ci si pe payload (daca exista)
        icmp_hdr->checksum = 0;
        icmp_hdr->checksum = Checksum(icmp_hdr, sizeof(struct icmphdr) + size);
    }

    void SendIPPacket(packet_t& packet)
    {
        struct ether_header *eth_hdr = reinterpret_cast<struct ether_header *>(packet.payload);
        struct iphdr *ip_hdr = reinterpret_cast<struct iphdr *>(packet.payload + sizeof(struct ether_header));

        MAC destMAC;
        IP destIP (ip_hdr->daddr);
        RoutingTable::Entry bestRoute;

        if (routingTable.GetBestRoute(destIP, bestRoute) == false) {
            // Verific daca am ruta inapoi la sursa (normal ar trebui) si anunt ca nu am ruta catre destinatie
            if (routingTable.GetBestRoute(ip_hdr->saddr, bestRoute) == false)
                return;

            packet_t icmpPacket;
            uint16_t icmpDataSize = sizeof (struct iphdr) + 8;
            uint16_t ipPacketSize = sizeof (struct iphdr) + sizeof (struct icmphdr) + icmpDataSize;

            icmpPacket.len = sizeof (ether_header) + ipPacketSize;

            ConstructIPPacket(icmpPacket, IPPROTO_ICMP, bestRoute.GetInterface().GetIP(), ip_hdr->saddr, ipPacketSize);
            ConstructICMPPacket(icmpPacket, ICMP_DEST_UNREACH, ICMP_NET_UNREACH, 0, 0, ip_hdr, icmpDataSize);
            SendIPPacket(icmpPacket);
            return;
        }

        bestRoute.GetInterface().GetMAC().CopyTo(eth_hdr->ether_shost);
        if (!arpTable.GetMAC(bestRoute.GetNextHop(), destMAC)) {
            // Nu cunost MAC-ul nexthop-ului, asa ca pun pachetul in coada
            // Trimit un ARP Request pe interfata spre nexthop

            ipPacketQueue.Add(bestRoute.GetNextHop(), bestRoute.GetInterface(), packet);
            SendARPRequest(bestRoute);
            return;
        }

        // Cunosc tot ce imi trebuie, rutez pachetul mai departe
        destMAC.CopyTo(eth_hdr->ether_dhost);
        send_packet(bestRoute.GetInterface().GetIndex(), &packet);
    }

    void SendARPRequest(const RoutingTable::Entry& routingEntry)
    {
        packet_t packet;
        struct ether_header *eth_hdr = reinterpret_cast<struct ether_header *>(packet.payload);
        struct ether_arp *arp_hdr = reinterpret_cast<struct ether_arp *>(packet.payload + sizeof(struct ether_header));
        packet.len = sizeof(struct ether_header) + sizeof(struct ether_arp);

        eth_hdr->ether_type = htons(ETHERTYPE_ARP);
        MAC_BROADCAST.CopyTo(eth_hdr->ether_dhost);
        routingEntry.GetInterface().GetMAC().CopyTo(eth_hdr->ether_shost);

        arp_hdr->ea_hdr.ar_op = htons(ARPOP_REQUEST);
        arp_hdr->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
        arp_hdr->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
        arp_hdr->ea_hdr.ar_hln = 6;
        arp_hdr->ea_hdr.ar_pln = 4;

        MAC_NULL.CopyTo(arp_hdr->arp_tha);
        routingEntry.GetNextHop().CopyTo(arp_hdr->arp_tpa);

        routingEntry.GetInterface().GetMAC().CopyTo(arp_hdr->arp_sha);
        routingEntry.GetInterface().GetIP().CopyTo(arp_hdr->arp_spa);

        send_packet(routingEntry.GetInterface().GetIndex(), &packet);
    }

    static uint16_t Checksum(const void *data, uint16_t len)
    {
        const uint8_t *p = reinterpret_cast<const uint8_t *>(data);
        uint64_t sum = 0;

        while (len >= 4) {
            sum += *reinterpret_cast<const uint32_t *>(p);
            p += 4;
            len -= 4;
        }
        if (len >= 2) {
            sum += *reinterpret_cast<const uint16_t *>(p);
            p += 2;
            len -= 2;
        }
        if (len == 1) {
            sum += *p;
        }

        while (sum >> 16) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        return ~static_cast<uint16_t>(sum);
    }

    static uint16_t IncrementalChecksum(uint16_t oldCheck, uint16_t oldValue, uint16_t newValue)
    {
        uint32_t sum;

        oldCheck = ntohs(oldCheck);
        oldValue = ntohs(oldValue);
        newValue = ntohs(newValue);

        sum = oldCheck + oldValue + (~newValue & 0xFFFF);
        sum = (sum & 0xFFFF) + (sum >> 16);
        return htons(sum + (sum >> 16));
    }

    static constexpr uint8_t kTTL = 64;

    RoutingTable routingTable;
    ARPTable arpTable;
    IPPacketQueue ipPacketQueue;
};

int main()
{
    Router router;
    router.Run();
    return EXIT_SUCCESS;
}
