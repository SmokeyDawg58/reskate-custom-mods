#pragma once
#include "Extension/UI/NativeMenu/deathrace_native_presentation.h"
#include "deathrace_art.h"
#include <imgui.h>
#include <string_view>

namespace dingosdk::deathrace_ui {
struct DrawContext {
    ImFont* body{};ImFont* marker{};ImTextureID atlas{};ImVec2 origin{},display{1920,1080};
    ImFont* secondary{};float seconds{},entrance=1;ImVec2 pointer{-1000,-1000};bool text_entry{},blank_name{};float beat=1;ImFont* display_font{};RedlineOptions options;float gate_pulse{};bool race_active{};std::array<ImFont*,3> clock_fonts{};ImFont* name_font{};
};
inline std::string_view glyph_sprite(std::string_view pia) {
    constexpr std::array<std::pair<std::string_view,std::string_view>,19> names{{
        {"UI_TabSecondaryNavLeft","ltrigger"},{"UI_TabSecondaryNavRight","rtrigger"},{"UI_TabPrimaryNavLeft","lbumper"},{"UI_TabPrimaryNavRight","rbumper"},
        {"UI_PrimaryAction","a"},{"UI_SecondaryAction","x"},{"UI_DpadLeft","dpad-left"},{"UI_DpadRight","dpad-right"},{"UI_DpadUp","dpad-up"},{"UI_DpadDown","dpad-down"},
        {"UI_Activities_AccessPause","options"},{"UI_Back","b"},{"UI_TertiaryAction","y"},{"UI_R3Click","rstick-click"},{"UI_L3Click","lstick-click"},{"UI_SelectButton","menu"},{"UI_DpadHorizontal","dpad-horizontal"},{"UI_DpadVertical","dpad-vertical"},{"UI_LeftStick","lstick"}}};
    for(const auto& [name,sprite]:names)if(name==pia)return sprite;return {};
}
inline void draw_scene_legacy(ImDrawList& draw,const DrawContext& ctx,const Scene& scene) {
    const Frame frame(ctx.display.x/std::max(1.f,ctx.display.y));
    const float unit=ctx.display.y/1080.f*frame.scale;
    const auto xy=[&](float x,float y){const auto p=frame.normalized(x,y);return ImVec2(ctx.origin.x+p[0]*ctx.display.x,ctx.origin.y+p[1]*ctx.display.y);};
    const auto tint=[](unsigned r,unsigned g,unsigned b,float a=1.f){return IM_COL32(r,g,b,static_cast<unsigned>(255.f*std::clamp(a,0.f,1.f)));};
    const auto sprite=[&](std::string_view name,ImVec2 a,ImVec2 b,ImU32 color){
        if(!ctx.atlas)return;for(const auto& art:sprites)if(art.name==name){draw.AddImage(ctx.atlas,a,b,ImVec2(art.uv[0],art.uv[1]),ImVec2(art.uv[2],art.uv[3]),color);break;}
    };
    for(const auto& c:scene.cells){
        if(c.alpha<=0 || c.fraction<=0)continue;
        const auto a=xy(c.box.x,c.box.y),b=xy(c.box.x+c.box.w,c.box.y+c.box.h);const float alpha=c.alpha*std::clamp(ctx.entrance,0.f,1.f);
        const bool hover=c.interactive && ctx.pointer.x>=a.x && ctx.pointer.x<=b.x && ctx.pointer.y>=a.y && ctx.pointer.y<=b.y;
        draw.PushClipRect(a,b,true);
        if(c.paint==Paint::race_clock){
            auto* font=ctx.display_font?ctx.display_font:ctx.body;
            for(auto* baked:ctx.clock_fonts)if(baked){font=baked;if(baked->FontSize>=c.points*unit)break;}
            if(!font){draw.PopClipRect();continue;}
            if(c.text.find_first_not_of("0123456789.: /-")!=std::string::npos){
                auto* fallback=ctx.body?ctx.body:font;const float points=c.points*unit;
                draw.AddText(fallback,points,a,tint(244,245,247,alpha),c.text.c_str());draw.PopClipRect();continue;
            }
            float points=c.points*unit,digit{};for(char ch='0';ch<='9';++ch){const char glyph[]{ch,0};digit=std::max(digit,font->CalcTextSizeA(points,FLT_MAX,0,glyph).x);}
            const auto decimal=c.text.find('.');float width{};
            for(std::size_t i=0;i<c.text.size();++i)width+=digit*(i>=decimal?.58f:c.text[i]==':'?.48f:1.f);
            const float fit=std::min(1.f,(b.x-a.x)/std::max(1.f,width));points*=fit;digit*=fit;
            float x=a.x;for(std::size_t i=0;i<c.text.size();++i){const bool precision=i>=decimal;const float size=points*(precision?.58f:1.f),advance=digit*(precision?.58f:c.text[i]==':'?.48f:1.f);
                const char glyph[]{c.text[i],0};const float actual=font->CalcTextSizeA(size,FLT_MAX,0,glyph).x;
                const ImVec2 at{x+(advance-actual)*.5f,a.y+(b.y-a.y-points)*.5f+(precision?points*.34f:0.f)};
                draw.AddText(font,size,ImVec2(at.x,at.y+unit),tint(0,0,0,.8f*alpha),glyph);
                draw.AddText(font,size,at,tint(244,245,247,alpha),glyph);x+=advance;
            }
            draw.PopClipRect();continue;
        }
        if(c.paint==Paint::podium || c.paint==Paint::countdown){
            auto* font=ctx.display_font?ctx.display_font:ctx.body;if(!font){draw.PopClipRect();continue;}
            auto* secondary=ctx.secondary?ctx.secondary:ctx.body;
            const bool count=c.paint==Paint::countdown,gold=c.rank==1 || (count && c.text=="GO!"),silver=c.rank==2,bronze=c.rank==3;
            const auto top=gold?std::array{255.f,249.f,182.f}:silver?std::array{248.f,253.f,255.f}:bronze?std::array{255.f,219.f,172.f}:std::array{255.f,249.f,237.f};
            const auto bottom=gold?std::array{231.f,165.f,28.f}:silver?std::array{139.f,164.f,188.f}:bronze?std::array{190.f,113.f,53.f}:count?std::array{255.f,114.f,89.f}:std::array{208.f,218.f,231.f};
            float points=c.points*unit;if(count){const float t=std::clamp(ctx.beat,0.f,1.f);points*=.84f+.16f*(1.f-(1.f-t)*(1.f-t)*(1.f-t));}
            const auto suffix=count?std::string{}:std::string(place_suffix(c.rank));
            auto size=font->CalcTextSizeA(points,FLT_MAX,0,c.text.c_str());
            float suffix_points=points*.34f;auto tail=secondary?secondary->CalcTextSizeA(suffix_points,FLT_MAX,0,suffix.c_str()):ImVec2{};
            const float fit=std::min({1.f,(b.x-a.x-6*unit)/std::max(1.f,size.x+tail.x+3*unit),(b.y-a.y-4*unit)/std::max(1.f,size.y)});
            points*=fit;suffix_points*=fit;size=font->CalcTextSizeA(points,FLT_MAX,0,c.text.c_str());if(secondary)tail=secondary->CalcTextSizeA(suffix_points,FLT_MAX,0,suffix.c_str());
            const float total=size.x+(count?0.f:tail.x+3*unit),xx=count?a.x+(b.x-a.x-total)*.5f:a.x+2*unit,yy=a.y+(b.y-a.y-size.y)*.5f;
            const auto lettering=[&](ImFont* f,float pt,ImVec2 at,const std::string& text,bool glow){
                if(!f || text.empty())return;const float stroke=(count?2.f:1.f)*unit;
                for(const auto offset:std::array<ImVec2,4>{{{-stroke,0},{stroke,0},{0,-stroke},{0,stroke}}})draw.AddText(f,pt,ImVec2(at.x+offset.x,at.y+offset.y),tint(3,6,12,.9f*alpha),text.c_str());
                if(glow){const float pulse=ctx.gate_pulse*ctx.options.effects;for(const float radius:{2.f,4.f})for(const float direction:{-1.f,1.f})
                    draw.AddText(f,pt,ImVec2(at.x+direction*radius*unit,at.y),tint(255,195,51,.09f*pulse*alpha),text.c_str());}
                const int begin=draw.VtxBuffer.Size;draw.AddText(f,pt,at,tint(255,255,255,alpha),text.c_str());
                const float low=at.y+pt*.2f,span=std::max(1.f,pt*.65f);
                for(int i=begin;i<draw.VtxBuffer.Size;++i){auto& v=draw.VtxBuffer[i];const float t=std::clamp((v.pos.y-low)/span,0.f,1.f);
                    v.col=tint(static_cast<unsigned>(top[0]+(bottom[0]-top[0])*t),static_cast<unsigned>(top[1]+(bottom[1]-top[1])*t),static_cast<unsigned>(top[2]+(bottom[2]-top[2])*t),alpha);}
            };
            lettering(font,points,ImVec2(xx,yy),c.text,gold && !count && ctx.gate_pulse>0 && !ctx.options.reduced_motion);
            if(!count)lettering(secondary,suffix_points,ImVec2(xx+size.x+3*unit,yy+points*.1f),suffix,false);
            draw.PopClipRect();continue;
        }
        if(!c.input.empty() || !c.image.empty()){
            const auto name=!c.input.empty()?glyph_sprite(c.input):c.image==logo?std::string_view("logo"):c.image=="native-rank-1"?"rank1":c.image=="native-rank-2"?"rank2":c.image=="native-rank-3"?"rank3":c.image.find("TimerOutline")!=std::string::npos?"timer":"people";
            sprite(name,a,b,tint(255,255,255,alpha));
        }else{
            ImU32 ink=tint(244,242,237,alpha);
            const auto gradient=[&](ImU32 top,ImU32 bottom){draw.AddRectFilledMultiColor(a,b,top,top,bottom,bottom);};
            const auto strip=[&](float x,float y,float w,float h,ImU32 color){draw.AddRectFilled(xy(x,y),xy(x+w,y+h),color);};
            if(c.paint==Paint::gate_frame || c.paint==Paint::gate_target || c.paint==Paint::race_notice || c.paint==Paint::race_label){
                const bool target=c.paint==Paint::gate_target,notice=c.paint==Paint::race_notice,label=c.paint==Paint::race_label;
                const float opacity=std::clamp(ctx.options.opacity,.82f,1.f),pulse=ctx.options.reduced_motion?0.f:ctx.gate_pulse*ctx.options.effects;
                gradient(tint(16,17,22,opacity*alpha),tint(16,17,22,(opacity-.02f)*alpha));
                const auto stroke=[&](float xx,float yy,float ww,float hh,ImU32 color){strip(xx,yy,ww,hh,color);};
                if(!label){const float length=target?9.f:std::min(28.f,c.box.w*.15f),weight=target?2.f:1.5f;
                    const auto edge=notice?tint(255,197,89,alpha):tint(255,48,79,alpha);
                    for(bool right:{false,true}){const float xx=right?c.box.x+c.box.w-weight:c.box.x;
                        stroke(xx,c.box.y,weight,length,edge);stroke(xx,c.box.y+c.box.h-length,weight,length,edge);
                        stroke(right?c.box.x+c.box.w-length:c.box.x,c.box.y,length,weight,edge);stroke(right?c.box.x+c.box.w-length:c.box.x,c.box.y+c.box.h-weight,length,weight,edge);
                    }
                    // A cheap bounded edge glow, not a bloom/blur pass. Text
                    // stays sharp and never enters an effects render target.
                    const float effect=ctx.options.effects*(target?.18f:.08f)+pulse*.35f;
                    if(effect>0){for(unsigned layer=1;layer<=3;++layer){const float spread=layer*unit;
                        draw.AddRectFilled(ImVec2(a.x,a.y),ImVec2(a.x+spread,b.y),tint(255,48,79,effect*alpha/(4.f*layer)));
                        draw.AddRectFilled(ImVec2(b.x-spread,a.y),ImVec2(b.x,b.y),tint(255,48,79,effect*alpha/(4.f*layer)));
                    }}
                    if(pulse>0 && !target){const float progress=1.f-std::sqrt(std::clamp(ctx.gate_pulse,0.f,1.f));
                        stroke(c.box.x+c.box.w*progress,c.box.y+c.box.h-2,std::min(36.f,c.box.w*(1-progress)),2,tint(244,245,247,pulse*alpha));
                        const float inset=6*(1-progress);const auto light=tint(255,102,122,pulse*.25f*alpha);
                        stroke(c.box.x+inset,c.box.y+inset,18,1,light);stroke(c.box.x+inset,c.box.y+inset,1,18,light);
                        stroke(c.box.x+c.box.w-inset-18,c.box.y+c.box.h-inset-1,18,1,light);stroke(c.box.x+c.box.w-inset-1,c.box.y+c.box.h-inset-18,1,18,light);
                    }
                }
                ink=tint(244,245,247,alpha);
            }else if(c.paint==Paint::gate_tag || c.paint==Paint::guide_tag || c.paint==Paint::finish_tag){
                const bool finish=c.paint==Paint::finish_tag,guide=c.paint==Paint::guide_tag;
                const auto accent=finish?tint(255,80,72,alpha):guide?tint(154,168,183,alpha):tint(255,220,83,alpha);
                gradient(tint(27,32,40,.94f*alpha),tint(9,13,20,.94f*alpha));
                sprite("smallbrush",a,b,finish?tint(205,52,53,.16f*alpha):guide?tint(160,175,190,.09f*alpha):tint(240,193,48,.13f*alpha));
                strip(c.box.x,c.box.y,2,c.box.h,accent);strip(c.box.x,c.box.y+c.box.h-1,c.box.w,1,accent);ink=accent;
            }else if(c.paint==Paint::panel){
                gradient(tint(26,32,41,.97f*alpha),tint(10,14,20,.97f*alpha));
                sprite("grunge",a,b,tint(137,150,165,.045f*alpha));
                strip(c.box.x,c.box.y,c.box.w,1,tint(175,186,198,.25f*alpha));
                strip(c.box.x,c.box.y+c.box.h-1,c.box.w,1,tint(0,0,0,.8f*alpha));
                strip(c.box.x,c.box.y,2,c.box.h,tint(213,43,47,.75f*alpha));
            }else if(c.paint==Paint::brush){
                draw.AddRectFilledMultiColor(a,b,tint(140,21,30,alpha),tint(203,36,43,alpha),tint(121,18,30,alpha),tint(64,13,24,alpha));
                sprite("brush",a,b,tint(249,144,125,.19f*alpha));
                strip(c.box.x,c.box.y,c.box.w,2,tint(255,104,91,.85f*alpha));
                strip(c.box.x,c.box.y+c.box.h-1,c.box.w,1,tint(27,3,10,.9f*alpha));
            }else if(c.paint==Paint::selected || c.paint==Paint::danger){
                draw.AddRectFilledMultiColor(a,b,tint(91,26,36,alpha),tint(43,23,32,alpha),tint(24,16,25,alpha),tint(61,19,31,alpha));
                strip(c.box.x,c.box.y,3,c.box.h,tint(255,70,65,alpha));
                strip(c.box.x,c.box.y,c.box.w,1,tint(255,117,107,.28f*alpha));
            }else if(c.paint==Paint::field){
                gradient(tint(12,17,24,alpha),tint(21,27,36,alpha));
                strip(c.box.x,c.box.y+c.box.h-2,c.box.w,2,tint(238,66,62,.8f*alpha));
                if(ctx.blank_name)ink=tint(133,146,161,alpha);
            }else if(c.paint==Paint::soft){
                gradient(hover?tint(44,53,65,alpha):tint(31,38,47,.94f*alpha),tint(17,22,30,.94f*alpha));
                strip(c.box.x,c.box.y,c.box.w,1,tint(175,186,198,(hover?.32f:.11f)*alpha));
            }else if(c.paint==Paint::line)gradient(tint(143,157,176,.18f*alpha),tint(143,157,176,.18f*alpha));
            else if(c.paint==Paint::muted)ink=tint(174,180,194,alpha);
            else if(c.paint==Paint::accent)ink=tint(255,117,105,alpha);
            else if(c.paint==Paint::success)ink=tint(127,216,174,alpha);
            else if(c.paint!=Paint::text && c.paint!=Paint::ink){
                const auto color=c.paint==Paint::red?tint(255,48,79,alpha):c.paint==Paint::yellow?tint(247,206,62,alpha):c.paint==Paint::paper || c.paint==Paint::field?tint(242,241,237,alpha):tint(36,40,42,alpha);
                draw.AddRectFilled(a,xy(c.box.x+c.box.w*c.fraction,c.box.y+c.box.h),color);
                if(c.paint==Paint::paper || c.paint==Paint::field)ink=tint(17,19,21,alpha);
            }
            if(hover){strip(c.box.x,c.box.y+c.box.h-2,c.box.w,2,tint(255,86,77,.8f*alpha));}
            // Restrained moving light on hold/progress bars. No motion changes
            // pointer targets or covers the game's reserved HUD regions.
            if((!ctx.race_active || ctx.gate_pulse>0) && !ctx.options.reduced_motion && ctx.options.effects>0 && (c.paint==Paint::red || c.paint==Paint::yellow) && c.box.h<=6 && c.fraction>0){
                const float at=std::fmod(ctx.seconds*.45f,1.f),span=c.box.w*c.fraction;
                const float xx=c.box.x+span*at,ww=std::min(28.f,span*(1.f-at));
                strip(xx,c.box.y,ww,c.box.h,tint(255,255,255,.24f*alpha));
            }
            if(c.paint==Paint::ink)ink=tint(17,19,21,alpha);
            if(!c.text.empty()){
                auto* font=c.marker?ctx.marker:ctx.secondary && (c.paint==Paint::muted || c.points<=19)?ctx.secondary:ctx.body;if(!font)font=ImGui::GetFont();
                float points=c.points*unit;if(c.marker && c.points>=100)points*=.98f+.02f*std::sin(ctx.seconds*7.f);const float inset=(c.paint==Paint::field?16.f:0.f)*unit;
                const float width=std::max(1.f,b.x-a.x-2*inset),height=std::max(1.f,b.y-a.y);
                auto extent=font->CalcTextSizeA(points,FLT_MAX,0,c.text.c_str());
                // Measure with the very font which will render, keeping whole
                // lines, logos and text inside the same physical pixel frame.
                const float fit=std::min({1.f,width/std::max(1.f,extent.x),height/std::max(1.f,extent.y)});
                points*=fit;extent=font->CalcTextSizeA(points,FLT_MAX,0,c.text.c_str());
                float y=a.y+(height-extent.y)*.5f;std::size_t at{};
                do{const auto end=c.text.find('\n',at);const auto len=end==std::string::npos?c.text.size()-at:end-at;
                    const auto* start=c.text.c_str()+at;const auto* finish=start+len;const auto size=font->CalcTextSizeA(points,FLT_MAX,0,start,finish);
                    const float x=c.align==1?b.x-inset-size.x:c.align==2?a.x+(b.x-a.x-size.x)*.5f:a.x+inset;
                    if(c.marker)draw.AddText(font,points,ImVec2(x+unit,y+2*unit),tint(0,0,0,.3f*alpha),start,finish);
                    draw.AddText(font,points,ImVec2(x,y),ink,start,finish);
                    if(c.paint==Paint::field && ctx.text_entry && end==std::string::npos && std::fmod(ctx.seconds,1.f)<.58f){
                        const float caret=std::min(b.x-inset-2*unit,ctx.blank_name?a.x+inset:x+size.x+3*unit);
                        draw.AddRectFilled(ImVec2(caret,y+3*unit),ImVec2(caret+1.5f*unit,y+points-2*unit),tint(255,170,158,alpha));
                    }
                    y+=points;
                    if(end==std::string::npos)break;at=end+1;
                }while(at<c.text.size());
            }
        }
        draw.PopClipRect();
    }
}
}

#include "deathrace_skate2_draw.inl"
