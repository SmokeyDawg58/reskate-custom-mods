// Authoring-only, lossless compaction. Native bytes and install-chunk ownership
// are preserved; each repeated encoded CAS payload is stored once per archive.
#include "Engine/Resource/toc.h"
#include "Engine/Resource/binary_bundle.h"
#include "Engine/Vfs/game_archives.h"
#include <Windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <tuple>
namespace fs=std::filesystem;namespace fb=dingosdk::frostbite;namespace vfs=dingosdk::vfs;
void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
std::vector<std::byte> read(const fs::path& p){std::ifstream f(p,std::ios::binary|std::ios::ate);check(bool(f),"Input file missing");auto size=f.tellg();check(size>=0,"Input size");std::vector<std::byte> data(static_cast<std::size_t>(size));f.seekg(0);if(size)f.read(reinterpret_cast<char*>(data.data()),size);check(bool(f),"Input read");return data;}
void write(const fs::path& p,std::span<const std::byte> data){fs::create_directories(p.parent_path());std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(data.data()),static_cast<std::streamsize>(data.size()));check(bool(f),"Output write");}
struct Archive {std::vector<std::byte> data;std::map<std::pair<std::array<unsigned char,32>,std::uint32_t>,std::uint32_t> offsets;};
int main(int argc,char** argv){try{
 check(argc==3,"compact input-Patch new-output-Patch");const fs::path input=fs::absolute(argv[1]),output=fs::absolute(argv[2]);check(!fs::exists(output),"Output must be new");check(fs::exists(input/"layout.toc"),"Input layout missing");
 auto layout=vfs::read_layout(input/"layout.toc");vfs::GameArchives source(input,layout.root);source.keep_open();
 BCRYPT_ALG_HANDLE algorithm{};check(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0,"SHA256 provider");
 struct Close{BCRYPT_ALG_HANDLE value;~Close(){BCryptCloseAlgorithmProvider(value,0);}} close{algorithm};
 std::map<fb::CasIdentifier,Archive> archives;
 using Original=std::tuple<fb::CasIdentifier,std::uint32_t,std::uint32_t>;
 std::map<Original,fb::BundleFileInfo> placements;std::vector<fs::path> tocs;
 std::uint64_t references{},duplicates{},unique_bytes{},old_cas_bytes{},new_cas_bytes{},verified{},bundles{},chunks{};
 for(const auto& f:fs::recursive_directory_iterator(input))if(f.is_regular_file()){
  const auto relative=fs::relative(f.path(),input);
  if(f.path().extension()==".toc" && relative.generic_string().starts_with("Win32/"))tocs.push_back(relative);
  else if(f.path().extension()==".cas")old_cas_bytes+=f.file_size();
  else {fs::create_directories((output/relative).parent_path());fs::copy_file(f.path(),output/relative);}
 }
 std::sort(tocs.begin(),tocs.end());check(!tocs.empty(),"No TOCs");
 const auto compact=[&](fb::BundleFileInfo file){
  if(!file.location.patch || !file.size)return file;
  ++references;const Original key{file.location,file.offset,file.size};
  if(const auto found=placements.find(key);found!=placements.end())return found->second;
  auto data=source.read(input,file.location,file.offset,file.size);check(data.size()==file.size,"Encoded payload size");std::array<unsigned char,32> digest{};
  check(BCryptHash(algorithm,nullptr,0,reinterpret_cast<PUCHAR>(data.data()),static_cast<ULONG>(data.size()),digest.data(),static_cast<ULONG>(digest.size()))>=0,"Payload checksum");
  auto& archive=archives[file.location];const auto match=std::pair{digest,file.size};auto result=file;
  if(const auto found=archive.offsets.find(match);found!=archive.offsets.end()){
   check(std::equal(data.begin(),data.end(),archive.data.begin()+found->second),"Hash collision");result.offset=found->second;++duplicates;
  }else{
   check(archive.data.size()+data.size()<UINT32_MAX,"Compacted archive exceeds native offset range");result.offset=static_cast<std::uint32_t>(archive.data.size());archive.offsets.emplace(match,result.offset);archive.data.insert(archive.data.end(),data.begin(),data.end());unique_bytes+=data.size();
  }
  placements.emplace(key,result);return result;
 };
 for(const auto& relative:tocs){auto toc=fb::read_toc(read(input/relative));
  for(auto& bundle:toc.bundles){auto region=fb::read_bundle_region(bundle.region);for(auto& file:region.files)file=compact(file);bundle.region=fb::write_bundle_region(region.files,region.inlineManifest);++bundles;}
  for(auto& chunk:toc.chunks)if(!chunk.removed){const auto result=compact({chunk.location,chunk.offset,chunk.size});chunk.location=result.location;chunk.offset=result.offset;chunk.size=result.size;++chunks;}
  write(output/relative,fb::write_patch_toc(toc.bundles,toc.chunks,toc.flags));
 }
 for(const auto& [location,archive]:archives){write(output/"Win32"/source.directory(location.installChunk)/vfs::archive_file(location.archive),archive.data);new_cas_bytes+=archive.data.size();}
 // Independently read every rewritten TOC and compare every referenced encoded
 // byte range with the original, including raw bundle manifests and chunks.
 auto new_layout=vfs::read_layout(output/"layout.toc");vfs::GameArchives result(output,new_layout.root);result.keep_open();
 const auto same=[&](const fb::BundleFileInfo& before,const fb::BundleFileInfo& after){
  check(before.location==after.location && before.size==after.size,"Native archive ownership changed");
  if(!before.location.patch || !before.size){check(before.offset==after.offset,"Unmodified base reference changed");return;}
  check(source.read(input,before.location,before.offset,before.size)==result.read(output,after.location,after.offset,after.size),"Compacted native payload differs");++verified;
 };
 for(const auto& relative:tocs){const auto before=fb::read_toc(read(input/relative)),after=fb::read_toc(read(output/relative));fb::verify_toc(after);check(before.flags==after.flags && before.bundles.size()==after.bundles.size() && before.chunks.size()==after.chunks.size(),"TOC semantics changed");
  for(std::size_t i=0;i<before.bundles.size();++i){check(before.bundles[i].name==after.bundles[i].name && before.bundles[i].loadFlag==after.bundles[i].loadFlag,"Bundle registration changed");const auto a=fb::read_bundle_region(before.bundles[i].region),b=fb::read_bundle_region(after.bundles[i].region);check(a.inlineManifest==b.inlineManifest && a.files.size()==b.files.size(),"Inline manifest changed");for(std::size_t j=0;j<a.files.size();++j)same(a.files[j],b.files[j]);}
  for(std::size_t i=0;i<before.chunks.size();++i){const auto& a=before.chunks[i];const auto& b=after.chunks[i];check(a.guid==b.guid && a.removed==b.removed,"Chunk registration changed");if(!a.removed)same({a.location,a.offset,a.size},{b.location,b.offset,b.size});}
 }
 check(verified==references && new_cas_bytes<old_cas_bytes,"Compaction did not reduce storage or verify all references");
 std::cout<<"{\"result\":\"PASS\",\"tocs\":"<<tocs.size()<<",\"bundles\":"<<bundles<<",\"chunks\":"<<chunks<<",\"references_verified\":"<<verified<<",\"duplicate_stored_payloads_removed\":"<<duplicates<<",\"old_cas_bytes\":"<<old_cas_bytes<<",\"new_cas_bytes\":"<<new_cas_bytes<<",\"saved_bytes\":"<<old_cas_bytes-new_cas_bytes<<",\"all_encoded_payloads\":\"BYTE_IDENTICAL\",\"archive_ownership\":\"UNCHANGED\"}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
