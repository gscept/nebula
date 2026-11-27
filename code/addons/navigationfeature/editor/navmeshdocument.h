#pragma once
//------------------------------------------------------------------------------
/**
    @class NavigationFeature::NavMeshDocument
    A navmesh open in the asset editor: the agent it is baked for, the build settings, and the
    geometry it is baked from, which is gathered from entities of the scene.

    The work file is a description, the bake turns it into the Detour tiles. Baking saves the
    description and has the batcher bake it; the area volumes of the scene are gathered into it on
    every bake, since they are not chosen. The baked navmesh is loaded while the document is open,
    so it shows in the viewport and paths can be tested on it.

    Every change is a command holding the description as it was and as it is, so undo puts it back
    whole. The description is small: settings and a list of sources.

    @see
    Editor::AssetDocument
    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
#include "editorassets/assetdocument.h"
#include "resources/resourceid.h"
#include "navigationfeature/editor/navmeshsources.h"

namespace NavigationFeature
{

class NavMeshDocument final : public Editor::AssetDocument
{
    __DeclareClass(NavMeshDocument)
public:
    NavMeshDocument();
    virtual ~NavMeshDocument();

    /// the settings, the sources and the bake
    void Draw() override;
    /// a navmesh is written back
    bool CanSave() const override;

    /// open a navmesh asset in the asset editor. The type registry calls this for 'nav'
    static void OpenAsset(const IO::Path& asset);
    /// write the work file of a new navmesh. The type registry calls this for 'nav'
    static bool CreateAsset(const IO::Path& asset, const IO::URI& workFile, const Util::String& variant, Util::String& outError);
    /// the agent kind of an open navmesh, empty when none is open. The agent kind field offers it
    static Util::String OpenAgentKind();
    /// the baked navmesh the document shows, invalid while there is none
    Resources::ResourceId GetPreview() const;

    /// put the description a command holds
    void ApplyDescription(const Util::Blob& description);
    /// gather the sources of entities. Entries of the same entities which are already there are
    /// replaced, unless replaceAll drops every source first. Public for the console command
    void GatherEntities(const Util::Array<Game::Entity>& entities, bool replaceAll);
    /// save and bake. Public for the console command
    bool Bake();

protected:
    bool OnOpen() override;
    void OnClose() override;
    bool OnSave(Util::String& outError) override;
    void OnRevert() override;

private:
    /// an entity which gave no sources, and why
    struct SkippedEntity
    {
        Util::String name;
        Util::String reason;
    };

    /// read the work file, false when it isn't a navmesh description
    bool LoadDescription(Util::String& outError);
    /// a description with the default agent and settings
    void ResetDescription();
    /// make the changes since the last command into one
    void Commit(const char* name);
    /// gather the sources of the same entities again, after they moved or changed
    void GatherAgain();
    /// replace the area volumes of the description with the ones of the scene
    void GatherAreaVolumes();
    /// the entity that loads the navmesh in the scene, invalid if there is none
    Game::Entity FindNavMeshEntity() const;
    /// select that entity, or make one
    void PlaceInScene();
    /// once the batcher is done, load the bake
    void UpdatePreview();
    void DiscardPreview();
    void SetStatus(bool error, const Util::String& status);

    void DrawSettings();
    void DrawSources();
    void DrawBake();

    Navigation::NavMeshT description;
    /// the description as the last command left it
    Util::Blob committed;

    NavMeshGatherOptions options;
    Util::Array<SkippedEntity> skipped;
    Util::String status;
    bool statusIsError;
    /// set while the batcher bakes the navmesh
    bool baking;
    Resources::ResourceId preview;
};

} // namespace NavigationFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
