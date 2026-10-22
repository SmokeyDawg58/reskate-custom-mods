#include "Extension/Modes/race_route_library.h"
#include "Extension/Modes/race_route_json.h"
#include <chrono>
#include <iostream>
using namespace dingosdk::modes;
namespace fs=std::filesystem;
void check(bool condition,const char* what){if(!condition)throw std::runtime_error(what);}
std::string text(const fs::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){try {
    if(argc!=2)throw std::runtime_error("Name the route test workspace");
    const auto root=fs::absolute(argv[1])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());fs::create_directories(root);
    Settings settings;settings.mode=Mode::race;bool finish{};
    edit_race_route(settings,finish,RaceObject::start,{1,12,4},75,6);
    edit_race_route(settings,finish,RaceObject::checkpoint,{8,20,80},-25,9,0,20);
    edit_race_route(settings,finish,RaceObject::arrow,{12,24,100},35,5,60,-45);
    edit_race_route(settings,finish,RaceObject::finish,{20,12,200},0,8);settings.duration_s=600;
    RaceRouteLibrary library(root);library.read();
    const auto id=library.save({0,"Rooftop Run","Skate3Improved",settings});
    const auto first=library.path(id);check(first.filename()=="Rooftop Run.json" && first.parent_path().filename()=="Skate3Improved","Route and map names form readable local filenames");
    const auto saved=text(first);check(saved.find("Rooftop Run")!=std::string::npos && saved.find("pitch_degrees")!=std::string::npos,"JSON includes the name and readable object geometry");
    auto readable=dingosdk::Json::parse(saved);check(readable.contains("starting_flag"),"Shared JSON exposes its saved flag position and yaw");
    check(decode_named_race_json(saved)->settings.points.front()==settings.points.front() && race_start_heading(decode_named_race_json(saved)->settings)==75,
        "Saved flag position, altitude and direction survive the JSON round trip");
    readable.erase("starting_flag");check(decode_named_race_json(readable.dump())->settings==settings,"Existing JSON routes retain their original flag without the new descriptive field");
    readable=dingosdk::Json::parse(saved);readable["starting_flag"]["yaw_degrees"]=45;
    check(!decode_named_race_json(readable.dump()),"A contradictory flag pose cannot silently move a shared route");
    const auto other=library.save({0,"Other route","Skate3Improved",settings});const auto untouched=text(library.path(other));
    library.rename(id,"New Name");check(!fs::exists(first) && library.path(id).filename()=="New Name.json","Pencil rename moves the actual JSON filename");
    auto edited=settings;edited.points[1][1]=40;library.save({id,"New Name","Skate3Improved",edited});
    RaceRouteLibrary reopened(root);reopened.read();
    check(reopened.find(id) && reopened.find(id)->settings==edited && reopened.routes().size()==2,"Reopened JSON retains stable ID and exact settings");
    check(text(reopened.path(other))==untouched,"Rename and update never change another route");
    bool failed{};try{reopened.rename(id,"Other route");}catch(...){failed=true;}check(failed,"Duplicate filename cannot overwrite another route");
    failed=false;try{reopened.rename(id,"  ");}catch(...){failed=true;}check(failed && reopened.find(id)->settings==edited,"Blank rename leaves the file intact");
    reopened.rename(id,"../Label?Name");check(reopened.path(id).parent_path()==root/"Skate3Improved" && reopened.path(id).filename()==".._Label_Name.json","Untrusted display names stay inside their map folder");
    const auto shared=root/"shared.json";{std::ofstream f(shared);f<<saved;}
    failed=false;try{reopened.import_json(shared,"WrongMap");}catch(...){failed=true;}check(failed,"Import from another map is rejected before writing");
    const auto imported=reopened.import_json(shared,"Skate3Improved");
    check(imported!=id && imported!=other && reopened.find(imported)->settings==settings,"Shared local ID collisions cannot overwrite an existing route");
    check(reopened.import_json(shared,"Skate3Improved")==imported && reopened.import_json(reopened.path(imported),"Skate3Improved")==imported,"Loading the same JSON or the local route file reuses it without duplicates");
    const auto legacy=encode_named_race({99,"Legacy Course","Skate2Improved",settings});
    {std::ofstream f(root/"99.race",std::ios::binary);f.write(reinterpret_cast<const char*>(legacy.data()),static_cast<std::streamsize>(legacy.size()));}
    fs::remove(shared); // The external picker source is not part of the route library.
    {std::ofstream f(root/"bad.json");f<<"{bad";}
    RaceRouteLibrary migrated(root);migrated.read();
    check(migrated.find(99) && migrated.find(99)->settings==settings && migrated.path(99).filename()=="Legacy Course.json","Existing binary routes migrate to readable named JSON");
    check(text(root/"99.race")==std::string(reinterpret_cast<const char*>(legacy.data()),legacy.size()),"Migration preserves the original route as a backup");
    const auto count=migrated.routes().size();migrated.read();check(migrated.routes().size()==count,"Repeated migration does not duplicate a saved route");
    const auto mods=root/"Mods/ReSkateRoutes/Deathrace";
    const auto source_json=migrated.path(99);const auto source_text=text(source_json);
    RaceRouteLibrary relocated(mods,root);relocated.read();
    check(relocated.routes().size()==count,"Moving the library under Mods imports JSON and deduplicates its binary backups");
    const auto course=std::find_if(relocated.routes().begin(),relocated.routes().end(),[](const auto& r){return r.name=="Legacy Course";});
    check(course!=relocated.routes().end() && course->settings==settings && relocated.path(course->id).parent_path()==mods/"Skate2Improved",
        "The folder action resolves a readable JSON inside Mods, with exact saved geometry");
    const auto moved_id=course->id;relocated.rename(moved_id,"Renamed in Mods");
    RaceRouteLibrary again(mods,root);again.read();
    check(again.routes().size()==count && again.find(moved_id)->name=="Renamed in Mods" && text(source_json)==source_text && fs::exists(root/"99.race"),
        "A later rename cannot resurrect the old name and both original source files remain unchanged");
    const auto delete_path=again.path(moved_id);const auto survivors=again.routes().size();
    again.erase(moved_id);
    check(!again.find(moved_id) && !fs::exists(delete_path),"Confirmed deletion removes exactly the selected JSON and library row");
    RaceRouteLibrary after_delete(mods,root);after_delete.read();
    check(!after_delete.find(moved_id) && after_delete.routes().size()==survivors-1 && text(source_json)==source_text,
        "Deleted routes remain absent on reopen without modifying original migration backups");
    const auto new_id=after_delete.save({0,"After deletion","Skate2Improved",settings});
    check(new_id!=moved_id,"A deleted route ID is not reused by a new saved route");
    failed=false;try{after_delete.erase(moved_id);}catch(...){failed=true;}
    check(failed && after_delete.find(new_id),"Stale confirmation cannot delete a different route");
    migrated.erase(99);RaceRouteLibrary deleted_legacy(root);deleted_legacy.read();
    check(!deleted_legacy.find(99) && fs::exists(root/"99.race"),"Deleting a migrated JSON cannot resurrect its old binary backup");
    std::cout<<"PASS JSON route save/update/import/rename, readable filenames, map checks, migration backups, exact geometry and collision isolation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
