//------------------------------------------------------------------------------
//  @file navigationpanel.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "navigationfeature/editor/navigationpanel.h"

#if WITH_NEBULA_EDITOR
#include "navigationfeature/editor/navigationeditor.h"
#include "navigationfeature/editor/navmeshdocument.h"
#include "navigationfeature/navigationfeatureunit.h"
#include "navigationfeature/streamnavmeshcache.h"
#include "navbuild/navareas.h"
#include "navbuild/navmeshbuilder.h"
#include "editorassets/asseteditorpanel.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/iconregistry.h"
#include "neweditor/console/editorcommands.h"
#include "neweditor/model/selectioncontext.h"
#include "io/ioserver.h"
#include "imgui.h"

namespace NavigationFeature
{

__ImplementClass(NavigationFeature::NavigationPanel, 'NAVP', Editor::EditorPanel);

//------------------------------------------------------------------------------
/**
*/
static const char*
Glyph(const char* name, const char* fallback)
{
    const char* glyph = Editor::IconGlyph(name);
    return glyph != nullptr ? glyph : fallback;
}

//------------------------------------------------------------------------------
/**
    A readable name for a loaded navmesh, its file and the agent kind it is for
*/
static Util::String
NavMeshLabel(Navigation::NavMeshId id)
{
    Navigation::StreamNavMeshCache* cache = GetNavMeshCache();
    // the cache knows the file it loaded, not the URN
    Util::String label = Util::String(cache->GetName(id).Value()).ExtractFileName();
    label.StripFileExtension();
    label.Append(" (");
    label.Append(cache->GetAgent(id).name.c_str());
    label.Append(")");
    return label;
}

//------------------------------------------------------------------------------
/**
*/
static Util::Array<Navigation::NavMeshId>
LoadedNavMeshes()
{
    Util::Array<Navigation::NavMeshId> loaded;
    Navigation::StreamNavMeshCache* cache = GetNavMeshCache();
    if (cache == nullptr)
        return loaded;
    for (Navigation::NavMeshId id : cache->GetLoadedMeshes())
    {
        if (id != Navigation::InvalidNavMeshId && cache->GetDetourMesh(id) != nullptr)
            loaded.Append(id);
    }
    return loaded;
}

//------------------------------------------------------------------------------
/**
    The panel starts closed, the navmesh is edited in the asset editor
*/
NavigationPanel::NavigationPanel() :
    editAreasLoaded(false),
    editAreasChanged(false)
{
    this->name = "navigation";
    this->open = false;
}

//------------------------------------------------------------------------------
/**
*/
NavigationPanel::~NavigationPanel()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
NavigationPanel::OnSetup()
{
    this->core->GetCommands().Register("nav", "nav [areas|areavolume <area>|gather <urn>|bake <urn>|path <x,y,z> <x,y,z>|stop] - navigation: the area table, a new area volume, gathering the selection into an open navmesh and baking it, and a path test on the loaded navmesh",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<NavigationPanel, &NavigationPanel::CommandNav>(this));
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
NavigationPanel::Draw()
{
    this->DrawDisplay();
    this->DrawAreas();
    this->DrawPathTest();
}

//------------------------------------------------------------------------------
/**
*/
void
NavigationPanel::DrawDisplay()
{
    NavigationDisplay& display = GetNavigationDisplay();
    ImGui::Checkbox("Show navmeshes and agents", &display.navMeshes);
    ImGui::SameLine();
    ImGui::Checkbox("Show area volumes", &display.areaVolumes);
    ImGui::SetItemTooltip("The selected volumes are always shown");
}

//------------------------------------------------------------------------------
/**
*/
void
NavigationPanel::ReloadTable()
{
    if (Navigation::Build::CompileAreaTable())
        this->tableMessage.Format("Loaded %s", Navigation::Build::AreaTableSource);
    else if (IO::FileExists(Navigation::Build::AreaTableSource))
        this->tableMessage.Format("%s doesn't compile, see the console", Navigation::Build::AreaTableSource);
    else
        this->tableMessage.Format("No %s, only the built in areas exist", Navigation::Build::AreaTableSource);
    ReloadAreaTable();
}

//------------------------------------------------------------------------------
/**
*/
void
NavigationPanel::LoadEditAreas()
{
    this->editAreas.Clear();
    for (const Navigation::Build::AreaDefinition& area : GetAreaTable().Areas())
    {
        if (area.id == Navigation::Build::NoArea || area.id == Navigation::Build::DefaultWalkableArea)
            continue;
        EditableArea edit;
        edit.id = area.id;
        snprintf(edit.name, sizeof(edit.name), "%s", area.name.AsCharPtr());
        edit.cost = area.cost;
        edit.flags = area.flags;
        edit.color = area.color;
        this->editAreas.Append(edit);
    }
    this->editAreasLoaded = true;
    this->editAreasChanged = false;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
NavigationPanel::ValidateEditAreas() const
{
    for (IndexT i = 0; i < this->editAreas.Size(); i++)
    {
        const Util::String name = this->editAreas[i].name;
        if (name.IsEmpty())
            return Util::Format("Area %d has no name", this->editAreas[i].id);
        if (name == "none" || name == "default")
            return Util::Format("'%s' is a built in area", name.AsCharPtr());
        for (IndexT j = 0; j < i; j++)
        {
            if (name == this->editAreas[j].name)
                return Util::Format("'%s' is there twice", name.AsCharPtr());
        }
    }
    return Util::String();
}

//------------------------------------------------------------------------------
/**
*/
bool
NavigationPanel::SaveEditAreas()
{
    Util::Array<Navigation::Build::AreaDefinition> areas;
    for (const EditableArea& edit : this->editAreas)
    {
        Navigation::Build::AreaDefinition area;
        area.id = edit.id;
        area.name = edit.name;
        area.cost = edit.cost;
        area.flags = edit.flags;
        area.color = edit.color;
        areas.Append(area);
    }
    if (!Navigation::Build::WriteAreaTableSource(areas, IO::URI(Navigation::Build::AreaTableSource)))
    {
        this->tableMessage.Format("Could not write %s", Navigation::Build::AreaTableSource);
        return false;
    }
    this->ReloadTable();
    this->LoadEditAreas();
    return true;
}

//------------------------------------------------------------------------------
/**
    The area types of the project. Built in areas can't be changed
*/
void
NavigationPanel::DrawAreas()
{
    if (!ImGui::CollapsingHeader("Areas", ImGuiTreeNodeFlags_DefaultOpen))
        return;
    if (!this->editAreasLoaded)
        this->LoadEditAreas();

    static const char* flagNames[] = { "Walk", "Swim", "Door", "Disabled" };
    const Navigation::Build::AreaTable& table = GetAreaTable();
    const float swatch = ImGui::GetTextLineHeight();
    const char* plus = Glyph("plus", "+");
    const char* trash = Glyph("trash-2", "x");
    IndexT removeIndex = InvalidIndex;
    Util::String createArea;

    // a row of a built in area, which can't be changed
    auto builtInRow = [&](unsigned char id, const char* tooltip)
    {
        const Navigation::Build::AreaDefinition* area = table.Get(id);
        if (area == nullptr)
            return;
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%d", id);
        ImGui::TableNextColumn();
        const Math::vec4 c = table.GetColor(id);
        ImGui::PushID(id);
        ImGui::ColorButton("##color", ImVec4(c.x, c.y, c.z, id == Navigation::Build::NoArea ? 0.0f : 1.0f),
            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(swatch, swatch));
        ImGui::SameLine();
        ImGui::TextDisabled("%s", area->name.AsCharPtr());
        ImGui::SetItemTooltip("%s", tooltip);
        ImGui::TableNextColumn();
        ImGui::TextDisabled("%.2f", area->cost);
        ImGui::TableNextColumn();
        ImGui::TextDisabled("%s", area->flags & 1 ? "Walk" : "-");
        ImGui::TableNextColumn();
        if (ImGui::SmallButton(plus))
            createArea = area->name;
        ImGui::SetItemTooltip(id == Navigation::Build::NoArea ? "Create an area volume which cuts the surface out of the navmesh" : "Create an area volume of '%s'", area->name.AsCharPtr());
        ImGui::PopID();
    };

    if (ImGui::BeginTable("##areas", 5, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Id");
        ImGui::TableSetupColumn("Area", ImGuiTableColumnFlags_WidthFixed, 170.0f);
        ImGui::TableSetupColumn("Cost", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Flags");
        ImGui::TableSetupColumn("");
        ImGui::TableHeadersRow();

        builtInRow(Navigation::Build::NoArea, "Built in, not walkable. A volume of it cuts the surface out of the navmesh");
        for (IndexT i = 0; i < this->editAreas.Size(); i++)
        {
            EditableArea& area = this->editAreas[i];
            ImGui::PushID((int)i + 1000);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%d", area.id);
            ImGui::TableNextColumn();
            this->editAreasChanged |= ImGui::ColorEdit4("##color", &area.color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);
            this->editAreasChanged |= ImGui::InputText("##name", area.name, sizeof(area.name));
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            this->editAreasChanged |= ImGui::DragFloat("##cost", &area.cost, 0.05f, 0.0f, 1000.0f, "%.2f");
            ImGui::SetItemTooltip("Cost of walking a unit on the area, paths prefer cheap areas");
            ImGui::TableNextColumn();
            for (int bit = 0; bit < 4; bit++)
            {
                if (bit > 0)
                    ImGui::SameLine();
                bool set = (area.flags & (1 << bit)) != 0;
                ImGui::PushID(bit);
                if (ImGui::Checkbox("##flag", &set))
                {
                    area.flags = set ? (area.flags | (1 << bit)) : (area.flags & ~(1 << bit));
                    this->editAreasChanged = true;
                }
                ImGui::PopID();
                ImGui::SetItemTooltip("%s%s", flagNames[bit], bit == 0 ? ", without any flag agents never walk the area" : "");
            }
            ImGui::TableNextColumn();
            if (ImGui::SmallButton(plus))
                createArea = area.name;
            ImGui::SetItemTooltip("Create an area volume of '%s'", area.name);
            ImGui::SameLine();
            if (ImGui::SmallButton(trash))
                removeIndex = i;
            ImGui::SetItemTooltip("Remove the area, volumes of it fall back to default");
            ImGui::PopID();
        }
        builtInRow(Navigation::Build::DefaultWalkableArea, "Built in, every surface nothing marks, and the area of volumes whose area is gone");
        ImGui::EndTable();
    }
    if (removeIndex != InvalidIndex)
    {
        this->editAreas.EraseIndex(removeIndex);
        this->editAreasChanged = true;
    }
    if (createArea.IsValid())
        CreateAreaVolume(createArea.AsCharPtr());

    // a new area takes the lowest free id, ids are stored in baked navmeshes
    ImGui::BeginDisabled(this->editAreas.Size() >= Navigation::Build::DefaultWalkableArea - 1);
    if (ImGui::Button(Util::Format("%s Add area", plus).AsCharPtr()))
    {
        EditableArea area;
        for (unsigned char id = 1; id < Navigation::Build::DefaultWalkableArea; id++)
        {
            bool used = false;
            for (const EditableArea& other : this->editAreas)
                used |= other.id == id;
            if (!used)
            {
                area.id = id;
                break;
            }
        }
        snprintf(area.name, sizeof(area.name), "area%d", area.id);
        // a color of its own, spread around the hue circle by id
        const float hue = Math::fmod(area.id * 0.618034f, 1.0f);
        ImGui::ColorConvertHSVtoRGB(hue, 0.65f, 0.9f, area.color.x, area.color.y, area.color.z);
        area.color.w = 1.0f;
        this->editAreas.Append(area);
        this->editAreasChanged = true;
    }
    ImGui::EndDisabled();

    const Util::String error = this->ValidateEditAreas();
    ImGui::SameLine();
    ImGui::BeginDisabled(!this->editAreasChanged || error.IsValid());
    if (ImGui::Button("Save areas"))
        this->SaveEditAreas();
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("Write %s, compile and load it. Bake again so navmeshes use the changed areas", Navigation::Build::AreaTableSource);
    ImGui::SameLine();
    ImGui::BeginDisabled(!this->editAreasChanged);
    if (ImGui::Button("Revert"))
        this->LoadEditAreas();
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Reload table"))
    {
        this->ReloadTable();
        this->LoadEditAreas();
    }
    ImGui::SetItemTooltip("Compile %s again, after it was edited outside the editor. Unsaved changes are lost", Navigation::Build::AreaTableSource);

    if (error.IsValid())
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", error.AsCharPtr());
    else if (this->editAreasChanged)
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Unsaved changes");
    else if (this->tableMessage.IsValid())
        ImGui::TextDisabled("%s", this->tableMessage.AsCharPtr());
    ImGui::TextDisabled("Volumes refer to areas by name, a renamed or removed area falls back to default");
}

//------------------------------------------------------------------------------
/**
    Paths are tested on any loaded navmesh, an open one or one the scene loads
*/
void
NavigationPanel::DrawPathTest()
{
    if (!ImGui::CollapsingHeader("Path test"))
        return;
    const Util::Array<Navigation::NavMeshId> loaded = LoadedNavMeshes();
    if (loaded.IsEmpty())
    {
        ImGui::TextDisabled("No navmesh is loaded, open one in the asset editor or place one in the scene");
        return;
    }

    static Navigation::NavMeshId picked = Navigation::InvalidNavMeshId;
    if (loaded.FindIndex(picked) == InvalidIndex)
        picked = loaded[0];

    ImGui::SetNextItemWidth(250.0f);
    if (ImGui::BeginCombo("Navmesh##pathtest", NavMeshLabel(picked).AsCharPtr()))
    {
        for (Navigation::NavMeshId id : loaded)
        {
            ImGui::PushID((int)id.resourceId);
            if (ImGui::Selectable(NavMeshLabel(id).AsCharPtr(), id == picked))
                picked = id;
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    // a new pick moves a running test over
    if (IsPathTesting() && PathTestNavMesh() != picked)
        BeginPathTest(picked);

    const bool active = IsPathTesting();
    if (active)
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    if (ImGui::Button(active ? "Stop testing" : "Test paths"))
    {
        if (active)
            EndPathTest();
        else
            BeginPathTest(picked);
    }
    if (active)
        ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::BeginDisabled(!PathTestHasStart() && !PathTestHasEnd());
    if (ImGui::Button("Clear##pathtest"))
        ClearPathTest();
    ImGui::EndDisabled();

    if (!IsPathTesting())
        return;
    ImGui::TextDisabled("Click the navmesh to place the start, shift and click to place the end. Escape stops.");

    const NavPath& path = PathTestPath();
    if (!PathTestHasStart() || !PathTestHasEnd())
    {
        ImGui::TextUnformatted(PathTestHasStart() ? "Place the end" : "Place the start");
    }
    else
    {
        static const char* statusText[] =
        {
            "Found", "Partial, the end can't be reached", "No navmesh near the start", "No navmesh near the end", "The search failed"
        };
        const bool good = path.status == PathStatus::Found;
        ImGui::TextColored(good ? ImVec4(0.5f, 1.0f, 0.5f, 1.0f) : ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "%s", statusText[(int)path.status]);
        if (!path.points.IsEmpty())
            ImGui::Text("Length %.2f, cost %.2f, %d corners", path.length, path.cost, path.points.Size());
    }

    // the costs the path is searched with, starting at the ones of the table
    QueryFilter& filter = PathTestFilter();
    if (ImGui::TreeNode("##pathcosts", "Area costs"))
    {
        const Navigation::Build::AreaTable& table = GetAreaTable();
        for (const Navigation::Build::AreaDefinition& area : table.Areas())
        {
            if (area.id == Navigation::Build::NoArea)
                continue;
            ImGui::PushID(area.id);
            const Math::vec4 c = table.GetColor(area.id);
            ImGui::ColorButton("##color", ImVec4(c.x, c.y, c.z, 1.0f), ImGuiColorEditFlags_NoTooltip, ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120.0f);
            ImGui::DragFloat(area.name.AsCharPtr(), &filter.areaCost[area.id], 0.05f, 0.0f, 1000.0f, "%.2f");
            if (filter.areaCost[area.id] != area.cost)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("table %.2f", area.cost);
            }
            ImGui::PopID();
        }
        if (ImGui::Button("Reset to the table"))
            filter = DefaultQueryFilter();
        ImGui::TreePop();
    }
}

//------------------------------------------------------------------------------
/**
*/
static bool
ParsePoint(const Util::String& text, Math::point& out)
{
    const Util::Array<Util::String> tokens = text.Tokenize(",");
    if (tokens.Size() != 3)
        return false;
    out = Math::point(tokens[0].AsFloat(), tokens[1].AsFloat(), tokens[2].AsFloat());
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
NavigationPanel::CommandNav(const Util::Array<Util::String>& args)
{
    const Util::String verb = args.IsEmpty() ? Util::String("areas") : args[0];
    if (verb == "areas")
    {
        for (const Navigation::Build::AreaDefinition& area : GetAreaTable().Areas())
            this->core->Log(Editor::LogMessageType::N_SYSTEM, Util::Format("%d %s cost %.2f flags %d", area.id, area.name.AsCharPtr(), area.cost, area.flags));
    }
    else if (verb == "areavolume" && args.Size() == 2)
    {
        CreateAreaVolume(args[1].AsCharPtr());
        this->core->Log(Editor::LogMessageType::N_SYSTEM, Util::Format("Created an area volume of '%s'", args[1].AsCharPtr()));
    }
    else if ((verb == "gather" || verb == "bake") && args.Size() == 2)
    {
        NavMeshDocument* document = static_cast<NavMeshDocument*>(Editor::AssetEditorPanel::Find(IO::Path::Parse(args[1])));
        if (document == nullptr || !document->IsA(NavMeshDocument::RTTI) || !document->IsReady())
        {
            this->core->Log(Editor::LogMessageType::N_WARNING, Util::Format("%s isn't open in the asset editor, 'asset open' it first", args[1].AsCharPtr()));
            return;
        }
        if (verb == "gather")
            document->GatherEntities(this->core->GetSelection()->GetSelection(), true);
        else if (!document->Bake())
            this->core->Log(Editor::LogMessageType::N_WARNING, Util::Format("%s wasn't baked, see the asset editor", args[1].AsCharPtr()));
    }
    else if (verb == "path" && args.Size() == 3)
    {
        const Util::Array<Navigation::NavMeshId> loaded = LoadedNavMeshes();
        Math::point start, end;
        if (loaded.IsEmpty() || !ParsePoint(args[1], start) || !ParsePoint(args[2], end))
        {
            this->core->Log(Editor::LogMessageType::N_WARNING, loaded.IsEmpty() ? "No navmesh is loaded" : "nav path takes two points as x,y,z");
            return;
        }
        BeginPathTest(IsPathTesting() ? PathTestNavMesh() : loaded[0]);
        SetPathTestPoint(false, start);
        SetPathTestPoint(true, end);
        const NavPath& path = PathTestPath();
        static const char* statusText[] = { "found", "partial", "no start", "no end", "failed" };
        this->core->Log(Editor::LogMessageType::N_SYSTEM, Util::Format("Path on %s: %s, %d corners, length %.2f, cost %.2f",
            NavMeshLabel(PathTestNavMesh()).AsCharPtr(), statusText[(int)path.status], path.points.Size(), path.length, path.cost));
    }
    else if (verb == "stop")
    {
        EndPathTest();
        EndOutlineEdit();
    }
    else
    {
        this->core->Log(Editor::LogMessageType::N_WARNING, "nav [areas|areavolume <area>|gather <urn>|bake <urn>|path <x,y,z> <x,y,z>|stop]");
    }
}

} // namespace NavigationFeature
#endif // WITH_NEBULA_EDITOR
