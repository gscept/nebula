#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Previewer

    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"
#include "editor/ui/modules/viewport.h"
#include "graphics/view.h"
#include "graphics/stage.h"
#include "coregraphics/meshresource.h"
#include "characters/skeletonresource.h"
#include "dynui/imguicontext.h"

namespace Presentation
{

class AssetEditor : public BaseWindow
{
    __DeclareClass(AssetEditor)
public:

    enum class AssetType
    {
        None,
        Material,
        Mesh,
        Skeleton,
        Model,
        Animation,
        Texture,

        NumAssetTypes
    };
    AssetEditor();
    ~AssetEditor();

    void Run(SaveMode save) override;

    // Select material for previewing
    void Open(const Resources::ResourceName& asset, const AssetType type);
};
__RegisterClass(AssetEditor)

struct ImageHolder
{
    Resources::ResourceId res;
    Dynui::ImguiTextureId texture;
};

struct AssetEditorItem
{
    AssetEditorItem()
        : assetType(AssetEditor::AssetType::None)
        , res(Resources::InvalidResourceId)
    {}

    AssetEditor::AssetType assetType;
    union U
    {
        U() { id = Resources::InvalidResourceUnknownId; }
        Materials::MaterialId material;
        CoreGraphics::MeshResourceId mesh;
        Characters::SkeletonResourceId skeleton;
        Models::ModelId model;
        CoreGraphics::TextureId texture;

        Resources::ResourceUnknownId id = Resources::InvalidResourceUnknownId;
    } asset;
    Resources::ResourceId res;
    Resources::ResourceName name;

    Memory::ArenaAllocator<2048> allocator;
    void* data; // use for editor specific data

    uint editCounter;
    bool grabFocus;
};

} // namespace Presentation

