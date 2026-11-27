#pragma once
//------------------------------------------------------------------------------
/**
    @class GraphicsFeature::MaterialDocument
    A material open in the asset editor: its textures and values, edited on the material the scene
    uses, so a change shows in the level as it is made, and on a knob in the asset preview.

    Every change is a command in the document's undo history. A value is shown while it is dragged
    and becomes a command when the drag ends. A texture is picked, dropped or pasted into its slot.

    Saving writes the work file, the template name with a value per parameter, textures by their URN.
    Only the template's parameters are written, along with what the work file had that the template
    does not know, which is kept as it was. The template is shown but not changed here: a loaded
    material cannot take another template, so that needs the material made again.

    @see
    Editor::AssetDocument
    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
#include "editorassets/assetdocument.h"
#include "dynui/imguicontext.h"
#include "graphics/graphicsentity.h"
#include "materials/material.h"
#include "resources/resourceid.h"
#include "util/array.h"
#include "util/dictionary.h"

namespace GraphicsFeature
{

class MaterialDocument final : public Editor::AssetDocument
{
    __DeclareClass(MaterialDocument)
public:
    MaterialDocument();
    virtual ~MaterialDocument();

    /// the template, the textures, the values and the preview
    void Draw() override;
    /// the knob is in the preview while the tab is in front
    void OnShown(bool shown) override;
    /// a material is written back
    bool CanSave() const override;

    /// open a material asset in the asset editor. The type registry calls this for 'mat'
    static void OpenAsset(const IO::Path& asset);
    /// write the work file of a new material with a template, its values the template's defaults
    static bool CreateAsset(const IO::Path& asset, const IO::URI& workFile, const Util::String& variant, Util::String& outError);
    /// the templates a new material can start from
    static void ListTemplates(Util::Array<Util::String>& out);

    /// write a value's bytes into the material. The commands change values through this
    void ApplyValue(uint offset, const void* data, uint size);
    /// bind a texture to a slot, an empty path binds the template's default. The commands change
    /// textures through this
    void ApplyTexture(IndexT slot, const IO::Path& texture);

protected:
    bool OnOpen() override;
    void OnClose() override;
    bool OnSave(Util::String& outError) override;

private:
    /// one texture of the template
    struct Slot
    {
        /// the texture the slot has, by URN, empty while it has the template's default
        IO::Path texture;
        /// what it had when the document was opened
        IO::Path original;
        /// the texture the document loaded for the slot, released when another replaces it
        Resources::ResourceId held = Resources::InvalidResourceId;
        /// the thumbnail
        Dynui::ImguiTextureId image;
        Ids::Id32 imageId = 0;
        bool hasImageId = false;
    };

    /// the material finished loading
    void Loaded(Resources::ResourceId id);
    /// read the work file's values, by name
    void ReadWorkFile();
    /// the textures and the values
    void DrawTextures(const MaterialTemplatesGPULang::Entry* materialTemplate);
    void DrawValues(const MaterialTemplatesGPULang::Entry* materialTemplate);
    /// make the texture change a field asked for into a command, once the frame has drawn
    void RunPendingTexture();

    Resources::ResourceId resource;
    Materials::MaterialId material;
    Graphics::GraphicsEntityId previewObject;
    Util::Array<Slot> slots;
    /// the values as the last command left them, what a drag is undone to
    Util::Array<ubyte> committed;
    /// the work file as it was read, its template and its values by parameter name
    Util::String workTemplate;
    Util::Dictionary<Util::String, Util::String> workValues;
    /// a texture a field picked, dropped or pasted, made into a command after the frame
    IndexT pendingSlot;
    IO::Path pendingTexture;
    bool shown;
};

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
