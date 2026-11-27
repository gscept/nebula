#pragma once
//------------------------------------------------------------------------------
/**
    @class GraphicsFeature::TerrainPanel
    The terrain editor: sculpting the heightmap, painting where each biome and each plant is, and
    the settings of the terrain, its biomes and its plants.

    It edits the Terrain of the selected entity. Each terrain that was edited keeps its session
    while the scene is open, so selecting something else doesn't lose the work, and the exit asks
    about the terrains that aren't saved. A stroke is one undo step of the scene.

    The brush works in the viewport while Edit is on: the left button paints, shift does the
    opposite (lower instead of raise, erase instead of paint), escape stops. The terrain is drawn by
    the game's frame script, so it shows in the viewport's Rendering view; the brush and picking work
    in both.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
#include "neweditor/core/editorpanel.h"
#include "graphicsfeature/editor/terrain/terrainsession.h"

namespace Editor
{
struct ToolView;
}

namespace GraphicsFeature
{

class TerrainPanel final : public Editor::EditorPanel
{
    __DeclareClass(TerrainPanel)
public:
    TerrainPanel();
    virtual ~TerrainPanel();

    void Draw() override;
    bool OnEditAction(Editor::EditAction action) override;

    /// register the panel, the viewport brush, the per frame update and the unsaved work
    static void Register();
    /// let the sessions go
    static void Clear();

protected:
    bool OnSetup() override;

private:
    /// the session of the selected entity, made when it has a Terrain and has none yet. Null otherwise
    TerrainSession* SelectedSession(bool create);
    /// the parts of the panel
    void DrawTools(TerrainSession* session);
    void DrawSettings(TerrainSession* session);
    void DrawBiomes(TerrainSession* session);
    void DrawPlants(TerrainSession* session);
    /// save the session, with the file the panel has for one that names none
    bool SaveSession(TerrainSession* session);

    /// the brush in the viewport
    static bool ViewportInput(const Editor::ToolView& view);
    /// every frame, whether the panel is shown or not
    static void UpdateSessions();
    static void ListUnsaved(Util::Array<Util::String>& out);
    static bool SaveUnsaved();

    /// terrain [status|edit on|off|tool <name>|brush <radius> [hardness] [strength]|stroke <x> <z> [frames]|height <x> <z>|biome add|plant add|biome <n>|plant <n>|save [file]]
    void CommandTerrain(const Util::Array<Util::String>& args);

    static TerrainPanel* instance;

    TerrainBrush brush;
    bool editing;
    /// what Paint and Erase work on
    TerrainSession::Map paintMap;
    IndexT paintIndex;
    IndexT selectedBiome;
    IndexT selectedPlant;
    char saveFile[256];

    /// the stroke the viewport holds
    Ptr<TerrainSession> strokeSession;
    Math::vec3 hover;
    bool hoverValid;
};

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
