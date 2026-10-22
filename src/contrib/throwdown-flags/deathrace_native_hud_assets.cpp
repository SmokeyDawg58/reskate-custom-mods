#include "modding/maps/ui/ui_assets.h"
#include "modding/projects/project.h"
#include "modding/mods/fbmod.h"
#include "sdk/ebx/query.h"
#include "sdk/ebx/boxed_value.h"
#include "sdk/hash/hash.h"
#include "../../Extension/UI/NativeMenu/deathrace_native_presentation.h"
#include <fstream>
#include <iostream>
#include <map>
#include <set>
namespace sdk=reskate::sdk;namespace ui=reskate::modding::maps::ui;
namespace ebx=reskate::modding::maps::ebx;namespace projects=reskate::modding::projects;
namespace dr=dingosdk::deathrace_ui;
using Resource=projects::ProjectResource;
void number(sdk::ebx::Object& o,const char* key,double v){if(auto* f=sdk::ebx::find_field(o,key))f->value.data=v;}
void flag(sdk::ebx::Object& o,const char* key,bool v){if(auto* f=sdk::ebx::find_field(o,key))f->value.data=v;}
void layout(sdk::ebx::Object& o,bool child=true) {
 for(const auto* axis:{"HorizontalLayout","VerticalLayout"})if(auto* f=sdk::ebx::find_field(o,axis)){
  auto& a=ui::require_object(f->value,axis);
  for(const auto* k:{"AnchorStart","OffsetStart","OffsetEnd","SizingWeight"})number(a,k,0);
  number(a,"AnchorEnd",child?1:0);number(a,"SizingPivot",child?.5:0);number(a,"SizingWeight",child?0:1);
 }
 for(const auto* key:{"FitWidthToContent","FitHeightToContent"})flag(o,key,false);
 for(const auto* key:{"WidthConstraint","HeightConstraint"})if(auto* f=sdk::ebx::find_field(o,key)){
  auto& constraint=ui::require_object(f->value,key);
  for(const auto* bound:{"Minimum","Maximum"})flag(ui::require_object(ui::require_field(constraint,bound).value,bound),"UseConstraint",false);
 }
 if(auto* f=sdk::ebx::find_field(o,"Padding"))for(auto& v:ui::require_object(f->value,"padding").fields)v.value.data=0.;
}
void color(sdk::ebx::Object& o,std::array<double,3> rgb) {
 // The design and screenshot pixels are sRGB; native Rime tint values are
 // linear. Passing sRGB directly makes the approved ink surfaces pale gray.
 for(auto& c:rgb)c=c<=.04045?c/12.92:std::pow((c+.055)/1.055,2.4);
 for(const auto* key:{"Color","ColorValue"})if(auto* f=sdk::ebx::find_field(o,key)){
  auto* a=&ui::require_object(f->value,key);
  if(std::string_view(key)=="ColorValue"){
   for(const auto* k:{"Asset","AccessibleAsset"})ui::require_field(*a,k).value.data=sdk::ebx::PointerReference{};
   ui::require_field(*a,"SelectedColorId").value.data=std::string{};
   a=&ui::require_object(ui::require_field(*a,"RawColor").value,"raw color");
  }
  for(unsigned i=0;i<3;++i)number(*a,std::array{"x","y","z"}[i],rgb[i]);
 }
}
int main(int argc,char** argv){try {
 if(argc!=4)throw std::runtime_error("native-deathrace-hud game input-project output-project");
 std::cout<<std::unitbuf;
 const auto db=sdk::assets::FrostbiteAssetManager::load_sync(argv[1]);auto project=projects::load_project(argv[2]);
 std::map<std::string,dr::Cell> variants;
 std::map<std::string,dr::Cell> images;
 for(unsigned sc=0;sc<=static_cast<unsigned>(dr::Screen::results);++sc)
 for(unsigned count=0;count<=6;++count)for(bool rotating:{false,true})for(unsigned action=0;action<6;++action){
  dr::Input in;in.screen=static_cast<dr::Screen>(sc);in.rows.resize(count);in.routes.resize(6);
  in.required=8;in.finish=true;in.arrow=rotating;in.roll_axis=rotating;in.error="Validation message";
  in.notification="Required gate passed";in.recovering=true;in.save_hold=in.clear_hold=.5f;
  in.route_action=action;in.selected_route=action;for(unsigned i=0;i<in.rows.size();++i)in.rows[i].self=i%2==0;
  for(auto& c:dr::compose(in).cells){if(c.image.empty() && c.input.empty())variants.emplace(dr::asset_name(c),c);
   else if(!c.image.empty())images.emplace(dr::image_asset_name(c),c);}
 }
 const auto& basic=ui::require_asset(db,"UI/Foundations/Components/Notices/BasicNotice_Widget");
 for(const auto paint:{dr::Paint::red,dr::Paint::yellow}){dr::Cell number;number.box={0,0,96,44};number.paint=paint;number.points=22;number.align=2;variants.emplace(dr::asset_name(number),number);}
 {dr::Input in;in.screen=dr::Screen::builder;for(unsigned a=0;a<15;++a){in.object_alpha=a/15.f;for(const auto& c:dr::compose(in).cells)if(c.alpha<1)variants.emplace(dr::asset_name(c),c);}}
 const auto donor=sdk::ebx::read_document(db.read(basic));
 const auto resident=std::ranges::find(project.resources,"UI/Features/Throwdowns/ContentResources/Throwdown_ModeSelect_ContentResources",&Resource::name);
 if(resident==project.resources.end())throw std::runtime_error("resident UI resource absent");
 const auto resident_name=resident->name;const auto bundles=resident->bundles;
 std::set<std::string> added;
 const auto append=[&](const std::string& name,ebx::Partition& p){
  ui::require_field(*p.root()->object,"Name").value.data=name;const auto a=ui::emit(name,p);
  Resource r;r.kind=projects::ResourceKind::ebx;r.name=name;r.assetGuid=a.partitionGuid;r.payload=a.bytes;r.added=true;r.bundles=bundles;
  if(std::ranges::find(project.resources,name,&Resource::name)!=project.resources.end())throw std::runtime_error("private native identity collision");
  project.resources.push_back(std::move(r));added.insert(name);
 };
 for(unsigned rank=1;rank<=3;++rank){dr::Cell c;c.box={0,0,48,48};c.image="native-rank-"+std::to_string(rank);images.emplace(dr::image_asset_name(c),c);}
 for(const auto& [name,c]:images){
  const auto source=sdk::ebx::read_document(db.read(ui::require_asset(db,"UI/Foundations/Components/Media/Icons/Texture_Widget")));
  ebx::Partition p(source);auto* root=p.find_instance("RimeWidgetEntityData");auto* layer=p.find_instance("RimeLayerEntityData");auto* texture=p.find_instance("RimeTextureElementData");
  if(!root || !layer || !texture)throw std::runtime_error("native texture widget donor changed");
  for(auto* node:{root,texture}){layout(*node->object,false);number(*node->object,"Width",c.box.w*2);number(*node->object,"Height",c.box.h*2);flag(*node->object,"Visible",true);flag(*node->object,"WidgetVisible",true);}
  color(*texture->object,{1,1,1});number(*texture->object,"Alpha",1);
  std::string path=c.image==dr::logo?"UI/ReSkate/Throwdowns/img_race_Placeholder":c.image;
  if(path.starts_with("native-rank-")){const auto rank=path.back();path=std::string("UI/Textures/Common/Activities/ActivityRankBadge/img_ActivityRankBadge_Backing")+rank+"_Sliced_128_"+(rank=='2'?"64_64":"128_128");}
  sdk::ebx::Document art;
  const auto existing=std::ranges::find(project.resources,path,&Resource::name);
  art=existing!=project.resources.end()?sdk::ebx::read_document(existing->payload):sdk::ebx::read_document(db.read(ui::require_asset(db,path)));
  ui::require_field(*texture->object,"Texture").value.data=p.import(art.fileGuid,art.root()->instanceGuid);
  ui::require_field(*layer->object,"Elements").value.data=sdk::ebx::Value::Array{{p.point_to(*texture)}};
  ui::require_field(*root->object,"Components").value.data=sdk::ebx::Value::Array{};
  for(const auto* key:{"PropertyConnections","EventConnections","LinkConnections"})if(auto* f=sdk::ebx::find_field(*p.root()->object,key))f->value.data=sdk::ebx::Value::Array{};
  append(name,p);
 }
 {
  const auto old=std::ranges::find(project.resources,"UI/ReSkate/Throwdowns/EditorGlyph_Widget",&Resource::name);
  if(old==project.resources.end())throw std::runtime_error("working device glyph asset missing");
  auto glyph_doc=sdk::ebx::read_document(old->payload);sdk::ebx::decode_boxed_values(glyph_doc,old->payload);
  ebx::Partition p(glyph_doc);auto* root=p.find_instance("RimeWidgetEntityData");auto* layer=p.find_instance("RimeLayerEntityData");
  if(!root || !layer)throw std::runtime_error("device glyph root missing");
  layout(*root->object,false);number(*root->object,"Width",dr::input_native_size);number(*root->object,"Height",dr::input_native_size);
  std::set<int> bounded;
  for(const auto& value:ui::require_array(ui::require_field(*layer->object,"Elements").value,"glyph render layer")){
   const auto* ptr=sdk::ebx::pointer(value);const auto* child=ptr?sdk::ebx::resolve_internal(p.document(),*ptr):nullptr;
   if(!child || !child->object)throw std::runtime_error("glyph render child missing");
   auto& node=*child->object;layout(node,false);number(node,"Width",dr::input_native_size);number(node,"Height",dr::input_native_size);bounded.insert(ptr->index);
   for(const auto* key:{"WidthConstraint","HeightConstraint"})if(auto* f=sdk::ebx::find_field(node,key))for(const auto* bound:{"Minimum","Maximum"}){
    auto& a=ui::require_object(ui::require_field(ui::require_object(f->value,key),bound).value,bound);number(a,"Value",dr::input_native_size);flag(a,"UseConstraint",true);
   }
  }
  std::set<std::uint32_t> geometry;
  for(const auto* key:{"Width","Height","HorizontalLayout","VerticalLayout","FitWidthToContent","FitHeightToContent","WidthConstraint","HeightConstraint","Padding","Transform"})
   geometry.insert(sdk::hash::fnv1(key));
  auto& links=ui::require_array(ui::require_field(*p.root()->object,"PropertyConnections").value,"glyph links");
  std::erase_if(links,[&](auto& value){auto& link=ui::require_object(value,"glyph link");const auto* target=sdk::ebx::pointer(ui::require_field(link,"Target").value);
   return target && target->kind==sdk::ebx::PointerKind::internal && bounded.contains(target->index) && geometry.contains(static_cast<std::uint32_t>(sdk::ebx::integer(ui::require_field(link,"TargetFieldId").value).value_or(0)));});
  append(dr::input_asset,p);
 }
 for(const auto& [name,cell]:variants) {
  ebx::Partition p(donor);auto& d=p.document();
  auto* root=p.find_instance("RimeWidgetEntityData");auto* layer=p.find_instance("RimeLayerEntityData");
  auto* container=p.find_instance("RimeContainerElementData");auto* label=p.find_instance("RimeLabelElementData");
  auto* fill=p.find_instance("RimeFillElementData");
  // The first BasicNotice fill is its tape outline. Reuse the actual native
  // notice background and its textured fill style instead of a flat rectangle.
  for(auto& node:d.instances)if(node.object && sdk::ebx::type_name(d,node)=="RimeFillElementData")
   if(const auto* f=sdk::ebx::find_field(*node.object,"DebugInstanceName");f && std::get_if<std::string>(&f->value.data) && std::get<std::string>(f->value.data)=="Notice_Background")fill=&node;
  auto* brush=p.find_instance("RimeTextureElementData");
  if(!root || !layer || !container || !label || !fill)throw std::runtime_error("native notice shape changed");
  const auto label_index=p.point_to(*label).index;
  ui::require_field(*p.root()->object,"Name").value.data=name;
  for(auto* node:{root,container,label,fill}){
   // A Rime layer supplies no layout rectangle. Its direct container must
   // measure its authored size; only the fill/label stretch inside that box.
   layout(*node->object,node!=root && node!=container);number(*node->object,"Width",cell.box.w*2);number(*node->object,"Height",cell.box.h*2);
   flag(*node->object,"Visible",true);flag(*node->object,"WidgetVisible",true);
  }
  // A single native container owns fill and text. It cannot grow its stride,
  // inherit a donor icon, or pull a label into an adjacent row.
  ui::require_field(*layer->object,"Elements").value.data=sdk::ebx::Value::Array{{p.point_to(*container)}};
  sdk::ebx::Value::Array children;
  if(cell.paint==dr::Paint::brush && brush)children.push_back({p.point_to(*brush)});
  else if(cell.paint!=dr::Paint::text && cell.paint!=dr::Paint::ink)children.push_back({p.point_to(*fill)});
  children.push_back({p.point_to(*label)});
  ui::require_field(*container->object,"Elements").value.data=children;
  std::array<double,3> background{.067,.075,.082},ink{.949,.945,.929};double alpha=.96;
  switch(cell.paint){
   case dr::Paint::ink:ink={.067,.075,.082};break;
   case dr::Paint::red:background={.929,.224,.204};ink={.067,.075,.082};alpha=1;break;
   case dr::Paint::yellow:background={.969,.808,.243};ink={.067,.075,.082};alpha=1;break;
   case dr::Paint::brush:background={.718,.145,.149};alpha=1;break;
   case dr::Paint::paper:case dr::Paint::field:background={.949,.945,.929};ink={.067,.075,.082};alpha=1;break;
   case dr::Paint::soft:background={.141,.157,.165};alpha=1;break;
   default:break;
  }
  color(*container->object,{1,1,1});number(*container->object,"Alpha",1);
  color(*fill->object,background);number(*fill->object,"Alpha",alpha*cell.alpha);
  // Native notice grain is retained on large panels; compact progress/action
  // strips use the native solid fill so their progress width stays exact.
  if(cell.paint!=dr::Paint::panel && cell.paint!=dr::Paint::paper)
   ui::require_field(*fill->object,"FillStyle").value.data=sdk::ebx::PointerReference{};
  if(cell.paint==dr::Paint::brush && brush){
   layout(*brush->object);number(*brush->object,"Width",cell.box.w*2);number(*brush->object,"Height",cell.box.h*2);
   color(*brush->object,background);number(*brush->object,"Alpha",cell.alpha);flag(*brush->object,"Visible",true);
   const auto texture=sdk::ebx::read_document(db.read(ui::require_asset(db,"UI/Textures/Common/Banner/img_BannerBrush_Large_1600_1600_660")));
   ui::require_field(*brush->object,"Texture").value.data=p.import(texture.fileGuid,texture.root()->instanceGuid);
   // Crop the transparent atlas gutters; text stays inside the painted band.
   for(auto* uv:{&ui::require_field(*brush->object,"UVRect").value,
       &ui::require_field(ui::require_object(ui::require_field(*brush->object,"TextureSource").value,"texture source"),"UVRect").value}){
    auto& rect=ui::require_object(*uv,"UV rect");number(rect,"x",0);number(rect,"y",.103);number(rect,"z",1);number(rect,"w",.82);
   }
  }
  color(*label->object,ink);number(*label->object,"Alpha",cell.alpha);
  number(*label->object,"PointSize",cell.points*2);
  ui::require_field(*label->object,"HorizontalAlignment").value.data=std::int64_t(cell.align==1?1:cell.align==2?2:0);
  ui::require_field(*label->object,"VerticalAlignment").value.data=std::int64_t{2};
  flag(*label->object,"WordWrap",cell.wrap);flag(*label->object,"ParseMarkup",false);flag(*label->object,"StripMarkup",true);
  // Font family/weight still come from the native LabelViewModel's style.
  // All text geometry and paint are private constants from the approved design.
  std::set<std::uint32_t> kept;
  // PropertyConnection IDs use the engine's name hash. EBX schema descriptor
  // hashes are a different namespace; comparing those discarded BOTH live
  // label bindings, leaving valid models and completely blank labels.
  for(const auto* key:{"RawText","FontStyle"})kept.insert(sdk::hash::fnv1(key));
  auto& links=ui::require_array(ui::require_field(*p.root()->object,"PropertyConnections").value,"notice links");
  std::erase_if(links,[&](const auto& value){
   const auto& link=ui::require_object(const_cast<sdk::ebx::Value&>(value),"link");
   const auto* ptr=sdk::ebx::pointer(ui::require_field(const_cast<sdk::ebx::Object&>(link),"Target").value);
   if(!ptr || ptr->kind!=sdk::ebx::PointerKind::internal)return false;
   const auto* target=sdk::ebx::resolve_internal(d,*ptr);if(!target)return false;
   const auto type=sdk::ebx::type_name(d,*target);
   if(!type.starts_with("Rime"))return false;
   const auto hash=static_cast<std::uint32_t>(sdk::ebx::integer(ui::require_field(const_cast<sdk::ebx::Object&>(link),"TargetFieldId").value).value_or(0));
   return ptr->index!=label_index || !kept.contains(hash);
  });
  const auto authored=ui::emit(name,p);
  Resource r;r.kind=projects::ResourceKind::ebx;r.name=name;r.assetGuid=authored.partitionGuid;r.payload=authored.bytes;r.added=true;r.bundles=bundles;
  if(std::ranges::find(project.resources,name,&Resource::name)!=project.resources.end())throw std::runtime_error("new HUD identity collision");
  project.resources.push_back(std::move(r));added.insert(name);
 }
 // Include the exact native texture closure in the same owning UI bundles.
 // This does not override any stock UI or pull in a Block Party controller.
 ui::UiAssets closure;ui::include_rime_texture(db,"UI/Textures/Common/Banner/img_BannerBrush_Large_1600_1600_660",closure);
 for(const auto* texture:{"UI/Textures/Icons/img_Generic_PlayersIcon_64_64_64","UI/Textures/Icons/Generic/img_Icon_TimerOutline_64_32_32",
     "UI/Textures/Common/Activities/ActivityRankBadge/img_ActivityRankBadge_Backing1_Sliced_128_128_128",
     "UI/Textures/Common/Activities/ActivityRankBadge/img_ActivityRankBadge_Backing2_Sliced_128_64_64",
     "UI/Textures/Common/Activities/ActivityRankBadge/img_ActivityRankBadge_Backing3_Sliced_128_128_128"})ui::include_rime_texture(db,texture,closure);
 for(const auto* a:closure.natives){
  const auto kind=a->kind==sdk::assets::AssetKind::ebx?projects::ResourceKind::ebx:a->kind==sdk::assets::AssetKind::resource?projects::ResourceKind::resource:projects::ResourceKind::chunk;
  auto old=std::ranges::find_if(project.resources,[&](const auto& r){return r.kind==kind && (kind==projects::ResourceKind::ebx?r.name==a->name:kind==projects::ResourceKind::resource?r.resourceId==a->resourceId:r.chunkId==a->guid);});
  if(old==project.resources.end()){
   Resource r;r.kind=kind;r.name=a->name;r.payload=db.read(*a);r.resourceId=a->resourceId;r.resourceType=a->resourceType;r.resourceMeta=a->resourceMeta;
   r.assetGuid=a->guid;r.chunkId=a->guid;r.logicalOffset=a->logicalOffset;r.logicalSize=a->logicalSize;
   if(kind==projects::ResourceKind::ebx)r.assetGuid=sdk::ebx::read_document(r.payload).fileGuid;
   r.bundles=bundles;project.resources.push_back(std::move(r));
  }else for(const auto& b:bundles)if(std::ranges::find(old->bundles,b)==old->bundles.end())old->bundles.push_back(b);
 }
 // Explicit imports give these private assets the same lifetime as the native
 // Throwdowns resource on every map, including a rebuilt custom-map domain.
 auto content=std::ranges::find(project.resources,resident_name,&Resource::name);
 auto cd=sdk::ebx::read_document(content->payload);sdk::ebx::decode_boxed_values(cd,content->payload);ebx::Partition cp(cd,false);
 for(const auto& r:project.resources)if(added.contains(r.name)){const auto id=sdk::ebx::read_root_identity(r.payload);(void)cp.import(id.fileGuid,id.classGuid);}
 content->payload=ui::emit(resident_name,cp).bytes;
 project.metadata.version="1.0.20";projects::save_project(project,argv[3]);
 // Re-read the serialized project independently of the mutable author graph.
 // A valid preview is insufficient if serializer layout, IDs or bindings differ.
 const auto saved=projects::load_project(argv[3]);std::set<sdk::io::Guid> identities;
 for(const auto& [name,cell]:variants){
  const auto found=std::ranges::find(saved.resources,name,&Resource::name);
  if(found==saved.resources.end())throw std::runtime_error("serialized HUD skin missing");
  const auto document=sdk::ebx::read_document(found->payload);
  if(!identities.insert(document.fileGuid).second)throw std::runtime_error("HUD skin identity collision");
  for(const auto* type:{"RimeWidgetEntityData","RimeContainerElementData","RimeLabelElementData"}){
   const auto* node=sdk::ebx::find_instance(document,type);if(!node || !node->object)throw std::runtime_error("serialized native element missing");
   for(const auto& [key,expected]:std::array<std::pair<const char*,double>,2>{{{"Width",cell.box.w*2},{"Height",cell.box.h*2}}}){
    const auto* f=sdk::ebx::find_field(*node->object,key);
    if(!f || std::abs(sdk::ebx::number(f->value).value_or(-1)-expected)>.01)throw std::runtime_error("serialized HUD geometry differs");
   }
   for(const auto* key:{"FitWidthToContent","FitHeightToContent"})if(const auto* f=sdk::ebx::find_field(*node->object,key);f && sdk::ebx::boolean(f->value).value_or(true))throw std::runtime_error("native HUD auto-size escaped its frame");
  }
  const auto* label=sdk::ebx::find_instance(document,"RimeLabelElementData");
  std::set<std::uint32_t> label_inputs;
  for(const auto& value:ui::require_array(ui::require_field(*document.root()->object,"PropertyConnections").value,"serialized label links")){
   auto& link=ui::require_object(const_cast<sdk::ebx::Value&>(value),"serialized link");
   const auto* target=sdk::ebx::pointer(ui::require_field(link,"Target").value);
   if(target && sdk::ebx::resolve_internal(document,*target)==label)
    label_inputs.insert(static_cast<std::uint32_t>(sdk::ebx::integer(ui::require_field(link,"TargetFieldId").value).value_or(0)));
  }
  if(!label_inputs.contains(sdk::hash::fnv1("RawText")) || !label_inputs.contains(sdk::hash::fnv1("FontStyle")))
   throw std::runtime_error("serialized native labels lost live text/font binding");
  const auto* parent=sdk::ebx::find_instance(document,"RimeContainerElementData");
  for(const auto* key:{"HorizontalLayout","VerticalLayout"}){
   const auto& axis=ui::require_object(ui::require_field(*parent->object,key).value,key);
   if(sdk::ebx::number(ui::require_field(const_cast<sdk::ebx::Object&>(axis),"AnchorEnd").value).value_or(-1)!=0 ||
      sdk::ebx::number(ui::require_field(const_cast<sdk::ebx::Object&>(axis),"SizingWeight").value).value_or(-1)!=1)
      throw std::runtime_error("native root container would collapse without a parent");
  }
  if(sdk::ebx::boolean(ui::require_field(*label->object,"ParseMarkup").value).value_or(true))throw std::runtime_error("native player label markup enabled");
  if(sdk::ebx::number(ui::require_field(*label->object,"PointSize").value).value_or(-1)!=cell.points*2)throw std::runtime_error("serialized native font scale differs");
 }
 for(const auto& [name,c]:images){
  const auto found=std::ranges::find(saved.resources,name,&Resource::name);if(found==saved.resources.end())throw std::runtime_error("native image skin missing");
  const auto doc=sdk::ebx::read_document(found->payload);const auto* root=sdk::ebx::find_instance(doc,"RimeWidgetEntityData");const auto* texture=sdk::ebx::find_instance(doc,"RimeTextureElementData");
  if(!root || !texture)throw std::runtime_error("native immutable artwork missing");
  if(!sdk::ebx::array(ui::require_field(*root->object,"Components").value)->empty())throw std::runtime_error("native image still has a sizing helper");
  for(const auto* node:{root,texture})for(const auto* key:{"HorizontalLayout","VerticalLayout"})if(auto* f=sdk::ebx::find_field(*node->object,key)){
   auto& axis=ui::require_object(f->value,key);if(sdk::ebx::number(ui::require_field(axis,"SizingWeight").value).value_or(-1)!=1)throw std::runtime_error("native static image root can collapse");
  }
 }
 {
  const auto found=std::ranges::find(saved.resources,dr::input_asset,&Resource::name);if(found==saved.resources.end())throw std::runtime_error("bounded glyph missing");
  const auto doc=sdk::ebx::read_document(found->payload);const auto* layer=sdk::ebx::find_instance(doc,"RimeLayerEntityData");
  for(const auto& v:ui::require_array(ui::require_field(*layer->object,"Elements").value,"glyph bounds")){
   const auto* node=sdk::ebx::resolve_internal(doc,*sdk::ebx::pointer(v));
   for(const auto* key:{"Width","Height"})if(sdk::ebx::number(ui::require_field(*node->object,key).value).value_or(-1)!=dr::input_native_size)throw std::runtime_error("native glyph size escaped its bounds");
  }
 }
 auto mod=std::filesystem::path(argv[3]);mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,argv[1],&db);
 std::cout<<"PASS native Deathrace skins="<<variants.size()<<" original resources retained; no native gameplay graph changed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
