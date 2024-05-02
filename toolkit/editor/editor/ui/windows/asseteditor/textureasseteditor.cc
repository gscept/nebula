//------------------------------------------------------------------------------
//  @file textureasseteditor.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "textureasseteditor.h"
namespace Presentation
{

Dynui::ImguiTextureId Texture;

//------------------------------------------------------------------------------
/**
*/
void 
TextureEditor(AssetEditor* assetEditor, AssetEditorItem* item)
{
    ImGui::PushFont(Dynui::ImguiContext::state.boldFont);
    ImGui::Text(item->name.Value());
    ImGui::PopFont();
    Resources::SetMinLod(item->images[0]->res, 0.0f, true);
    CoreGraphics::TextureId tex = item->images[0]->res.resourceId;
    CoreGraphics::TextureIdLock _0(tex);
    CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(tex);
    CoreGraphics::TextureType type = CoreGraphics::TextureGetType(tex);
    ImGui::PushFont(Dynui::ImguiContext::state.boldFont);
    switch (type)
    {
        case CoreGraphics::Texture1D:
            ImGui::Text("Texture 1D");
            break;
        case CoreGraphics::Texture2D:
            ImGui::Text("Texture 2D");
            break;
        case CoreGraphics::Texture3D:
            ImGui::Text("Texture 3D");
            break;
        case CoreGraphics::TextureCube:
            ImGui::Text("Texture Cube");
            break;
    }
    ImGui::PopFont();

    ImVec2 remainder = ImGui::GetContentRegionAvail();
    float ratio = dims.height / float(dims.width);
    ImGui::Image(&item->images[0]->texture, ImVec2{ remainder.x, remainder.x * ratio }, ImVec2{ 0,0 }, ImVec2{ 1,1 }, ImVec4{ 1,1,1,1 }, ImVec4{ 0,0,0,1 });

    ImGui::Text("Width: %d", dims.width);
    ImGui::SameLine();
    ImGui::Text("Height: %d", dims.height);
    ImGui::SameLine();
    ImGui::Text("Depth: %d", dims.depth);

    ImGui::Text("Layers: %d", CoreGraphics::TextureGetNumLayers(tex));
    ImGui::SameLine();
    ImGui::Text("Mips: %d", CoreGraphics::TextureGetNumMips(tex));
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureSetup(AssetEditorItem* item)
{
    ImageHolder* data = item->allocator.Alloc<ImageHolder>();
    data->res = item->res;
    data->texture.layer = 0;
    data->texture.mip = 0;
    data->texture.nebulaHandle.resourceId = item->res.resourceId;
    item->images.Append(data);
}

} // namespace Presentation
