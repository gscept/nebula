//------------------------------------------------------------------------------
//  framescriptinspector.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framescriptinspector.h"
#include "frame/framescript.h"
#include "dynui/imguicontext.h"
#include "imgui.h"
namespace Debug
{

struct
{
    int selectedTarget = 0;
    int selectedMip = 0;
    int selectedLayer = 0;
} state;
//------------------------------------------------------------------------------
/**
*/
void 
FrameScriptInspector::Run(const Ptr<Frame::FrameScript>& script)
{
    ImGui::Begin(script->GetResourceName().Value());

    auto textures = script->GetTextures();
    ImGui::InputInt("mip", &state.selectedMip);
    ImGui::InputInt("layer", &state.selectedLayer);

    ImGui::Text("Textures");
    for (int i = 0; i < textures.Size(); i++)
    {
        if (ImGui::Selectable(textures.KeyAtIndex(i).Value(), i == state.selectedTarget))
        {
            state.selectedTarget = i;
            break;
        }
    }

    CoreGraphics::TextureId textureId = textures.ValueAtIndex(state.selectedTarget);
    CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(textureId);

    using namespace CoreGraphics;

    // Needs to not be nuked scope since we're sending a void*
    static CoreGraphics::TextureId id;
    id = textureId;

    ImVec2 imageSize = {(float)dims.width, (float)dims.height};

    static Dynui::ImguiTextureId textureInfo;
    textureInfo.nebulaHandle = id.HashCode64();
    textureInfo.mip = state.selectedMip;
    textureInfo.layer = state.selectedLayer;

    ImGui::NewLine();
    ImGui::Separator();

    static bool fitToWindow = true;
    ImGui::Checkbox("Fit to window", &fitToWindow);
    if (fitToWindow)
    {
        imageSize.x = ImGui::GetWindowContentRegionWidth();
        float ratio = (float)dims.height / (float)dims.width;
        imageSize.y = imageSize.x * ratio;
    }

    ImGui::Image((void*)& textureInfo, imageSize);
    ImGui::End();

}
} // namespace Debug
