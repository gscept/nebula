#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Previewer

    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"
#include "editor/ui/modules/viewport.h"
#include "graphics/view.h"
#include "coregraphics/meshresource.h"
#include "characters/skeletonresource.h"
#include "particles/particleresource.h"
#include "dynui/imguicontext.h"
#include "io/path.h"

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/model.h"
#include "flat/material.h"
#include "flat/particle.h"


namespace Presentation
{

struct AssetEditorItem;
class AssetEditor : public BaseWindow
{
public:

    enum class AssetType
    {
        None,
        Material,
        Model,
        Texture,
        Particle,

        NumAssetTypes
    };
    AssetEditor();
    ~AssetEditor();

    void Run(SaveMode save) override;
    void Update() override;

    Modules::Viewport viewport;

    // Select material for previewing
    void Open(const IO::Path& asset, const Util::String root, const AssetType type);
    /// Get the item an asset is open in, nullptr if it isn't open. Items move when others are opened or closed, so they are looked up again instead of kept
    static AssetEditorItem* GetOpenItem(const IO::Path& asset);
};

struct ImageHolder
{
    Resources::ResourceId res;
    Ids::Id32 textureId;
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
        Particles::ParticleResourceId particle;
        Characters::SkeletonResourceId skeleton;
        Models::ModelId model;
        CoreGraphics::TextureId texture;

        Resources::ResourceUnknownId id = Resources::InvalidResourceUnknownId;
    } asset;

    Resources::ResourceId res;
    IO::Path path;
    Graphics::GraphicsEntityId previewObject;


    Memory::ArenaAllocator<2048> allocator;
    void* data; // use for editor specific data

    uint editCounter;
    bool grabFocus;
    /// the tab was closed with unsaved changes, ask what to do with them once it's selected
    bool closeRequested = false;
};

/// Get the URN (folder:file:type) of an exported asset from the name it's loaded by, an export path like tex:folder/file.dds or a URN.
/// Returns false for assets that aren't exported from the work folder
bool AssetURN(const Util::String& resourceName, const char* type, Util::String& outURN);

/// Draw the field of an asset an asset refers to, like the resource fields of components: its name, which a double click opens
/// in the asset editor, and a button picking another one in the asset browser. Assets of the type (tex, mat, ...), or of any type
/// for a type of nullptr, can be dropped on the name, right click copies its URN, middle click pastes one. An asset that isn't a URN
/// is shown by its label only.
/// onChange gets the asset picked, dropped or pasted. The picker calls it later, after the item drawing the field may have moved
/// or closed, so it looks the item up again
void AssetField(const char* id, const Util::String& label, const IO::Path& asset, const char* type, const std::function<void(const IO::Path&)>& onChange);

} // namespace Presentation

