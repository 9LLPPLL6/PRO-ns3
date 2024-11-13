#include "ns3/pro-routing.h"

#include "assert.h"
#include "ns3/assert.h"
#include "ns3/event-id.h"
#include "ns3/ipv4-header.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/settings.h"
#include "ns3/simulator.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ProRouting");

std::map<uint32_t, std::vector<uint32_t>> ProRouting::SwitchId2hostId;
std::map<uint32_t, std::map<uint32_t, std::set<uint32_t>>> ProRouting::paths;
uint32_t ProRouting::pro_c = 0;
uint32_t ProRouting::path_num = 0;
std::map<uint64_t, uint32_t> ProRouting::packet2path;
// bool ProRouting::ispro = false;
// uint64_t ProRouting::sample_t = 0; // ns
uint64_t ProRouting::maxdelay = 3;  // us
std::map<uint32_t, std::map<uint32_t, uint64_t>> ProRouting::path2rtt;

ProRouting::ProRouting() {}
ProRouting::~ProRouting(){}