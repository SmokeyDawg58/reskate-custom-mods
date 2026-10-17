#include "Extension/Throwdowns/challenge_registry.h"
#include "Extension/Modes/mode_rules.h"
#include <iostream>
#include <set>
#include <stdexcept>
using namespace dingosdk;
void check(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
int main() {
    try {
        const auto& builtins=throwdowns::registry();
        check(builtins.definitions().size()==12,"Two stock, R.E.S.K.A.T.E., canonical 1-Up and eight other custom modes are registered.");
        std::set<std::string> ids,logos;std::set<unsigned> colors,modes;
        throwdowns::Registry repeated;
        for(const auto& d:builtins.definitions()) {
            check(ids.insert(d.id).second,"Unique stable IDs.");
            check(repeated.add(d).accepted,"Built-ins validate.");
            const auto revision=repeated.revision();const auto duplicate=repeated.add(d);
            check(duplicate.accepted && !duplicate.inserted && repeated.revision()==revision,"Repeated registration is idempotent.");
            auto changed=d;changed.name+=" conflict";
            check(!repeated.add(changed).accepted,"Conflicting repeated metadata is rejected.");
            if(d.adapter==throwdowns::Adapter::game_mode) {
                check(d.mode!=2,"Canonical 1-Up has no duplicate custom adapter.");
                check(modes.insert(d.mode).second,"Only one adapter owns each imported mode.");
                check(colors.insert(d.palette.primary).second,"All custom challenges have different colors.");
                check(!d.logo.empty() && logos.insert(d.logo).second,"All custom logos have distinct identities.");
                const auto resolved=dingosdk::modes::parse_mode(d.key);
                check(resolved && static_cast<unsigned>(*resolved)==d.mode,"Every generated card resolves to real rules.");
                dingosdk::modes::Settings settings;settings.mode=*resolved;
                check(d.needs_points==!dingosdk::modes::missing(settings).empty(),"Cards expose their mode's required placement.");
            }
        }
        check(modes.size()==9,"All supplied compatible game modes have adapters.");
        check(builtins.find("native.oneup")->palette.primary==0xffef8b22U,
            "1-Up uses its approved blue instead of a fallback challenge hue.");
        check(builtins.find("native.oneup")->adapter==throwdowns::Adapter::native_one_up &&
              !builtins.mode(static_cast<unsigned>(dingosdk::modes::Mode::one_up)),
            "Native 1-UP results resolve its canonical record rather than a nonexistent custom adapter.");
        const auto* reskate=builtins.find("native.skate");
        check(reskate && reskate->name=="R.E.S.K.A.T.E." && reskate->mode==8 && reskate->adapter==throwdowns::Adapter::game_mode,
            "The replacement card uses the seven-letter referee rather than the stock five-letter challenge.");
        auto invalid=*builtins.find("mode.jam");invalid.id="bad path/escape";
        check(!repeated.add(invalid).accepted,"Invalid stable IDs are refused.");
        invalid.id="other.mode";invalid.key="not-a-mode";
        check(!repeated.add(invalid).accepted,"An unavailable gameplay adapter cannot launch.");
        invalid=*builtins.find("mode.jam");invalid.id="second.jam";
        check(!repeated.add(invalid).accepted,"Duplicate mode ownership is rejected.");
        throwdowns::Registry empty;check(empty.definitions().empty() && !empty.find("missing"),"Empty/missing registry is safe.");
        check(throwdowns::palette_for("custom.future")==throwdowns::palette_for("custom.future"),"Fallback colors remain deterministic.");
        std::cout<<"Challenge registry integration contracts passed\n";return 0;
    }catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
