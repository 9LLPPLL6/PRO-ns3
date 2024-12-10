#pragma once

#include <cstdint>
#include "ns3/tag.h"
#include "ns3/type-id.h"

namespace ns3 {

class RepsTag : public Tag {
   public:
    RepsTag();
    ~RepsTag();

    static TypeId GetTypeId(void);
    TypeId GetInstanceTypeId(void) const;
    virtual uint32_t GetSerializedSize(void)const;
    virtual void Serialize(TagBuffer i) const;
    virtual void Deserialize (TagBuffer i);
    virtual void Print (std::ostream &os) const;
    // getter and setter
    void SetEv(uint16_t ev);
    uint16_t GetEv() const;
    void SetTime(uint64_t time);
    uint64_t GetTime() const;


    
   private:
    uint16_t ev;
    uint64_t time;  //ns
};

}