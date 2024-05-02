//------------------------------------------------------------------------------
//  @file resourcebrowser.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "resourcebrowser.h"
#include "dynui/imguicontext.h"

#include "coregraphics/texture.h"
namespace Presentation
{
__ImplementClass(Presentation::ResourceBrowser, 'RsBw', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
ResourceBrowser::ResourceBrowser()
{
}

//------------------------------------------------------------------------------
/**
*/
ResourceBrowser::~ResourceBrowser()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceBrowser::Update()
{
    
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceBrowser::Run(SaveMode save)
{
    static Dynui::ImguiTextureId selectedTex;
    ImVec2 size = ImGui::GetContentRegionAvail();
    if (ImGui::BeginChild("List", ImVec2{ size.x / 2, size.y }, false, ImGuiWindowFlags_NoScrollbar))
    {
        static int current = -1;
        ImGui::PushFont(Dynui::ImguiContext::state.boldFont);
        ImGui::Text("Textures");
        ImGui::PopFont();
        if (ImGui::BeginTable("Textures###Table", 4, ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Dimensions");
            ImGui::TableSetupColumn("Format");
            ImGui::TableSetupColumn("Size");
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            int counter = 0;
            for (auto tex : CoreGraphics::TrackedTextures)
            {
                ImGui::TableNextColumn();
                CoreGraphics::TextureIdLock _0(tex);
                CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(tex);
                CoreGraphics::PixelFormat::Code format = CoreGraphics::TextureGetPixelFormat(tex);

                bool selected = false;
                if (ImGui::Selectable(CoreGraphics::TextureGetName(tex).Value(), counter == current, ImGuiSelectableFlags_SpanAllColumns))
                    current = counter;

                uint byteSize = dims.width * dims.height * dims.depth * CoreGraphics::PixelFormat::ToSize(format);
                ImGui::TableNextColumn();
                ImGui::Text(Util::Format("%dx%dx%d", dims.width, dims.height, dims.depth).AsCharPtr());
                ImGui::TableNextColumn();
                ImGui::Text(Util::Format("%s", CoreGraphics::PixelFormat::ToString(format).AsCharPtr()).AsCharPtr());
                ImGui::TableNextColumn();
                if (byteSize > 1_GB)
                    ImGui::Text(Util::Format("%.2f GB", byteSize / float(1_GB)).AsCharPtr());
                else if (byteSize > 1_MB)
                    ImGui::Text(Util::Format("%.2f MB", byteSize / float(1_MB)).AsCharPtr());
                else if (byteSize > 1_KB)
                    ImGui::Text(Util::Format("%.2f KB", byteSize / float(1_KB)).AsCharPtr());
                else
                    ImGui::Text(Util::Format("%.2f B", float(byteSize)).AsCharPtr());
                counter++;
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (current != -1)
        {
            CoreGraphics::TextureIdLock _0(CoreGraphics::TrackedTextures[current]);
            selectedTex.nebulaHandle = CoreGraphics::TrackedTextures[current];
            CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(CoreGraphics::TrackedTextures[current]);
            float ratio = dims.width / dims.height;
            ImVec2 remainder = ImGui::GetContentRegionAvail();
            static int mip = 0, layer = 0;
            if (ImGui::BeginChild("Preview", ImVec2{ size.x / 2, size.y }))
            {
                ImGui::Image(&selectedTex, ImVec2{ (float)remainder.x / 2, (float)remainder.x / 2 * ratio });
                ImGui::InputInt("Mip", &mip);
                ImGui::InputInt("Layer", &layer);
                mip = Math::min(Math::max(0, mip), CoreGraphics::TextureGetNumMips(CoreGraphics::TrackedTextures[current]) - 1);
                layer = Math::min(Math::max(0, layer), CoreGraphics::TextureGetNumLayers(CoreGraphics::TrackedTextures[current]) - 1);
                ImGui::EndChild();
            }

            selectedTex.layer = layer;
            selectedTex.mip = mip;
        }
        else
        {
            ImGuiStyle& style = ImGui::GetStyle();

            static const char* EmptyString = "Nothing selected";
            float sizeX = ImGui::CalcTextSize(EmptyString).x + style.FramePadding.x * 2.0f;
            float availX = ImGui::GetContentRegionAvail().x;
            float sizeY = ImGui::CalcTextSize(EmptyString).y + style.FramePadding.y * 2.0f;
            float availY = ImGui::GetContentRegionAvail().y;

            float off = (availX - sizeX) * 0.5f;
            if (off > 0.0f)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
            off = (availY - sizeY) * 0.5f;
            if (off > 0.0f)
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + off);
            ImGui::Text(EmptyString);
        }
    }
}

} // namespace Presentation
