#include <cstdint>
#include <iostream>
#include <random>
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "reps-routing.h"
#include "src/core/model/simulator.h"
#include "src/point-to-point/model/qbb-header.h"
#include "src/point-to-point/model/reps-tag.h"

namespace ns3 {

RepsRouting::RepsRouting() {
    head = 0;
    offset = 0;
    isFreezingMode = false;
    numVaildEvs = 0;
    is_empty = true;
    lfRtt = 4300;
}

RepsRouting::~RepsRouting() {}

/**getter and setter*/

void RepsRouting::SetExploreCounter(uint32_t counter) {
    exploreCounter = counter;
}

uint32_t RepsRouting::GetExploreCounter() { return exploreCounter; }

uint16_t RepsRouting::GetNextEntropy() {
    if (numVaildEvs > 0) {
        offset = (head - numVaildEvs) % BUFFER_SIZE;
        buffer[offset].isVaild = false;
        numVaildEvs--;
    } else {
        offset = head;
        head = (head + 1) % BUFFER_SIZE;
    }
    return buffer[offset].cacheEv;
}

uint64_t RepsRouting::GetlfRtt() { return lfRtt; }
void RepsRouting::SetlfRtt(uint64_t rtt) { lfRtt = rtt; }

bool RepsRouting::GetIsFreezingMode() { return isFreezingMode; }
void RepsRouting::SetIsFreezingMode(bool mode) { isFreezingMode = mode; }

void RepsRouting::SetNumPktsBdp(uint32_t num) { numPktsBdp = num; }
uint32_t RepsRouting::GetNumPktsBdp() { return numPktsBdp; }

//generate a 16-bit random entropy value
uint16_t RepsRouting::GetRandEv() {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<uint16_t> dis(0, 65535);
    return dis(gen);
}

bool RepsRouting::IsEmpty() { return is_empty; }

Ptr<Packet> RepsRouting::OnSend(Ptr<Packet> packet) {
    RepsTag repsTag;
    if (is_empty || (numVaildEvs == 0 && !isFreezingMode) || exploreCounter) {
        uint16_t ev = GetRandEv();
        repsTag.SetEv(ev);
        exploreCounter = exploreCounter-1 > 0 ? exploreCounter - 1 : 0;
    } else {
        uint16_t ev = GetNextEntropy();
        repsTag.SetEv(ev);
    }
    repsTag.SetTime(Simulator::Now().GetNanoSeconds());
    packet->AddPacketTag(repsTag);
    return packet;
}

//return packet rtt
uint64_t RepsRouting::OnAck(Ptr<Packet> packet) {
    // std::cout << "OnAck" << std::endl;
    uint64_t rtt = 0;
    qbbHeader qbbHeader;
    packet->PeekHeader(qbbHeader);
    uint8_t cnpBits = qbbHeader.GetCnp();
    RepsTag repsTag;
    bool found = packet->PeekPacketTag(repsTag);
    if (found) {
        rtt = Simulator::Now().GetNanoSeconds() - repsTag.GetTime();
        if (cnpBits != 0) return rtt;
        if (!buffer[head].isVaild) {
            numVaildEvs++;
        }
        
        buffer[head].cacheEv = repsTag.GetEv();
        buffer[head].isVaild = true;
        head = (head + 1) % BUFFER_SIZE;
        if(is_empty) is_empty = false;
        
        if (isFreezingMode && Now() > exitFreezingMode) {
            isFreezingMode = false;
            exploreCounter = numPktsBdp;
        }
    } else {
        std::cout << "RepsRouting::OnAck: No RepsTag found" << std::endl;
        exit(1);
    }
    //std::cout << "RTT: " << Simulator::Now().GetNanoSeconds() - repsTag.GetTime() << std::endl;
    return rtt;
}

void RepsRouting::OnFailureDetection() {
    if (!isFreezingMode && exploreCounter == 0) {
        isFreezingMode = true;
        exitFreezingMode = Now() + MicroSeconds(FREEZING_TIMEOUT);
    }
}
}