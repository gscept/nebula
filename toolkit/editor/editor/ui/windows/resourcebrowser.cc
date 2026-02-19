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
    this->additionalFlags = ImGuiWindowFlags_(ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
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
ResourceBrowser::Run(SaveMode save)
{
    static Dynui::ImguiTextureId selectedTex;
    ImVec2 size = ImGui::GetContentRegionAvail();

    if (ImGui::BeginTable("Resource Browser Contents", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupScrollFreeze(2, 1);
        ImGui::TableNextColumn();
        static int current = -1;
        if (ImGui::BeginChild("List", ImVec2{ 0, 0 }, false, ImGuiWindowFlags_NoScrollbar))
        {
            ImGui::PushFont(Dynui::ImguiBoldFont);
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
        }
        ImGui::TableNextColumn();

        if (current != -1)
        {
            CoreGraphics::TextureIdLock _0(CoreGraphics::TrackedTextures[current]);
            selectedTex.nebulaHandle = CoreGraphics::TrackedTextures[current];
            CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(CoreGraphics::TrackedTextures[current]);
            float ratio = dims.height / float(dims.width);
            ImVec2 remainder = ImGui::GetContentRegionAvail();
            static int mip = 0, layer = 0;
            static bool alpha = false;
            static bool range = false;
            static bool red = true, green = true, blue = true, a = true;
            static float minRange = 0, maxRange = 1;
            if (ImGui::BeginChild("Preview", ImVec2{ 0, 0 }))
            {
                ImGui::Image(&selectedTex, ImVec2{ (float)remainder.x, (float)remainder.x * ratio });
                ImGui::InputInt("Mip", &mip);
                ImGui::InputInt("Layer", &layer);
                
                ImGui::Checkbox("R", &red);
                ImGui::SameLine();
                ImGui::Checkbox("G", &green);
                ImGui::SameLine();
                ImGui::Checkbox("B", &blue);
                ImGui::SameLine();
                ImGui::Checkbox("A", &a);
                
                ImGui::Checkbox("Alpha", &alpha);
                ImGui::Checkbox("Range", &range);
                if (range)
                {
                    static float rangeMax = 200.0f;
                    ImGui::InputFloat("Range Max", &rangeMax);
                    ImGui::SliderFloat("Min", &minRange, 0.001f, rangeMax);
                    ImGui::SliderFloat("Max", &maxRange, 0.001f, rangeMax);
                    float tempMin = minRange;
                    minRange = Math::min(minRange, maxRange);
                    maxRange = Math::max(tempMin, maxRange);
                }
                mip = Math::min(Math::max(0, mip), CoreGraphics::TextureGetNumMips(CoreGraphics::TrackedTextures[current]) - 1);
                layer = Math::min(Math::max(0, layer), CoreGraphics::TextureGetNumLayers(CoreGraphics::TrackedTextures[current]) - 1);
            }
            ImGui::EndChild();

            selectedTex.layer = layer;
            selectedTex.mip = mip;
            selectedTex.useAlpha = alpha;
            selectedTex.useRange = range;
            selectedTex.rangeMin = minRange;
            selectedTex.rangeMax = maxRange;
            selectedTex.red = red;
            selectedTex.green = green;
            selectedTex.blue = blue;
            selectedTex.alpha = a;
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
        ImGui::EndTable();
    }
}

} // namespace Presentation
