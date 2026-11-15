//------------------------------------------------------------------------------
//  @file textureasseteditor.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "textureasseteditor.h"
namespace Presentation
{

Dynui::ImguiTextureId Texture;

struct TextureEditorItemData
{
    ImageHolder* image;
    float exposureStops = 0.0f;
    int layer = 0;      // cube face, array layer or 3D slice to show
    int mip = 0;
};


//------------------------------------------------------------------------------
/**
*/
void 
TextureEditor(AssetEditor* assetEditor, AssetEditorItem* item)
{
    ImGui::PushFont(Dynui::ImguiBoldFont, 0.0f);
    ImGui::Text(item->path.GetFolderAndFile().AsCharPtr());
    ImGui::PopFont();
    auto itemData = (TextureEditorItemData*)item->data;

    Resources::SetMinLod(itemData->image->res, 0.0f, true);
    CoreGraphics::TextureId tex = itemData->image->res.resourceId;
    CoreGraphics::TextureIdLock _0(tex);
    CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(tex);
    CoreGraphics::TextureType type = CoreGraphics::TextureGetType(tex);
    ImGui::PushFont(Dynui::ImguiBoldFont, 0.0f);
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
        case CoreGraphics::Texture1DArray:
            ImGui::Text("Texture 1D Array");
            break;
        case CoreGraphics::Texture2DArray:
            ImGui::Text("Texture 2D Array");
            break;
        case CoreGraphics::TextureCubeArray:
            ImGui::Text("Texture Cube Array");
            break;
        default:
            n_error("unhandled TextureType");
            break;
    }
    ImGui::PopFont();

    ImVec2 remainder = ImGui::GetContentRegionAvail();
    float ratio = dims.height / float(dims.width);

    // HDR textures are tonemapped, they would show up as white otherwise
    const bool hdr = CoreGraphics::PixelFormat::IsHighDynamicRange(CoreGraphics::TextureGetPixelFormat(tex));
    if (hdr)
    {
        ImGui::SliderFloat("Exposure", &itemData->exposureStops, -10.0f, 10.0f, "%.1f EV");
        ImGui::SetItemTooltip("Preview only, HDR textures are shown tonemapped. Ctrl+click to type a value.");
    }

    // the imgui texture id holds 8 bits for the layer and 4 for the mip
    const int layers = CoreGraphics::TextureGetNumLayers(tex);
    const int mips = Math::min(CoreGraphics::TextureGetNumMips(tex), 16);
    if (type == CoreGraphics::TextureCube)
    {
        static const char* faceNames[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
        for (int i = 0; i < 6; i++)
        {
            ImGui::RadioButton(faceNames[i], &itemData->layer, i);
            ImGui::SameLine();
        }
        ImGui::TextUnformatted("Face");
    }
    else if (type == CoreGraphics::Texture3D && dims.depth > 1)
    {
        itemData->layer = Math::min(itemData->layer, Math::min(dims.depth, 256) - 1);
        ImGui::SliderInt("Slice", &itemData->layer, 0, Math::min(dims.depth, 256) - 1);
    }
    else if (layers > 1)
    {
        itemData->layer = Math::min(itemData->layer, Math::min(layers, 256) - 1);
        ImGui::SliderInt("Layer", &itemData->layer, 0, Math::min(layers, 256) - 1);
    }
    else
    {
        itemData->layer = 0;
    }
    itemData->mip = Math::min(itemData->mip, mips - 1);
    if (mips > 1)
    {
        Util::String mipFormat = Util::Format("%%d (%dx%d)", Math::max(dims.width >> itemData->mip, 1), Math::max(dims.height >> itemData->mip, 1));
        ImGui::SliderInt("Mip", &itemData->mip, 0, mips - 1, mipFormat.AsCharPtr(), ImGuiSliderFlags_AlwaysClamp);
    }

    itemData->image->texture.nebulaHandle = tex;
    itemData->image->texture.layer = itemData->layer;
    itemData->image->texture.mip = itemData->mip;
    itemData->image->texture.tonemap = 1;
    itemData->image->texture.exposure = Math::exp2(itemData->exposureStops);
    Dynui::SetImguiTextureIdData(itemData->image->textureId, itemData->image->texture);
    ImTextureRef ref {itemData->image->textureId};
    ImGui::Image(ref, ImVec2{ remainder.x, remainder.x * ratio }, ImVec2{ 0,0 }, ImVec2{ 1,1 });

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
    auto itemData = item->allocator.Alloc<TextureEditorItemData>();
    itemData->image = item->allocator.Alloc<ImageHolder>();
    itemData->image->res = item->res;
    itemData->image->texture.layer = 0;
    itemData->image->texture.mip = 0;
    itemData->image->texture.nebulaHandle.resourceId = item->res.resourceId;
    // the arena doesn't initialize the id, it has to be allocated before it can be set
    itemData->image->textureId = Dynui::AllocateImguiTextureId(itemData->image->texture);
    item->data = itemData;
}

} // namespace Presentation
