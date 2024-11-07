#pragma once

#include <arpa/inet.h>

#include <map>
#include <queue>
#include <unordered_map>
#include <vector>

#include "ns3/address.h"
#include "ns3/callback.h"
#include "ns3/event-id.h"
#include "ns3/net-device.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/settings.h"
#include "ns3/simulator.h"
#include "ns3/tag.h"

using namespace ns3;

#define M 8

class ProRouting : public Object {
    friend class SwitchMmu;
    friend class SwitchNode;

   public:
    ProRouting();
    ~ProRouting();

    // static bool ispro;
    static std::map<uint32_t, std::vector<uint32_t>> SwitchId2hostId;  // connected Switch's Id -> host's id
    static std::map<uint32_t, std::map<uint32_t, std::set<uint32_t>>> paths;    // <srcid,dstid> -> paths
    static uint32_t path_num;
    static uint32_t pro_c;  // to Inter-QP round robin
    static std::map<uint64_t, uint32_t> packet2path;  // packet -> path
    static double sample_t;                         // The sampling interval of sending packets (s)
    static uint32_t maxdelay;
};