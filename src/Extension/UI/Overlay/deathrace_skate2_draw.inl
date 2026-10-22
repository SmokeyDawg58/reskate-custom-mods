#include "deathrace_skate2_art.h"
namespace dingosdk::deathrace_ui {
inline const Skate2Sprite* skate2_sprite(std::string_view name){for(const auto& s:skate2_sprites)if(s.name==name)return &s;return nullptr;}
inline void skate2_image(ImDrawList& draw,ImTextureID texture,std::string_view name,ImVec2 a,ImVec2 b,ImU32 tint){if(const auto* s=skate2_sprite(name);s && texture)draw.AddImage(texture,a,b,{s->uv[0],s->uv[1]},{s->uv[2],s->uv[3]},tint);}
inline unsigned skate2_code(std::string_view text,std::size_t& at){const auto first=static_cast<unsigned char>(text[at++]);if(first<128)return first;const unsigned count=(first&0xe0)==0xc0?2:(first&0xf0)==0xe0?3:4;unsigned value=first&((1u<<(7-count))-1);for(unsigned i=1;i<count && at<text.size();++i)value=(value<<6)|(static_cast<unsigned char>(text[at++])&63);return value;}
inline const Skate2Glyph* skate2_glyph(const Skate2Font& font,unsigned code){for(std::size_t i=0;i<font.count;++i)if(font.glyphs[i].code==code)return font.glyphs+i;return nullptr;}
inline ImVec2 skate2_extent(const Skate2Font& font,std::string_view text,float points,ImFont* fallback){float x{},width{},bottom{},y{};for(std::size_t i=0;i<text.size();){const auto start=i;const auto ch=skate2_code(text,i);if(ch=='\n'){width=std::max(width,x);x=0;y+=points*1.15f;continue;}if(const auto* g=skate2_glyph(font,ch)){bottom=std::max(bottom,y+(font.ascent+g->y+g->height)*font.sy*points);x+=g->advance*font.sx*points;}else if(fallback){const auto value=text.substr(start,i-start);x+=fallback->CalcTextSizeA(points,FLT_MAX,0,value.data(),value.data()+value.size()).x;bottom=std::max(bottom,y+points);}}return {std::max(width,x),std::max(points*.75f,bottom)};}
inline void skate2_text(ImDrawList& draw,ImTextureID texture,const Skate2Font& font,std::string_view text,ImVec2 at,float points,ImU32 tint,ImFont* fallback){float x=at.x,y=at.y;for(std::size_t i=0;i<text.size();){const auto start=i;const auto ch=skate2_code(text,i);if(ch=='\n'){x=at.x;y+=points*1.15f;continue;}if(const auto* g=skate2_glyph(font,ch)){if(g->width>0 && g->height>0){const ImVec2 a{x+g->x*font.sx*points,y+(font.ascent+g->y)*font.sy*points},b{a.x+g->width*font.sx*points,a.y+g->height*font.sy*points};draw.AddImage(texture,a,b,{g->uv[0],g->uv[1]},{g->uv[2],g->uv[3]},tint);}x+=g->advance*font.sx*points;}else if(fallback){const auto value=text.substr(start,i-start);draw.AddText(fallback,points,{x,y},tint,value.data(),value.data()+value.size());x+=fallback->CalcTextSizeA(points,FLT_MAX,0,value.data(),value.data()+value.size()).x;}}}
inline void draw_scene(ImDrawList& draw,const DrawContext& ctx,const Scene& scene){
    const Frame frame(ctx.display.x/std::max(1.f,ctx.display.y));const float unit=ctx.display.y/1080.f*frame.scale;
    const auto xy=[&](float x,float y){const auto p=frame.normalized(x,y);return ImVec2{ctx.origin.x+p[0]*ctx.display.x,ctx.origin.y+p[1]*ctx.display.y};};
    const auto color=[](unsigned r,unsigned g,unsigned b,float a=1.f){return IM_COL32(r,g,b,static_cast<unsigned>(255.f*std::clamp(a,0.f,1.f)));};
    for(const auto& c:scene.cells){
        if(c.alpha<=0 || c.fraction<=0)continue;const auto a=xy(c.box.x,c.box.y),b=xy(c.box.x+c.box.w,c.box.y+c.box.h);const float alpha=c.alpha*std::clamp(ctx.entrance,0.f,1.f);if(b.x<=a.x || b.y<=a.y)continue;
        draw.PushClipRect(a,b,true);auto ink=color(255,244,220,alpha);
        if(c.paint==Paint::panel){
            if(const auto* art=skate2_sprite("panel");art && ctx.atlas)draw.AddImageRounded(ctx.atlas,a,b,{art->uv[0],art->uv[1]},{art->uv[2],art->uv[3]},color(255,255,255,ctx.options.opacity*alpha),10*unit);
        }
        else if(c.paint==Paint::selected || c.paint==Paint::danger){draw.AddRectFilled(a,b,color(105,24,29,(c.interactive?.86f:.60f)*alpha));draw.AddRectFilled(a,{a.x+2*unit,b.y},color(217,51,56,.95f*alpha));if(c.interactive)draw.AddRect(a,b,color(226,118,101,.65f*alpha),3*unit,0,unit);}
        else if(c.paint==Paint::soft){draw.AddRectFilled(a,b,color(21,18,17,(c.interactive?.78f:.27f)*alpha),c.interactive?3*unit:0);if(c.interactive)draw.AddRect(a,b,color(220,196,160,.36f*alpha),3*unit,0,unit);}
        else if(c.paint==Paint::field){draw.AddRectFilled(a,b,color(17,13,12,.72f*alpha));draw.AddRectFilled({a.x,b.y-2*unit},b,color(180,41,50,.95f*alpha));if(ctx.blank_name)ink=color(191,180,164,alpha);}
        else if(c.paint==Paint::line)draw.AddRectFilled(a,b,color(220,196,160,.20f*alpha));
        else if(c.paint==Paint::red || c.paint==Paint::yellow)draw.AddRectFilled(a,xy(c.box.x+c.box.w*c.fraction,c.box.y+c.box.h),c.paint==Paint::red?color(179,39,49,alpha):color(245,213,92,alpha));
        else if(c.paint==Paint::gate_tag || c.paint==Paint::finish_tag || c.paint==Paint::guide_tag){skate2_image(draw,ctx.atlas,"panel",a,b,color(255,255,255,.75f*alpha));const auto accent=c.paint==Paint::finish_tag?color(242,74,62,alpha):c.paint==Paint::guide_tag?color(188,193,194,alpha):color(255,225,91,alpha);draw.AddRectFilled({a.x,b.y-2*unit},b,accent);ink=accent;}
        if(c.paint==Paint::muted)ink=color(211,201,184,alpha);else if(c.paint==Paint::gold || c.paint==Paint::success)ink=color(255,230,139,alpha);
        if(!c.input.empty())skate2_image(draw,ctx.atlas,glyph_sprite(c.input),a,b,color(255,255,255,alpha));
        else if(!c.image.empty())skate2_image(draw,ctx.atlas,c.image==logo?"logo":c.image,a,b,color(255,255,255,alpha));
        else if(!c.text.empty()){
            const bool ordinal=c.paint==Paint::podium,count=c.paint==Paint::countdown;
            ImFont* font=c.paint==Paint::player_name && ctx.name_font?ctx.name_font:c.marker?ctx.marker:ctx.body;
            if(ordinal || (count && c.text!="GO!"))font=ctx.display_font?ctx.display_font:ctx.body;
            if(c.paint==Paint::race_clock){font=ctx.body;for(auto* baked:ctx.clock_fonts)if(baked){font=baked;if(baked->FontSize>=c.points*unit)break;}}
            if(c.paint==Paint::muted && ctx.secondary)font=ctx.secondary;
            if(!font)font=ImGui::GetFont();
            const std::string suffix=ordinal?std::string(place_suffix(c.rank)):std::string{};
            ImFont* tail_font=ctx.body?ctx.body:font;
            float points=(count && c.text=="GO!"?86.f:c.points)*unit;
            const auto measure=[&](ImFont* f,std::string_view text,float px){return f->CalcTextSizeA(px,FLT_MAX,0,text.data(),text.data()+text.size());};
            auto size=measure(font,c.text,points),tail=measure(tail_font,suffix,points*.30f);
            const float inset=c.paint==Paint::field?14*unit:0.f;
            const auto total_text=c.total?ordinal_total(c.total):std::string{};const auto total_size=measure(tail_font,total_text,points*.34f);
            const float fit=std::min({1.f,(b.x-a.x-2*inset-(ordinal?6*unit:0.f))/std::max(1.f,size.x+tail.x+total_size.x+(c.total?12*unit:0.f)),(b.y-a.y)/std::max(1.f,size.y)});
            points*=fit;size=measure(font,c.text,points);tail=measure(tail_font,suffix,points*.30f);
            const float total=size.x+(ordinal?tail.x+4*unit:0.f);
            const float tx=std::round(c.align==1?b.x-total-inset:c.align==2?a.x+(b.x-a.x-total)*.5f:a.x+inset);
            const float ty=std::round(a.y+std::max(0.f,(b.y-a.y-size.y)*.5f));
            if(ordinal)ink=c.rank==1?color(255,224,113,alpha):c.rank==2?color(223,230,233,alpha):c.rank==3?color(228,179,132,alpha):ink;
            const auto lettering=[&](ImU32 tint,float dx,float dy){
                unsigned line{};std::size_t at{};
                do{const auto end=c.text.find('\n',at);const auto text=std::string_view(c.text).substr(at,end==std::string::npos?c.text.size()-at:end-at);const auto extent=measure(font,text,points);
                    const float left=ordinal?tx:std::round(c.align==1?b.x-extent.x-inset:c.align==2?a.x+(b.x-a.x-extent.x)*.5f:a.x+inset);
                    const auto start_vertex=draw.VtxBuffer.Size;
                    draw.AddText(font,points,{left+dx,ty+line*points+dy},tint,text.data(),text.data()+text.size());
                    // A small shear keeps the racing numeral's energy, without enlarging bitmap art or stacking blurred glow copies.
                    if(ordinal)for(int v=start_vertex;v<draw.VtxBuffer.Size;++v)draw.VtxBuffer[v].pos.x+=(ty+points-draw.VtxBuffer[v].pos.y)*.08f;
                    if(end==std::string::npos)break;at=end+1;++line;
                }while(at<c.text.size());
            };
            if(ordinal || count)lettering(color(8,5,3,.85f*alpha),unit,unit);
            lettering(ink,0,0);
            if(ordinal)draw.AddText(tail_font,points*.30f,{std::round(tx+size.x+7*unit),std::round(ty+points*.12f)},ink,suffix.c_str());
            if(ordinal && c.total)draw.AddText(tail_font,points*.34f,{std::round(tx+size.x+tail.x+15*unit),std::round(ty+points*.58f)},ink,total_text.c_str());
            if(c.paint==Paint::field && ctx.text_entry && std::fmod(ctx.seconds,1.f)<.58f){const auto xx=ctx.blank_name?a.x+inset:std::min(b.x-3*unit,tx+size.x+3*unit);draw.AddRectFilled({xx,a.y+9*unit},{xx+unit,b.y-9*unit},ink);}
        }
        if(c.interactive && ctx.pointer.x>=a.x && ctx.pointer.x<=b.x && ctx.pointer.y>=a.y && ctx.pointer.y<=b.y)draw.AddRectFilled({a.x,b.y-2*unit},b,color(216,62,64,alpha));
        draw.PopClipRect();
    }
}
}
