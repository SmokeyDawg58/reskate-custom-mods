#include "Extension/World/guid_table_probe.h"
#include <cstdio>
#include <stdexcept>
#include <vector>

using namespace dingosdk::guid_table_guard::detail;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main() {
    try {
        check(slot_count(0)==16 && slot_count(10)==16 && slot_count(11)==16 && slot_count(12)==32,"native capacity rounding");
        check(slot_count(65535)==131072 && slot_count(65536)==0,"record index range is bounded");
        const std::array<std::uint32_t,4> wanted{0x53c67866,0x92746592,0x83eb5c10,0xc9f8725e};
        std::vector<std::uint16_t> slots(16,0xffff);
        unsigned calls=0;
        auto key_of=[&](auto index)->const std::uint32_t* {++calls;return index==3?wanted.data():nullptr;};
        check(probe_slots(slots,wanted,key_of)==Probe::missing && calls==0,"empty slots do not invoke record lookup");
        // A deleted/empty slot before a present record must not hide that record.
        const auto start=wanted[0]&15,step=(wanted[0]&0x3e)|1u;
        slots[(start+step*4)&15]=3;
        check(probe_slots(slots,wanted,key_of)==Probe::found && calls==1,"colliding present key retains original native lookup");
        slots.assign(16,1);calls=0;
        const std::array<std::uint32_t,4> different{wanted[0],0,0,0};
        check(probe_slots(slots,wanted,[&](auto){++calls;return different.data();})==Probe::missing && calls==16,"full missing-key table completes after exactly one traversal");
        for(unsigned first=0;first<64;++first) {
            auto key=wanted;key[0]=first;unsigned seen=0;
            for(unsigned i=0;i<slots.size();++i)slots[i]=static_cast<std::uint16_t>(i);
            check(probe_slots(slots,key,[&](auto index)->const std::uint32_t* {check(!(seen&(1u<<index)),"probe revisited a slot");seen|=1u<<index;return nullptr;})==Probe::missing && seen==0xffffu,"all odd probe strides visit every slot once");
        }
        check(probe_slots(std::span<const std::uint16_t>{},wanted,key_of)==Probe::unreadable,"absent table rejected");
        std::puts("GUID table bounded-probe regressions passed");return 0;
    }catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}
}
