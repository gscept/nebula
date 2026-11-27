#pragma once
//------------------------------------------------------------------------------
/**
    @class GraphicsFeature::ModelDocument
    A model open in the asset editor, shown in the asset preview: the material of each node, the
    meshes, and for a skinned model its skeleton, joint masks and animation.

    The animation is played on the preview's character: clips with a transport, a second clip
    layered over the first on the joints of a mask, and the clip sections and events the asset adds
    when it is packaged, shown and dragged on the time slider.

    What the document edits is small next to the asset, which holds the meshes: the node materials,
    the clip sections, the clip events and the joint masks. Every edit that is done becomes a command
    holding those parts as they were and as they are, so undo puts them back whole. A material
    change shows on the model in the scene as well, the way the old editor showed it.

    Saving writes the whole asset, the meshes as they were read, and batches it, which packages the
    sections, events and masks into the model's animation and skeleton.

    @see
    Editor::AssetDocument
    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
#include "editorassets/assetdocument.h"

namespace GraphicsFeature
{

struct ModelEdits;
struct ModelDocumentState;

class ModelDocument final : public Editor::AssetDocument
{
    __DeclareClass(ModelDocument)
public:
    ModelDocument();
    virtual ~ModelDocument();

    /// the preview with the skeleton over it, and the sections
    void Draw() override;
    /// the model is in the preview while the tab is in front
    void OnShown(bool shown) override;
    /// a model is written back
    bool CanSave() const override;

    /// open a model asset in the asset editor. The type registry calls this for 'mdl'
    static void OpenAsset(const IO::Path& asset);

    /// put the edited parts of the asset as a snapshot has them. The commands change the asset
    /// through this
    void ApplyEdits(const ModelEdits& edits);

protected:
    bool OnOpen() override;
    void OnClose() override;
    bool OnSave(Util::String& outError) override;

private:
    /// the model resource finished loading
    void Loaded();
    /// the preview's model finished loading, it gets its character
    void PreviewLoaded();
    /// make the edits done since the last command into one, after the frame. The sections are drawn
    /// over the asset's lists, so a command made while they draw would replace what they walk
    void Commit(const char* name);
    /// make the command now
    void CommitNow(const char* name);
    /// show a node's material on the model and its instances
    void ApplyNodeMaterial(const Util::String& node, const Util::String& material);
    /// make a material a field picked into a command, once the frame has drawn
    void RunPendingMaterial();

    /// the sections of the right column
    void DrawMaterials();
    void DrawMesh();
    void DrawJointMasks();
    void DrawAnimationLayer();
    void DrawClipSections();
    void DrawClipEvents();

    ModelDocumentState* state;
};

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
