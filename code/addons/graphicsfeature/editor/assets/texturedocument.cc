//------------------------------------------------------------------------------
//  @file texturedocument.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "graphicsfeature/editor/assets/texturedocument.h"

#if WITH_NEBULA_EDITOR
#include "editorassets/asseteditorpanel.h"
#include "coregraphics/texture.h"
#include "resources/resourceserver.h"
#include "imgui.h"

namespace GraphicsFeature
{

__ImplementClass(GraphicsFeature::TextureDocument, 'GTXD', Editor::AssetDocument);

//------------------------------------------------------------------------------
/**
*/
TextureDocument::TextureDocument() :
    resource(Resources::InvalidResourceId),
    textureId(0),
    hasTextureId(false),
    exposureStops(0.0f),
    layer(0),
    mip(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
TextureDocument::~TextureDocument()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
TextureDocument::OpenAsset(const IO::Path& asset)
{
    Ptr<TextureDocument> document = TextureDocument::Create();
    document->asset = asset;
    Editor::AssetEditorPanel::Open(document.upcast<Editor::AssetDocument>());
}

//------------------------------------------------------------------------------
/**
    The texture loads in the background. The document holds itself in the callbacks, so a tab
    closed before the load finished still lets go of the texture.
*/
bool
TextureDocument::OnOpen()
{
    if (!Resources::ResourceServer::Instance()->HasStreamLoader(this->asset))
        return false;

    Ptr<TextureDocument> self = this;
    this->resource = Resources::CreateResource(this->asset, "editor",
        [self](Resources::ResourceId id)
        {
            self->Loaded(id);
        },
        [self](Resources::ResourceId id)
        {
            self->SetFailed("the texture could not be loaded");
        });
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
TextureDocument::Loaded(Resources::ResourceId id)
{
    if (this->IsClosed())
        return;

    this->resource = id;
    this->texture.layer = 0;
    this->texture.mip = 0;
    this->texture.nebulaHandle.resourceId = id.resourceId;
    this->textureId = Dynui::AllocateImguiTextureId(this->texture);
    this->hasTextureId = true;
    this->SetReady();
}

//------------------------------------------------------------------------------
/**
*/
void
TextureDocument::OnClose()
{
    if (this->hasTextureId)
    {
        Dynui::DeallocateImguiTextureId(this->textureId);
        this->hasTextureId = false;
    }
    if (this->resource != Resources::InvalidResourceId)
    {
        Resources::DiscardResource(this->resource);
        this->resource = Resources::InvalidResourceId;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TextureDocument::Draw()
{
    // the editor streams textures in, the document shows every mip
    Resources::SetMinLod(this->resource, 0.0f, true);
    CoreGraphics::TextureId tex = this->resource.resourceId;
    CoreGraphics::TextureIdLock _0(tex);
    CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(tex);
    CoreGraphics::TextureType type = CoreGraphics::TextureGetType(tex);

    const char* typeName = "Unknown texture type";
    switch (type)
    {
        case CoreGraphics::Texture1D:           typeName = "Texture 1D"; break;
        case CoreGraphics::Texture2D:           typeName = "Texture 2D"; break;
        case CoreGraphics::Texture3D:           typeName = "Texture 3D"; break;
        case CoreGraphics::TextureCube:         typeName = "Texture Cube"; break;
        case CoreGraphics::Texture1DArray:      typeName = "Texture 1D Array"; break;
        case CoreGraphics::Texture2DArray:      typeName = "Texture 2D Array"; break;
        case CoreGraphics::TextureCubeArray:    typeName = "Texture Cube Array"; break;
        default: break;
    }
    ImGui::PushFont(Dynui::ImguiBoldFont, 0.0f);
    ImGui::TextUnformatted(typeName);
    ImGui::PopFont();

    // HDR textures are tonemapped, they would show up as white otherwise
    const bool hdr = CoreGraphics::PixelFormat::IsHighDynamicRange(CoreGraphics::TextureGetPixelFormat(tex));
    if (hdr)
    {
        ImGui::SliderFloat("Exposure", &this->exposureStops, -10.0f, 10.0f, "%.1f EV");
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
            ImGui::RadioButton(faceNames[i], &this->layer, i);
            ImGui::SameLine();
        }
        ImGui::TextUnformatted("Face");
    }
    else if (type == CoreGraphics::Texture3D && dims.depth > 1)
    {
        this->layer = Math::min(this->layer, Math::min(dims.depth, 256) - 1);
        ImGui::SliderInt("Slice", &this->layer, 0, Math::min(dims.depth, 256) - 1);
    }
    else if (layers > 1)
    {
        this->layer = Math::min(this->layer, Math::min(layers, 256) - 1);
        ImGui::SliderInt("Layer", &this->layer, 0, Math::min(layers, 256) - 1);
    }
    else
    {
        this->layer = 0;
    }
    this->mip = Math::min(this->mip, mips - 1);
    if (mips > 1)
    {
        Util::String mipFormat = Util::Format("%%d (%dx%d)", Math::max(dims.width >> this->mip, 1), Math::max(dims.height >> this->mip, 1));
        ImGui::SliderInt("Mip", &this->mip, 0, mips - 1, mipFormat.AsCharPtr(), ImGuiSliderFlags_AlwaysClamp);
    }

    ImGui::Text("Width: %d", dims.width);
    ImGui::SameLine();
    ImGui::Text("Height: %d", dims.height);
    ImGui::SameLine();
    ImGui::Text("Depth: %d", dims.depth);
    ImGui::SameLine();
    ImGui::Text("Layers: %d", layers);
    ImGui::SameLine();
    ImGui::Text("Mips: %d", CoreGraphics::TextureGetNumMips(tex));

    this->texture.nebulaHandle = tex;
    this->texture.layer = this->layer;
    this->texture.mip = this->mip;
    this->texture.tonemap = 1;
    this->texture.exposure = Math::exp2(this->exposureStops);
    Dynui::SetImguiTextureIdData(this->textureId, this->texture);

    // as large as it fits, keeping its aspect
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float width = Math::max((float)dims.width, 1.0f);
    const float height = Math::max((float)dims.height, 1.0f);
    const float scale = Math::max(Math::min(available.x / width, available.y / height), 0.0f);
    ImTextureRef ref { this->textureId };
    ImGui::Image(ref, ImVec2{ width * scale, height * scale }, ImVec2{ 0, 0 }, ImVec2{ 1, 1 });
}

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
