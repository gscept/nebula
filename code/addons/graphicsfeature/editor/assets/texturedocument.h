#pragma once
//------------------------------------------------------------------------------
/**
    @class GraphicsFeature::TextureDocument
    A texture open in the asset editor. It shows the texture as it was exported, one mip and one
    layer, cube face or slice at a time, with HDR textures tonemapped at an exposure the slider sets.
    A texture is changed by importing it again, so the document only shows it.
    @see
    Editor::AssetDocument
    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
#include "editorassets/assetdocument.h"
#include "dynui/imguicontext.h"
#include "ids/id.h"
#include "resources/resourceid.h"

namespace GraphicsFeature
{

class TextureDocument final : public Editor::AssetDocument
{
    __DeclareClass(TextureDocument)
public:
    TextureDocument();
    virtual ~TextureDocument();

    /// the texture, its view settings and what it is
    void Draw() override;

    /// open a texture asset in the asset editor. The type registry calls this for 'tex'
    static void OpenAsset(const IO::Path& asset);

protected:
    bool OnOpen() override;
    void OnClose() override;

private:
    /// the texture finished loading
    void Loaded(Resources::ResourceId id);

    Resources::ResourceId resource;
    /// what the ImGui shader is told about the texture: the mip, the layer and the exposure
    Dynui::ImguiTextureId texture;
    Ids::Id32 textureId;
    bool hasTextureId;
    float exposureStops;
    /// cube face, array layer or 3D slice to show
    int layer;
    int mip;
};

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
