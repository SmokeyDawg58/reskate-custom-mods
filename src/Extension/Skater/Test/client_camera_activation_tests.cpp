// Run the production activation/recovery path against native-call fixtures.
// A flag-picker reset retains the objects but restores mode zero; the old
// lease-only check incorrectly treated that state as a working free camera.
#include "Extension/Skater/client_camera_activation.h"
#include "Extension/Modes/placement_camera.h"
#include <iostream>
#include <stdexcept>

using namespace dingosdk;
using namespace dingosdk::client_source::detail;
namespace {
constexpr std::array<float, 16> initial_pose{1,0,0,0,0,1,0,0,0,0,1,0,20,30,40,1};
SourceCameraSnapshot fixture_camera;
std::array<float, 16> written_pose;
unsigned mode_calls{}, transform_calls{};
bool fail_mode{}, verify_mode=true;
void check(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
void mode(std::uintptr_t controller,std::uint32_t value) {
    check(controller==fixture_camera.identity.controller && value==1,"Activation used the wrong native controller or mode");
    ++mode_calls;
    if(fail_mode)throw SourceGuard{"native mode exception"};
    if(verify_mode) {fixture_camera.mode=1;fixture_camera.active=fixture_camera.identity.camera;fixture_camera.input_active=1;}
}
void transform(std::uintptr_t camera,const std::array<float,16>* pose) {
    check(camera==fixture_camera.identity.camera,"Activation wrote a different camera");
    ++transform_calls;written_pose=*pose;
}
void reset() {
    fixture_camera={};fixture_camera.identity={0x10000,0x20000,0x30000,0x40000,0x50000,0x60000,0x70000,0x80000,{0x90000,0xa0000},1};
    fixture_camera.active=0xb0000;fixture_camera.view_enabled=1;
    mode_calls=transform_calls=0;fail_mode=false;verify_mode=true;written_pose={};
}
SourceTrial trial() {SourceTrial t;t.camera_mode=mode;t.camera_transform=transform;return t;}
template<class F> void rejected(F&& run,const char* why) {
    bool caught{};try {run();}catch(const SourceGuard&){caught=true;}check(caught,why);
}
}
namespace dingosdk::client_source::detail {
SourceCameraSnapshot source_camera_snapshot(const SourceTrial&,std::uintptr_t) {return fixture_camera;}
std::array<float,16> debug_view_matrix(const SourceTrial&,const SourceCameraSnapshot&) {return initial_pose;}
}
int main() {
    try {
        reset();auto t=trial();auto camera=fixture_camera;
        modes::PlacementCamera placement;
        check(placement.update(true,verified_free_camera_view(t.debug,camera),100).value_or(false),"Placement did not request its initial camera");
        check(activate_free_camera(t,1,camera,0) && verified_free_camera_view(t.debug,camera),"Native freecam did not activate");
        check(mode_calls==1 && transform_calls==1 && written_pose==initial_pose,"Initial camera did not use the current view pose");
        check(!placement.update(true,verified_free_camera_view(t.debug,camera),200),"Verified active camera was redundantly requested");
        t.debug.flight_matrix[12]=123.f;t.debug.flight_matrix[13]=321.f;
        fixture_camera.mode=0;fixture_camera.active=0xb0000;fixture_camera.input_active=0;camera=fixture_camera;
        check(t.debug.camera_owned && !verified_free_camera_view(t.debug,camera),"Mode-zero reset was still considered selected freecam");
        check(placement.update(true,verified_free_camera_view(t.debug,camera),600).value_or(false),"Late native reset did not request reacquisition");
        check(activate_free_camera(t,1,camera,0) && verified_free_camera_view(t.debug,camera),"Same camera could not recover from the native flag reset");
        check(mode_calls==2 && transform_calls==2 && written_pose[12]==123.f && written_pose[13]==321.f,"Recovery returned to the skater instead of preserving the flight pose");
        check(!activate_free_camera(t,1,camera,0) && mode_calls==2 && transform_calls==2,"Active camera switched or rewrote its pose on repeated enable");
        check(placement.update(false,true,700)==false,"Placement did not release the camera after exit");

        camera.identity.context+=8;
        rejected([&]{activate_free_camera(t,1,camera,0);},"Replaced-world camera was taken over");
        check(mode_calls==2 && transform_calls==2,"Replacement rejection called native camera functions");
        camera=fixture_camera;camera.mode=2;
        rejected([&]{activate_free_camera(t,1,camera,0);},"Another native camera mode was taken over");
        camera=fixture_camera;camera.active=0xb0000;
        rejected([&]{activate_free_camera(t,1,camera,0);},"Mode-one competing view was taken over");
        check(!verified_free_camera_view(t.debug,camera),"Competing view still authorized placement rays");
        camera=fixture_camera;t.debug.first_person=true;
        check(!verified_free_camera_view(t.debug,camera),"First person authorized freecam placement rays");

        t.debug.first_person=false;t.debug.drone_follow=true;
        check(!verified_free_camera_view(t.debug,fixture_camera),"Drone follow authorized editor placement rays");
        t.debug.drone_follow=false;
        reset();t=trial();camera=fixture_camera;fail_mode=true;
        rejected([&]{activate_free_camera(t,1,camera,0);},"Native mode exception escaped the ownership guard");
        check(t.debug.camera_ambiguous && !verified_free_camera_view(t.debug,camera),"Uncertain native call left the placement camera verified");
        fail_mode=false;
        rejected([&]{activate_free_camera(t,1,camera,0);},"Ambiguous native mode call was retried");
        check(mode_calls==1 && transform_calls==0,"Ambiguous recovery issued a second native mode call");

        reset();t=trial();camera=fixture_camera;verify_mode=false;
        rejected([&]{activate_free_camera(t,1,camera,0);},"Unverified activation was accepted");
        check(!verified_free_camera_view(t.debug,camera) && transform_calls==0,"Unverified activation wrote a camera pose or authorized placement");
        std::cout<<"Native camera activation and flag-reset recovery passed\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
      catch(const SourceGuard& e) {std::cerr<<e.message<<'\n';return 1;}
}
