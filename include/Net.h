#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/ethernet.h>

#include <string>

#include "skel.h"


class IP
{
public:
    IP () : IP(kIPNull) { }
    IP (uint32_t ip)
    {
        addr.s_addr = ip;
        addrString.assign(inet_ntoa(addr));
    }
    IP (in_addr addr) : IP(addr.s_addr) { }
    IP (const uint8_t *ip) : IP(*reinterpret_cast<const uint32_t *>(ip)) { }
    IP (const char *ip) : IP (inet_addr(ip)) { }
    IP (const std::string& ip) : IP (ip.c_str()) { }

    void CopyTo(void *dest) const { memcpy(dest, &addr, sizeof(addr));}

    operator in_addr() const { return addr; }
    operator uint32_t() const { return addr.s_addr; }
    explicit operator const std::string&() const {return addrString; }
    explicit operator const char *() const { return addrString.c_str(); }

    bool operator==(const IP& rhs) const { return this->addr.s_addr == rhs.addr.s_addr; }
    bool operator!=(const IP& rhs) const { return this->addr.s_addr != rhs.addr.s_addr; }

    static constexpr uint32_t kIPNull = 0x00000000;

private:
    in_addr addr;
    std::string addrString;
};

class MAC {
public:
    MAC () : MAC(kMACNull) { }
    MAC (const uint8_t *mac)
    {
        char macString[kMACStringLength];

        memcpy(addr, mac, ETH_ALEN);
        sprintf(macString, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        addrString.assign(macString);
    }

    void CopyTo(void *dest) const { memcpy(dest, &addr, sizeof(addr)); }

    explicit operator const uint8_t *() const { return addr; }
    explicit operator const std::string&() const { return addrString; }
    explicit operator const char *() const { return addrString.c_str(); }

    bool operator==(const MAC& rhs) const { return memcmp(this->addr, rhs.addr, ETH_ALEN) == 0; }
    bool operator!=(const MAC& rhs) const { return memcmp(this->addr, rhs.addr, ETH_ALEN) != 0; }

    static constexpr uint8_t kMACNull[]      = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    static constexpr uint8_t kMACBroadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

private:
    uint8_t addr[ETH_ALEN];
    std::string addrString;

    static constexpr int kMACStringLength = 18;
};

class Interface
{
public:
    Interface() { }
    Interface(int index) : index(index)
    {
        uint8_t macAddr[ETH_ALEN];
        char ipAddr[INET_ADDRSTRLEN];

        get_interface_mac(index, macAddr);
        strcpy(ipAddr, get_interface_ip(index));

        ip = IP(ipAddr);
        mac = MAC(macAddr);
    }

    const IP& GetIP() const { return ip; }
    const MAC& GetMAC() const { return mac; }
    int GetIndex() const { return index; }

private:
    IP ip;
    MAC mac;
    int index;
};

static const IP IP_NULL (IP::kIPNull);
static const MAC MAC_NULL (MAC::kMACNull);
static const MAC MAC_BROADCAST (MAC::kMACBroadcast);
