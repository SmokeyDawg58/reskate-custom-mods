#pragma once
#include "mode_rules.h"
#include <algorithm>
#include <map>
#include <stdexcept>

namespace dingosdk::modes {
// Keep the session's 4096-byte packet contract. The editor has no gate cap;
// only a defensive transfer budget bounds untrusted remote allocations.
inline constexpr std::size_t mode_packet_bytes=4096, mode_fragment_header=27,
    mode_fragment_bytes=mode_packet_bytes-mode_fragment_header, mode_transfer_bytes=64*1024*1024;
inline std::uint32_t mode_transfer_hash(std::span<const std::uint8_t> bytes) noexcept {
    std::uint32_t value=2166136261u;for(const auto byte:bytes)value=(value^byte)*16777619u;return value;
}
inline std::vector<std::vector<std::uint8_t>> transport_messages(const Message& m) {
    auto bytes=encode(m);
    if(bytes.size()<=mode_packet_bytes)return {std::move(bytes)};
    if(m.kind!=Message::Kind::setup || bytes.size()>mode_transfer_bytes)throw std::invalid_argument("route transfer exceeds resource budget");
    const auto hash=mode_transfer_hash(bytes);
    std::vector<std::vector<std::uint8_t>> out;
    for(std::size_t at=0;at<bytes.size();at+=mode_fragment_bytes) {
        std::vector<std::uint8_t> packet(bytes.begin(),bytes.begin()+15);packet[2]=9;
        const auto put=[&](std::uint32_t n){for(unsigned i=0;i<4;++i)packet.push_back(static_cast<std::uint8_t>(n>>(8*i)));};
        put(hash);put(static_cast<std::uint32_t>(bytes.size()));put(static_cast<std::uint32_t>(at));
        packet.insert(packet.end(),bytes.begin()+at,bytes.begin()+std::min(at+mode_fragment_bytes,bytes.size()));
        out.push_back(std::move(packet));
    }
    return out;
}
inline bool mode_fragment(std::span<const std::uint8_t> bytes) noexcept {
    return bytes.size()>=3 && bytes[0]==wire_magic && bytes[1]==wire_version && bytes[2]==9;
}
class ModeTransfers {
    struct Pending {
        std::uint32_t hash{},total{};std::uint64_t leader{},updated{};std::uint32_t game{};
        std::map<std::uint32_t,std::vector<std::uint8_t>> chunks;std::size_t received{};
    };
    std::map<std::uint64_t,Pending> pending_;
public:
    void clear() {pending_.clear();}
    std::optional<Message> receive(std::uint64_t sender,std::span<const std::uint8_t> bytes,std::uint64_t now) {
        std::erase_if(pending_,[&](const auto& p){return now>=p.second.updated && now-p.second.updated>10000;});
        if(!mode_fragment(bytes) || bytes.size()<mode_fragment_header || bytes.size()>mode_packet_bytes)return {};
        const auto get=[&](std::size_t at,unsigned n){std::uint64_t value{};for(unsigned i=0;i<n;++i)value|=std::uint64_t(bytes[at+i])<<(8*i);return value;};
        const auto leader=get(3,8);const auto game=static_cast<std::uint32_t>(get(11,4));
        const auto hash=static_cast<std::uint32_t>(get(15,4)),total=static_cast<std::uint32_t>(get(19,4)),at=static_cast<std::uint32_t>(get(23,4));
        const auto size=bytes.size()-mode_fragment_header;
        if(!sender || leader!=sender || !game || total<=mode_packet_bytes || total>mode_transfer_bytes || at>=total ||
           at%mode_fragment_bytes || size!=std::min<std::size_t>(mode_fragment_bytes,total-at))return {};
        if(!pending_.contains(sender) && pending_.size()>=max_players)return {};
        auto& p=pending_[sender];
        if(p.hash!=hash || p.total!=total || p.leader!=leader || p.game!=game)p=Pending{hash,total,leader,now,game};
        p.updated=now;
        const auto part=bytes.subspan(mode_fragment_header);
        auto [it,inserted]=p.chunks.try_emplace(at,part.begin(),part.end());
        if(inserted)p.received+=size;
        else if(!std::equal(part.begin(),part.end(),it->second.begin())) {pending_.erase(sender);return {};}
        if(p.received!=total)return {};
        std::vector<std::uint8_t> joined;joined.reserve(total);
        for(const auto& [offset,data]:p.chunks)joined.insert(joined.end(),data.begin(),data.end());
        pending_.erase(sender);
        if(mode_transfer_hash(joined)!=hash)return {};
        auto m=decode(joined);
        if(!m || m->kind!=Message::Kind::setup || m->leader!=leader || m->game!=game)return {};
        return m;
    }
};
}
