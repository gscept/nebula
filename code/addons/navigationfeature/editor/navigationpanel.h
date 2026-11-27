#pragma once
//------------------------------------------------------------------------------
/**
    @class NavigationFeature::NavigationPanel
    The navigation panel: what the viewport shows of navigation, the project's area table, and path
    tests on the loaded navmeshes. A navmesh itself is edited in the asset editor.

    The area table is written to the project's table and compiled when it is saved. Volumes and
    navmeshes refer to areas by name, one that was renamed or removed falls back to default.

    The 'nav' command does what the panel does from a script.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
#include "neweditor/core/editorpanel.h"
#include "math/vec4.h"
#include "util/array.h"
#include "util/string.h"

namespace NavigationFeature
{

class NavigationPanel final : public Editor::EditorPanel
{
    __DeclareClass(NavigationPanel)
public:
    NavigationPanel();
    virtual ~NavigationPanel();

    void Draw() override;

protected:
    bool OnSetup() override;

private:
    /// a project area being edited, written to the table when saved
    struct EditableArea
    {
        unsigned char id = 0;
        char name[64] = "";
        float cost = 1.0f;
        unsigned short flags = 1;
        Math::vec4 color = Math::vec4(1.0f);
    };

    void DrawDisplay();
    void DrawAreas();
    void DrawPathTest();

    /// the project areas of the table, to edit
    void LoadEditAreas();
    /// what's wrong with the edited areas, empty if they can be saved
    Util::String ValidateEditAreas() const;
    /// write the edited areas to the project table, compile and load it
    bool SaveEditAreas();
    /// compile the project table and load it
    void ReloadTable();

    /// nav [areavolume <area>|bake <urn>|gather <urn>|path <x,y,z> <x,y,z>|areas]
    void CommandNav(const Util::Array<Util::String>& args);

    Util::Array<EditableArea> editAreas;
    bool editAreasLoaded;
    bool editAreasChanged;
    Util::String tableMessage;
};

} // namespace NavigationFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
