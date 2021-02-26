#include <fstream>
#include "RoutingTable.h"

// Implementare cu trie -> lookup in O(1)

bool RoutingTable::Add(const IP& prefix, const IP& mask, const IP& nextHop, const Interface& interface)
{
    Node *node = &root;
    uint32_t prefixHost = ntohl(prefix);
    uint32_t maskHost = ntohl(mask);

    for (uint32_t i = 0x80000000; i != 0; i >>= 1) {
        uint32_t p = !!(prefixHost & i);
        uint32_t m = maskHost & i;

        if (m == 0) {
            if (node->entry) {
                return false;
            }

            node->entry = new Entry(nextHop, interface);
            return true;
        }

        if (node->children[p] == nullptr) {
            node->children[p] = new Node();
        }
        node = node->children[p];
    }

    if (node->entry) {
        return false;
    }
    node->entry = new Entry(nextHop, interface);
    return true;
}

bool RoutingTable::GetBestRoute(const IP& destIP, Entry& bestRoute) const
{
    const Node *node = &root;
    bool found = false;
    uint32_t destIPHost = ntohl(destIP);

    for (uint32_t i = 0x80000000; i != 0; i >>= 1) {
        uint32_t p = !!(destIPHost & i);

        if (node->entry != nullptr) {
            bestRoute = *node->entry;
            found = true;
        }

        node = node->children[p];
        if (node == nullptr) {
            break;
        }
    }

    if (node != nullptr && node->entry != nullptr) {
        bestRoute = *node->entry;
        found = true;
    }
    return found;
}

bool RoutingTable::ParseFile(const std::string& file_name)
{
    std::ifstream file {file_name};
    if (!file.is_open()) {
        return 0;
    }

    std::string prefix, mask, nextHop;
    uint32_t interface;

    while (file >> prefix >> nextHop >> mask >> interface) {
        Add(prefix, mask, nextHop, interface);
    }
    return 1;
}
