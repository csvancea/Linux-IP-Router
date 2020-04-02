#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include "Net.h"

class RoutingTable
{
public:
    class Entry
    {
    public:
        Entry () { }
        Entry (const IP& nextHop, const Interface& interface) :
            nextHop(nextHop), interface(interface) { }

        const IP& GetNextHop() const { return nextHop; }
        const Interface& GetInterface() const { return interface; }

    private:
        IP nextHop;
        Interface interface;

        friend class RoutingTable; 
    };

    bool Add(const IP& prefix, const IP& mask, const IP& nextHop, const Interface& interface);
    bool GetBestRoute(const IP& destIP, Entry& bestRoute) const;
    bool ParseFile(const std::string& fileName);

private:
    struct Node
    {
        static constexpr int kMaxChildren = 2;
        
        Node()
        {
            entry = nullptr;
            for (int i = 0; i != kMaxChildren; ++i)
                children[i] = nullptr;
        }
        ~Node()
        {
            if (entry)
                delete entry;
            for (int i = 0; i != kMaxChildren; ++i)
                if (children[i])
                    delete children[i];
        }
        Node(const Node&) = delete;
        void operator=(const Node&) = delete;

        Node *children[kMaxChildren];
        Entry *entry;
    };

    Node root;
};