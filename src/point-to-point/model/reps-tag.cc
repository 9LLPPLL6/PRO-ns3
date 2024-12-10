#include "reps-tag.h"
#include <cstdint>
#include "ns3/tag.h"
#include "ns3/type-id.h"

namespace ns3 {

RepsTag::RepsTag() {}
RepsTag::~RepsTag() {}

TypeId RepsTag::GetTypeId() {
    static TypeId tid = TypeId("ns3::RepsTag").SetParent<Tag>().AddConstructor<RepsTag>();
    return tid;
}

TypeId RepsTag::GetInstanceTypeId() const { return GetTypeId(); }

uint32_t RepsTag::GetSerializedSize() const { return sizeof(ev) + sizeof(time); }

void RepsTag::Serialize(TagBuffer i) const {
    i.WriteU16(ev);
    i.WriteU64(time);
}

void RepsTag::Deserialize(TagBuffer i) {
    ev = i.ReadU16();
    time = i.ReadU64();
}

void RepsTag::Print(std::ostream &os) const {
    os << "ev=" << ev << std::endl;
    os << "time=" << time << std::endl;
}

// getters and setters
uint16_t RepsTag::GetEv() const { return ev; }
void RepsTag::SetEv(uint16_t ev) { this->ev = ev; }
uint64_t RepsTag::GetTime() const { return time; }
void RepsTag::SetTime(uint64_t time) { this->time = time; }

}