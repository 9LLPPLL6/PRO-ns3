#pragma once
#include <cstdint>
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"

#define BUFFER_SIZE 8
// todo: setup the freezing timeout
//ms
#define FREEZING_TIMEOUT 100

namespace ns3 {

struct RepsElements {
    uint16_t cacheEv;
    bool isVaild = false;
};

class RepsRouting : public Object {
   private:
    RepsElements buffer[BUFFER_SIZE];
    uint16_t head;
    uint16_t offset;
    bool isFreezingMode;
    uint32_t exploreCounter;
    uint32_t numVaildEvs;
    bool is_empty;
    Time exitFreezingMode;
    uint32_t numPktsBdp;
    uint64_t lfRtt; // ns

   public:
    RepsRouting();
    ~RepsRouting();

    // getter and setter
    void SetExploreCounter(uint32_t counter);
    uint32_t GetExploreCounter();
    void SetlfRtt(uint64_t rtt);
    uint64_t GetlfRtt();
    void SetIsFreezingMode(bool isFreezing);
    bool GetIsFreezingMode();
    void SetNumPktsBdp(uint32_t numPkts);
    uint32_t GetNumPktsBdp();

    uint16_t GetNextEntropy();
    uint16_t GetRandEv();
    bool IsEmpty();
    Ptr<Packet> OnSend(Ptr<Packet> packet);
    uint64_t OnAck(Ptr<Packet> packet);
    void OnFailureDetection();
};
}