//------------------------------------------------------------------------------
//  navigation.cc
//  (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "navigation.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/cmds.h"
#include "editor/components/editorcomponents.h"
#include "editor/ui/windowserver.h"
#include "editor/ui/windows/scene.h"
#include "editor/ui/windows/assetbrowser.h"
#include "editor/tools/selectioncontext.h"
#include "editor/tools/livebatcher.h"
#include "editor/tools/navmeshsources.h"
#include "editor/tools/navareapointtool.h"
#include "editor/tools/navpathtool.h"
#include "navigationfeature/navquery.h"
#include "navigationfeature/navigationfeatureunit.h"
#include "navbuild/navmeshasset.h"
#include "navbuild/navareas.h"
#include "dynui/im3d/im3dcontext.h"
#include "game/componentinspection.h"
#include "navigationfeature/components/navigation.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "resources/resourceserver.h"
#include "io/ioserver.h"
#include "dynui/nebula_icons.h"
#include "imgui.h"

using namespace Editor;

namespace Presentation
{

namespace
{

/// an entity which gave no sources, and why
struct SkippedEntity
{
    Util::String name;
    Util::String reason;
};

struct NavigationEditorState
{
    /// the edited navmesh, folder:name:nav
    IO::Path asset = IO::Path::File("navigation", "navmesh", "nav");
    char nameBuffer[128] = "navmesh";
    /// the description written to the work file
    ::Navigation::NavMeshT description;
    Editor::NavMeshGatherOptions options;

    Util::Array<SkippedEntity> skipped;
    Util::String status;
    bool statusIsError = false;

    /// set while the live batcher bakes this navmesh
    bool baking = false;
    /// the baked navmesh, loaded so it is drawn
    Resources::ResourceId preview = Resources::InvalidResourceId;

    bool folderPicked = false;
    IO::Path pickedFolder;
    bool filePicked = false;
    IO::Path pickedFile;

    /// draw the area volumes of the level in the viewport
    bool showAreaVolumes = true;
    /// the area table is compiled from the project table the first time the window runs
    bool areaTableCompiled = false;
    Util::String areaTableMessage;

    /// the project areas being edited, written to the project table when saved
    struct EditableArea
    {
        unsigned char id = 0;
        char name[64] = "";
        float cost = 1.0f;
        unsigned short flags = 1;
        Math::vec4 color = Math::vec4(1.0f);
    };
    Util::Array<EditableArea> editAreas;
    bool editAreasLoaded = false;
    bool editAreasChanged = false;
} navState;

//------------------------------------------------------------------------------
/**
*/
void
SetStatus(bool error, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    navState.status.FormatArgList(format, args);
    va_end(args);
    navState.statusIsError = error;
}

//------------------------------------------------------------------------------
/**
*/
void
ResetDescription()
{
    navState.description = ::Navigation::NavMeshT();
    navState.description.agent_kind = std::make_unique<::Navigation::AgentT>();
    navState.description.agent_kind->name = "default";
    navState.description.agent_kind->agent_radius = 0.5f;
    navState.description.agent_kind->agent_height = 2.0f;
    navState.description.agent_kind->agent_max_climb = 0.5f;
    navState.description.navmesh_settings = std::make_unique<::Navigation::NavMeshSettingsT>();
}

//------------------------------------------------------------------------------
/**
    A file name, without folders or the separators URNs use
*/
bool
IsValidName(const Util::String& name)
{
    if (name.IsEmpty())
    {
        return false;
    }
    for (IndexT i = 0; i < name.Length(); i++)
    {
        const char c = name[i];
        if (!(isalnum((unsigned char)c) || c == '_' || c == '-'))
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
DiscardPreview()
{
    if (navState.preview != Resources::InvalidResourceId)
    {
        Resources::DiscardResource(navState.preview);
        navState.preview = Resources::InvalidResourceId;
    }
}

//------------------------------------------------------------------------------
/**
    Load the work file of an existing navmesh, returns false if there is none
*/
bool
LoadDescription(const IO::Path& asset)
{
    const IO::URI uri = asset.WorkURI("assets");
    if (!IO::FileExists(uri))
    {
        return false;
    }
    Ptr<IO::Stream> stream = IO::CreateStream(uri);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (!stream->Open())
    {
        SetStatus(true, "Could not open %s", asset.GetWorkFile().AsCharPtr());
        return false;
    }
    bool loaded = false;
    const uint8_t* data = (const uint8_t*)stream->MemoryMap();
    flatbuffers::Verifier verifier(data, (size_t)stream->GetSize());
    if (data != nullptr && verifier.VerifyBuffer<::Navigation::NavMesh>(nullptr))
    {
        ResetDescription();
        Flat::FlatbufferInterface::DeserializeFlatbuffer<::Navigation::NavMesh>(navState.description, data);
        if (navState.description.agent_kind == nullptr)
        {
            navState.description.agent_kind = std::make_unique<::Navigation::AgentT>();
        }
        if (navState.description.navmesh_settings == nullptr)
        {
            navState.description.navmesh_settings = std::make_unique<::Navigation::NavMeshSettingsT>();
        }
        loaded = true;
    }
    else
    {
        SetStatus(true, "%s is not a navmesh work file", asset.GetWorkFile().AsCharPtr());
    }
    stream->MemoryUnmap();
    stream->Close();
    return loaded;
}

//------------------------------------------------------------------------------
/**
    Switch to another navmesh asset, loading it if it exists
*/
void
SetAsset(const IO::Path& asset)
{
    if (!(asset == navState.asset))
    {
        DiscardPreview();
    }
    navState.asset = asset;
    asset.GetFile().CopyToBuffer(navState.nameBuffer, sizeof(navState.nameBuffer));
    navState.skipped.Clear();
    if (LoadDescription(asset))
    {
        SetStatus(false, "Opened %s", asset.GetWorkFile().AsCharPtr());
        // show what was baked before
        if (navState.preview == Resources::InvalidResourceId && IO::FileExists(asset.ExportURI()))
        {
            navState.preview = Resources::CreateResource(asset, "Editor", nullptr, nullptr, {.immediate=true});
        }
    }
}

//------------------------------------------------------------------------------
/**
    Compile the project area table and load it, so names and colors match what a bake uses
*/
void
ReloadAreaTable()
{
    if (::Navigation::Build::CompileAreaTable())
    {
        navState.areaTableMessage.Format("Loaded %s", ::Navigation::Build::AreaTableSource);
    }
    else if (IO::FileExists(::Navigation::Build::AreaTableSource))
    {
        navState.areaTableMessage.Format("%s doesn't compile, see the console", ::Navigation::Build::AreaTableSource);
    }
    else
    {
        navState.areaTableMessage.Format("No %s, only the built in areas exist", ::Navigation::Build::AreaTableSource);
    }
    NavigationFeature::ReloadAreaTable();
    navState.areaTableCompiled = true;
}

//------------------------------------------------------------------------------
/**
    Area volumes are not chosen, every NavArea of the level is part of every navmesh. Volumes outside
    of a navmesh don't touch it
*/
void
GatherAreaVolumes()
{
    auto& modifiers = navState.description.area_modifiers;
    modifiers.erase(std::remove_if(modifiers.begin(), modifiers.end(),
        [](const std::unique_ptr<::Navigation::NavAreaModifierT>& modifier)
        {
            return !modifier->entity_guid.empty();
        }), modifiers.end());
    for (const Editor::NavAreaVolume& volume : Editor::GetNavAreaVolumes())
    {
        modifiers.push_back(Editor::MakeAreaModifier(volume));
    }
}

//------------------------------------------------------------------------------
/**
    The volumes with an area the table doesn't have, which would fail the bake
*/
Util::Array<Util::String>
FindUnknownAreas()
{
    Util::Array<Util::String> unknown;
    const ::Navigation::Build::AreaTable& table = NavigationFeature::GetAreaTable();
    for (const auto& modifier : navState.description.area_modifiers)
    {
        if (!modifier->area.empty() && table.Find(modifier->area.c_str()) == nullptr && unknown.FindIndex(modifier->area.c_str()) == InvalidIndex)
        {
            unknown.Append(modifier->area.c_str());
        }
    }
    return unknown;
}

//------------------------------------------------------------------------------
/**
    Write the description to the work file and have the live batcher bake it
*/
bool
WriteAndBake()
{
    if (!IsValidName(navState.asset.GetFile()))
    {
        SetStatus(true, "'%s' is not a valid name, use letters, digits, '_' and '-'", navState.asset.GetFile().AsCharPtr());
        return false;
    }
    if (navState.description.sources.empty())
    {
        SetStatus(true, "The navmesh has no sources, select static models or colliders and create it from the selection");
        return false;
    }
    navState.description.name = navState.asset.GetFile().AsCharPtr();

    GatherAreaVolumes();
    const Util::Array<Util::String> unknown = FindUnknownAreas();
    if (!unknown.IsEmpty())
    {
        SetStatus(true, "Area volumes use '%s', which isn't in %s", unknown[0].AsCharPtr(), ::Navigation::Build::AreaTableSource);
        return false;
    }

    const IO::URI uri = navState.asset.WorkURI("assets");
    IO::CreateDirectory(IO::URI(uri.LocalPath().ExtractDirName()));
    const Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<::Navigation::NavMesh>(navState.description);
    Ptr<IO::Stream> stream = IO::CreateStream(uri);
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (!stream->Open())
    {
        SetStatus(true, "Could not write %s", uri.LocalPath().AsCharPtr());
        return false;
    }
    stream->Write(data.GetPtr(), data.Size());
    stream->Close();

    // the asset browser picks the new file up through its folder watch
    Editor::LiveBatcher::BatchFile(navState.asset);
    navState.baking = true;
    SetStatus(false, "Baking %s", navState.asset.AsString().AsCharPtr());
    return true;
}

//------------------------------------------------------------------------------
/**
    True if two entries rasterize the same triangles
*/
bool
SameGeometry(const ::Navigation::GeometryEntryT& a, const ::Navigation::GeometryEntryT& b)
{
    if (memcmp(&a.transform, &b.transform, sizeof(Math::mat4)) != 0)
    {
        return false;
    }
    const ::Navigation::MeshSourceT* meshA = a.geometry.AsMeshSource();
    const ::Navigation::MeshSourceT* meshB = b.geometry.AsMeshSource();
    if (meshA != nullptr && meshB != nullptr)
    {
        return meshA->resource == meshB->resource && meshA->mesh_index == meshB->mesh_index && meshA->prim_group == meshB->prim_group;
    }
    const ::Navigation::HeightFieldSourceT* fieldA = a.geometry.AsHeightFieldSource();
    const ::Navigation::HeightFieldSourceT* fieldB = b.geometry.AsHeightFieldSource();
    if (fieldA != nullptr && fieldB != nullptr)
    {
        return fieldA->resource == fieldB->resource && fieldA->height_range == fieldB->height_range
            && fieldA->size_x == fieldB->size_x && fieldA->size_z == fieldB->size_z;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Gather the sources of entities. Entries of the same entities which are already in the description
    are replaced, unless replaceAll drops every source first
*/
void
GatherEntities(const Util::Array<Editor::Entity>& entities, bool replaceAll)
{
    std::vector<std::unique_ptr<::Navigation::GeometryEntryT>> gathered;
    Util::Array<Util::String> gatheredGuids;
    Util::String lastGathered;
    navState.skipped.Clear();
    for (const Editor::Entity& entity : entities)
    {
        Util::String reason;
        const Util::String name = entity.index < (uint)Editor::state.editables.Size() ? Editor::state.editables[entity.index].name : Util::String("?");
        const size_t before = gathered.size();
        if (Editor::GatherNavMeshSources(entity, navState.options, gathered, reason))
        {
            gatheredGuids.Append(Editor::state.editables[entity.index].guid.AsString());
            lastGathered = name;
            // a model and a collider often use the same mesh at the same place, it only has to be rasterized once
            for (size_t i = before; i < gathered.size(); i++)
            {
                for (size_t j = before; j < i; j++)
                {
                    if (gathered[j] != nullptr && gathered[i] != nullptr && SameGeometry(*gathered[i], *gathered[j]))
                    {
                        gathered[i] = nullptr;
                        break;
                    }
                }
            }
        }
        else
        {
            navState.skipped.Append({ name, reason });
        }
    }
    gathered.erase(std::remove(gathered.begin(), gathered.end(), nullptr), gathered.end());

    // without a report every time, a skipped entity is easy to miss when adding entities one by one
    if (navState.skipped.Size() == 1 && entities.Size() == 1)
    {
        SetStatus(true, "%s was skipped: %s", navState.skipped[0].name.AsCharPtr(), navState.skipped[0].reason.AsCharPtr());
    }
    else if (!navState.skipped.IsEmpty())
    {
        SetStatus(true, "%d of %d entities were skipped, see the list below", navState.skipped.Size(), entities.Size());
    }
    else if (entities.Size() == 1)
    {
        SetStatus(false, "Added %s, %d sources", lastGathered.AsCharPtr(), (int)gathered.size());
    }
    else
    {
        SetStatus(false, "Added %d entities, %d sources", entities.Size(), (int)gathered.size());
    }

    auto& sources = navState.description.sources;
    if (replaceAll)
    {
        sources.clear();
    }
    else
    {
        // drop what the gathered entities gave before, so gathering again doesn't add them twice
        sources.erase(std::remove_if(sources.begin(), sources.end(),
            [&](const std::unique_ptr<::Navigation::GeometryEntryT>& entry)
            {
                return !entry->entity_guid.empty() && gatheredGuids.FindIndex(entry->entity_guid.c_str()) != InvalidIndex;
            }), sources.end());
    }
    for (auto& entry : gathered)
    {
        sources.push_back(std::move(entry));
    }
}

//------------------------------------------------------------------------------
/**
    Gather the sources of the entities the navmesh was made from again. Entities which are gone
    are dropped, sources placed by hand stay as they are
*/
void
RebuildFromScene()
{
    Util::Array<Util::String> guids;
    for (const auto& entry : navState.description.sources)
    {
        if (!entry->entity_guid.empty() && guids.FindIndex(entry->entity_guid.c_str()) == InvalidIndex)
        {
            guids.Append(entry->entity_guid.c_str());
        }
    }

    Util::Array<Editor::Entity> entities;
    SizeT missing = 0;
    for (const Util::String& guid : guids)
    {
        const Editor::Entity entity = Editor::FindEntityByGuid(Util::Guid::FromString(guid));
        if (entity == Editor::Entity::Invalid())
        {
            missing++;
        }
        else
        {
            entities.Append(entity);
        }
    }

    // sources of entities which are gone don't come back
    auto& sources = navState.description.sources;
    sources.erase(std::remove_if(sources.begin(), sources.end(),
        [](const std::unique_ptr<::Navigation::GeometryEntryT>& entry)
        {
            return !entry->entity_guid.empty();
        }), sources.end());
    GatherEntities(entities, false);
    if (missing > 0)
    {
        // the names are gone with the entities
        navState.skipped.Append({ Util::String::Sprintf("%d entities", missing), "they are no longer in the level" });
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CountSources(SizeT& meshes, SizeT& heightFields, SizeT& entities)
{
    meshes = heightFields = entities = 0;
    Util::Array<Util::String> guids;
    for (const auto& entry : navState.description.sources)
    {
        if (entry->geometry.AsMeshSource() != nullptr)
        {
            meshes++;
        }
        else if (entry->geometry.AsHeightFieldSource() != nullptr)
        {
            heightFields++;
        }
        if (!entry->entity_guid.empty() && guids.FindIndex(entry->entity_guid.c_str()) == InvalidIndex)
        {
            guids.Append(entry->entity_guid.c_str());
        }
    }
    entities = guids.Size();
}

//------------------------------------------------------------------------------
/**
    The editor entity whose NavMesh component uses the asset, invalid if the level has none
*/
Editor::Entity
FindNavMeshEntity(const IO::Path& asset)
{
    const Util::StringAtom urn = asset.AsString();
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    Game::Filter filter = Game::FilterBuilder()
        .Including<const NavigationFeature::NavMesh, const Editor::EditorEntity>()
        .Build();
    Game::Dataset dataset = world->Query(filter);
    Editor::Entity found = Editor::Entity::Invalid();
    for (IndexT v = 0; v < dataset.numViews && found == Editor::Entity::Invalid(); v++)
    {
        Game::Dataset::View const* view = dataset.views + v;
        for (IndexT i = 0; i < view->numInstances; i++)
        {
            const NavigationFeature::NavMesh& navMesh = *((const NavigationFeature::NavMesh*)view->buffers[0] + i);
            if (navMesh.resource == urn)
            {
                found = ((const Editor::EditorEntity*)view->buffers[1] + i)->id;
                break;
            }
        }
    }
    Game::DestroyFilter(filter);
    return found;
}

//------------------------------------------------------------------------------
/**
    Selects the entity which loads the navmesh in the level, or creates one
*/
void
PlaceInLevel()
{
    const Editor::Entity existing = FindNavMeshEntity(navState.asset);
    if (existing != Editor::Entity::Invalid())
    {
        Edit::SetSelection({ existing });
        SetStatus(false, "%s is already in the level", navState.asset.AsString().AsCharPtr());
        return;
    }

    Edit::CommandManager::BeginMacro("Place navmesh in level", true);
    Editor::Entity entity = Edit::CreateEntity();
    const Game::ComponentId component = Game::GetComponentId<NavigationFeature::NavMesh>();
    // added with its value, a component added in the editor is only in the table from the next frame,
    // setting it right after the add would write to a column which isn't there yet
    NavigationFeature::NavMesh navMesh;
    navMesh.resource = navState.asset.AsString();
    Edit::AddComponent(entity, component, &navMesh);
    Util::String name = "NavMesh ";
    name.Append(navState.asset.GetFile());
    Edit::SetEntityName(entity, name);
    Edit::SetSelection({ entity });
    Edit::CommandManager::EndMacro();
    SetStatus(false, "Placed %s in the level", navState.asset.AsString().AsCharPtr());
}

//------------------------------------------------------------------------------
/**
    A readable name for a loaded navmesh, its file and the agent kind it is for
*/
Util::String
NavMeshLabel(::Navigation::NavMeshId id)
{
    ::Navigation::StreamNavMeshCache* cache = NavigationFeature::GetNavMeshCache();
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
    Paths are tested on any loaded navmesh, the baked one of the window or one the level loads
*/
void
PathTestSection()
{
    if (!ImGui::CollapsingHeader("Path test"))
    {
        return;
    }
    ::Navigation::StreamNavMeshCache* cache = NavigationFeature::GetNavMeshCache();
    Util::Array<::Navigation::NavMeshId> loaded;
    if (cache != nullptr)
    {
        for (::Navigation::NavMeshId id : cache->GetLoadedMeshes())
        {
            if (id != ::Navigation::InvalidNavMeshId && cache->GetDetourMesh(id) != nullptr)
            {
                loaded.Append(id);
            }
        }
    }
    if (loaded.IsEmpty())
    {
        ImGui::TextDisabled("No navmesh is loaded, bake one or place one in the level");
        return;
    }

    // the navmesh of the window first, when it is loaded
    static ::Navigation::NavMeshId picked = ::Navigation::InvalidNavMeshId;
    if (loaded.FindIndex(picked) == InvalidIndex)
    {
        picked = loaded[0];
        if (navState.preview != Resources::InvalidResourceId && loaded.FindIndex(::Navigation::NavMeshId(navState.preview)) != InvalidIndex)
        {
            picked = navState.preview;
        }
    }

    ImGui::SetNextItemWidth(250.0f);
    if (ImGui::BeginCombo("Navmesh##pathtest", NavMeshLabel(picked).AsCharPtr()))
    {
        for (::Navigation::NavMeshId id : loaded)
        {
            ImGui::PushID((int)id.resourceId);
            if (ImGui::Selectable(NavMeshLabel(id).AsCharPtr(), id == picked))
            {
                picked = id;
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    // a new pick moves a running test over
    if (Tools::NavPathTool::Active() && Tools::NavPathTool::NavMesh() != picked)
    {
        Tools::NavPathTool::Begin(picked);
    }

    const bool active = Tools::NavPathTool::Active();
    if (active)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }
    if (ImGui::Button(active ? "Stop testing" : "Test paths"))
    {
        if (active)
        {
            Tools::NavPathTool::End();
        }
        else
        {
            Tools::NavPathTool::Begin(picked);
        }
    }
    if (active)
    {
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!Tools::NavPathTool::HasStart() && !Tools::NavPathTool::HasEnd());
    if (ImGui::Button("Clear##pathtest"))
    {
        Tools::NavPathTool::Clear();
    }
    ImGui::EndDisabled();

    if (!Tools::NavPathTool::Active())
    {
        return;
    }
    ImGui::TextDisabled("Click the navmesh to place the start, shift and click to place the end. Escape stops.");

    // what came of the search
    const NavigationFeature::NavPath& path = Tools::NavPathTool::Path();
    if (!Tools::NavPathTool::HasStart() || !Tools::NavPathTool::HasEnd())
    {
        ImGui::TextUnformatted(Tools::NavPathTool::HasStart() ? "Place the end" : "Place the start");
    }
    else
    {
        static const char* statusText[] =
        {
            "Found", "Partial, the end can't be reached", "No navmesh near the start", "No navmesh near the end", "The search failed"
        };
        const bool good = path.status == NavigationFeature::PathStatus::Found;
        ImGui::TextColored(good ? ImVec4(0.5f, 1.0f, 0.5f, 1.0f) : ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "%s", statusText[(int)path.status]);
        if (!path.points.IsEmpty())
        {
            ImGui::Text("Length %.2f, cost %.2f, %d corners", path.length, path.cost, path.points.Size());
        }
    }

    // the costs the path is searched with, starting at the ones of the table
    NavigationFeature::QueryFilter& filter = Tools::NavPathTool::Filter();
    if (ImGui::TreeNode("##pathcosts", "Area costs"))
    {
        const ::Navigation::Build::AreaTable& table = NavigationFeature::GetAreaTable();
        for (const ::Navigation::Build::AreaDefinition& area : table.Areas())
        {
            if (area.id == ::Navigation::Build::NoArea)
            {
                continue;
            }
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
        {
            filter = NavigationFeature::DefaultQueryFilter();
        }
        ImGui::TreePop();
    }
}

//------------------------------------------------------------------------------
/**
    Once the batcher is done, load the baked navmesh. A loaded navmesh is reloaded by the live batcher itself
*/
void
UpdatePreview()
{
    if (!navState.baking || Editor::LiveBatcher::WorkCounter.counter > 0)
    {
        return;
    }
    navState.baking = false;
    if (!IO::FileExists(navState.asset.ExportURI()))
    {
        SetStatus(true, "Baking %s failed, see the batcher output", navState.asset.AsString().AsCharPtr());
        return;
    }
    // a failed load is never reloaded, try again from scratch
    if (navState.preview != Resources::InvalidResourceId && Resources::ResourceServer::Instance()->GetState(navState.preview) == Resources::Resource::Failed)
    {
        DiscardPreview();
    }
    if (navState.preview == Resources::InvalidResourceId)
    {
        navState.preview = Resources::CreateResource(navState.asset, "Editor", nullptr, nullptr, {.immediate=true});
    }
    SetStatus(false, "Baked %s", navState.asset.AsString().AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
void
AssetSection()
{
    if (navState.folderPicked)
    {
        navState.folderPicked = false;
        SetAsset(IO::Path::File(navState.pickedFolder.GetFolder(), navState.asset.GetFile(), "nav"));
    }
    if (navState.filePicked)
    {
        navState.filePicked = false;
        if (navState.pickedFile.GetType() == "nav")
        {
            SetAsset(navState.pickedFile);
        }
        else
        {
            SetStatus(true, "'%s' is not a navmesh", navState.pickedFile.AsString().AsCharPtr());
        }
    }

    auto browser = (AssetBrowser*)AssetBrowserPickerWindow;
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Folder");
    ImGui::SameLine();
    ImGui::PushID("navfolder");
    const bool pickFolder = ImGui::Button(ICON_ttf_FOLDER_OPEN);
    ImGui::PopID();
    if (pickFolder)
    {
        browser->PickFolder(IO::Path::Folder(navState.asset.GetFolder()), [](const IO::Path& path)
        {
            navState.pickedFolder = path;
            navState.folderPicked = true;
        });
    }
    ImGui::SetItemTooltip("Pick the folder the navmesh is saved in");
    ImGui::SameLine();
    ImGui::Text("assets/%s", navState.asset.GetFolder().AsCharPtr());

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Name");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::InputText("##navname", navState.nameBuffer, sizeof(navState.nameBuffer)))
    {
        // a new name starts a new asset, the description is kept so it can be saved under it
        DiscardPreview();
        navState.asset = IO::Path::File(navState.asset.GetFolder(), navState.nameBuffer, "nav");
    }
    ImGui::SameLine();
    if (ImGui::Button("Open..."))
    {
        browser->PickFile(navState.asset, [](const IO::Path& path)
        {
            navState.pickedFile = path;
            navState.filePicked = true;
        }, "nav");
    }
    ImGui::SetItemTooltip("Open an existing navmesh");

    const bool exists = IO::FileExists(navState.asset.WorkURI("assets"));
    ImGui::TextDisabled("%s, %s", navState.asset.AsString().AsCharPtr(), exists ? "saved" : "not saved yet");
}

//------------------------------------------------------------------------------
/**
*/
void
SettingsSection()
{
    ::Navigation::AgentT& agent = *navState.description.agent_kind;
    ::Navigation::NavMeshSettingsT& settings = *navState.description.navmesh_settings;

    if (ImGui::CollapsingHeader("Agent", ImGuiTreeNodeFlags_DefaultOpen))
    {
        char agentName[64];
        Util::String(agent.name.c_str()).CopyToBuffer(agentName, sizeof(agentName));
        if (ImGui::InputText("Name##agent", agentName, sizeof(agentName)))
        {
            agent.name = agentName;
        }
        ImGui::DragFloat("Radius", &agent.agent_radius, 0.01f, 0.0f, 10.0f, "%.2f m");
        ImGui::DragFloat("Height", &agent.agent_height, 0.01f, 0.1f, 20.0f, "%.2f m");
        ImGui::DragFloat("Max climb", &agent.agent_max_climb, 0.01f, 0.0f, 10.0f, "%.2f m");
    }
    if (ImGui::CollapsingHeader("Build settings"))
    {
        ImGui::DragFloat("Cell size", &settings.cell_size, 0.01f, 0.05f, 5.0f, "%.2f m");
        ImGui::DragFloat("Cell height", &settings.cell_height, 0.01f, 0.05f, 5.0f, "%.2f m");
        ImGui::DragFloat("Max slope", &settings.max_slope, 0.5f, 0.0f, 89.0f, "%.1f deg");
        ImGui::DragInt("Tile size", &settings.tile_size, 1.0f, 8, 1024, "%d cells");
        ImGui::SetItemTooltip("A tile is %.1f m wide", settings.tile_size * settings.cell_size);
        ImGui::DragInt("Region min size", &settings.region_min_size, 1.0f, 0, 150);
        ImGui::DragInt("Region merge size", &settings.region_merge_size, 1.0f, 0, 150);
        ImGui::DragInt("Max edge length", &settings.max_edge_length, 1.0f, 0, 100, "%d m");
        ImGui::DragFloat("Max edge error", &settings.max_edge_error, 0.01f, 0.1f, 3.0f);
        ImGui::DragFloat("Detail sample distance", &settings.detail_sample_dist, 0.1f, 0.0f, 16.0f);
        ImGui::DragFloat("Detail sample error", &settings.detail_sample_max_error, 0.1f, 0.0f, 16.0f);
        if (ImGui::Button("Reset settings"))
        {
            settings = ::Navigation::NavMeshSettingsT();
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SourcesSection()
{
    if (!ImGui::CollapsingHeader("Sources", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Checkbox("Only static entities", &navState.options.onlyStatic);
    ImGui::SetItemTooltip("Use entities marked Static, or below a static body");
    ImGui::SameLine();
    ImGui::Checkbox("Models", &navState.options.models);
    ImGui::SameLine();
    ImGui::Checkbox("Colliders", &navState.options.colliders);

    SizeT meshes, heightFields, entities;
    CountSources(meshes, heightFields, entities);
    ImGui::Text("%d meshes and %d height fields from %d entities", meshes, heightFields, entities);

    const Util::Array<Editor::Entity>& selection = Tools::SelectionContext::Selection();
    ImGui::BeginDisabled(selection.IsEmpty());
    if (ImGui::Button(Util::String::Sprintf("Set from selection (%d)", selection.Size()).AsCharPtr()))
    {
        GatherEntities(selection, true);
    }
    ImGui::SetItemTooltip("Replace the sources with the selected entities");
    ImGui::SameLine();
    if (ImGui::Button("Add selection"))
    {
        GatherEntities(selection, false);
    }
    ImGui::SetItemTooltip("Add the selected entities, or update them if they are already sources");
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(entities == 0);
    if (ImGui::Button("Gather again"))
    {
        RebuildFromScene();
    }
    ImGui::SetItemTooltip("Gather the sources of the same entities again, after they moved or changed");
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Clear"))
    {
        navState.description.sources.clear();
        navState.skipped.Clear();
    }

    if (!navState.description.sources.empty() && ImGui::TreeNodeEx("##baked", ImGuiTreeNodeFlags_DefaultOpen, "Geometry being baked"))
    {
        // grouped by the entity each source was gathered from, sources without an entity were placed by hand
        Util::Array<Util::String> groups;
        for (const auto& entry : navState.description.sources)
        {
            if (groups.FindIndex(entry->entity_guid.c_str()) == InvalidIndex)
            {
                groups.Append(entry->entity_guid.c_str());
            }
        }

        // a long list scrolls instead of growing the window
        const float rowHeight = ImGui::GetTextLineHeightWithSpacing();
        const float listHeight = Math::min(rowHeight * (float)(groups.Size() + navState.description.sources.size() + 1), 300.0f);
        ImGui::BeginChild("##bakedlist", ImVec2(500.0f, listHeight), ImGuiChildFlags_Borders);

        Util::String removeGroup;
        bool removeRequested = false;
        for (IndexT g = 0; g < groups.Size(); g++)
        {
            const Util::String& guid = groups[g];
            Util::String name;
            if (guid.IsEmpty())
            {
                name = "Placed by hand";
            }
            else
            {
                const Editor::Entity entity = Editor::FindEntityByGuid(Util::Guid::FromString(guid));
                name = entity != Editor::Entity::Invalid() ? Editor::state.editables[entity.index].name : Util::String("(not in the level)");
            }
            int count = 0;
            for (const auto& entry : navState.description.sources)
            {
                count += guid == entry->entity_guid.c_str() ? 1 : 0;
            }

            ImGui::PushID(g);
            if (ImGui::SmallButton(ICON_ttf_TRASH))
            {
                removeGroup = guid;
                removeRequested = true;
            }
            ImGui::SetItemTooltip("Remove from the navmesh");
            ImGui::SameLine();
            if (ImGui::TreeNodeEx("##group", ImGuiTreeNodeFlags_DefaultOpen, "%s (%d)", name.AsCharPtr(), count))
            {
                for (const auto& entry : navState.description.sources)
                {
                    if (!(guid == entry->entity_guid.c_str()))
                    {
                        continue;
                    }
                    const Math::vec4& p = entry->transform.position();
                    if (const ::Navigation::MeshSourceT* mesh = entry->geometry.AsMeshSource())
                    {
                        ImGui::BulletText("%s", mesh->resource.c_str());
                        ImGui::SameLine();
                        ImGui::TextDisabled("mesh %u, group %u, at %.1f %.1f %.1f", mesh->mesh_index, mesh->prim_group, p.x, p.y, p.z);
                    }
                    else if (const ::Navigation::HeightFieldSourceT* field = entry->geometry.AsHeightFieldSource())
                    {
                        ImGui::BulletText("%s", field->resource.c_str());
                        ImGui::SameLine();
                        ImGui::TextDisabled("height field %.1f x %.1f m, %.1f m high, at %.1f %.1f %.1f",
                            field->size_x, field->size_z, field->height_range, p.x, p.y, p.z);
                    }
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        ImGui::EndChild();

        if (removeRequested)
        {
            auto& sources = navState.description.sources;
            sources.erase(std::remove_if(sources.begin(), sources.end(),
                [&](const std::unique_ptr<::Navigation::GeometryEntryT>& entry)
                {
                    return removeGroup == entry->entity_guid.c_str();
                }), sources.end());
        }
        ImGui::TreePop();
    }

    // the volumes are gathered on every bake, the level is what is listed
    const Util::Array<Editor::NavAreaVolume> volumes = Editor::GetNavAreaVolumes();
    if (ImGui::TreeNode("##volumes", "Area volumes (%d)", volumes.Size()))
    {
        if (volumes.IsEmpty())
        {
            ImGui::TextDisabled("Create them with Create > Navigation > Area Volume");
        }
        const ::Navigation::Build::AreaTable& table = NavigationFeature::GetAreaTable();
        for (const Editor::NavAreaVolume& volume : volumes)
        {
            const ::Navigation::Build::AreaDefinition* area = table.Find(volume.area.Value());
            const Util::String name = volume.entity.index < (uint)Editor::state.editables.Size() ? Editor::state.editables[volume.entity.index].name : Util::String("?");
            if (area != nullptr)
            {
                const Math::vec4 c = table.GetColor(area->id);
                ImGui::ColorButton("##color", ImVec4(c.x, c.y, c.z, 1.0f), ImGuiColorEditFlags_NoTooltip, ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()));
                ImGui::SameLine();
                ImGui::Text("%s: %s, priority %d", name.AsCharPtr(), volume.area.Value(), volume.priority);
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s: '%s' is not in the area table", name.AsCharPtr(), volume.area.Value());
            }
        }
        ImGui::TreePop();
    }

    if (!navState.skipped.IsEmpty())
    {
        if (ImGui::TreeNode("##skipped", "%d entities were skipped", navState.skipped.Size()))
        {
            for (const SkippedEntity& skipped : navState.skipped)
            {
                ImGui::BulletText("%s: %s", skipped.name.AsCharPtr(), skipped.reason.AsCharPtr());
            }
            ImGui::TreePop();
        }
    }
}

//------------------------------------------------------------------------------
/**
    The project areas of the table, to edit
*/
void
LoadEditAreas()
{
    navState.editAreas.Clear();
    for (const ::Navigation::Build::AreaDefinition& area : NavigationFeature::GetAreaTable().Areas())
    {
        if (area.id == ::Navigation::Build::NoArea || area.id == ::Navigation::Build::DefaultWalkableArea)
        {
            continue;
        }
        NavigationEditorState::EditableArea edit;
        edit.id = area.id;
        snprintf(edit.name, sizeof(edit.name), "%s", area.name.AsCharPtr());
        edit.cost = area.cost;
        edit.flags = area.flags;
        edit.color = area.color;
        navState.editAreas.Append(edit);
    }
    navState.editAreasLoaded = true;
    navState.editAreasChanged = false;
}

//------------------------------------------------------------------------------
/**
    What's wrong with the edited areas, empty if they can be saved
*/
Util::String
ValidateEditAreas()
{
    for (IndexT i = 0; i < navState.editAreas.Size(); i++)
    {
        const Util::String name = navState.editAreas[i].name;
        if (name.IsEmpty())
        {
            return Util::String::Sprintf("Area %d has no name", navState.editAreas[i].id);
        }
        if (name == "none" || name == "default")
        {
            return Util::String::Sprintf("'%s' is a built in area", name.AsCharPtr());
        }
        for (IndexT j = 0; j < i; j++)
        {
            if (name == navState.editAreas[j].name)
            {
                return Util::String::Sprintf("'%s' is there twice", name.AsCharPtr());
            }
        }
    }
    return Util::String();
}

//------------------------------------------------------------------------------
/**
    Writes the edited areas to the project table, compiles and loads it
*/
bool
SaveEditAreas()
{
    Util::Array<::Navigation::Build::AreaDefinition> areas;
    for (const NavigationEditorState::EditableArea& edit : navState.editAreas)
    {
        ::Navigation::Build::AreaDefinition area;
        area.id = edit.id;
        area.name = edit.name;
        area.cost = edit.cost;
        area.flags = edit.flags;
        area.color = edit.color;
        areas.Append(area);
    }
    if (!::Navigation::Build::WriteAreaTableSource(areas, IO::URI(::Navigation::Build::AreaTableSource)))
    {
        navState.areaTableMessage.Format("Could not write %s", ::Navigation::Build::AreaTableSource);
        return false;
    }
    ReloadAreaTable();
    LoadEditAreas();
    return true;
}

//------------------------------------------------------------------------------
/**
    A new area volume in the level, with the area given, selected so it can be placed and its outline edited
*/
void
CreateAreaVolume(const char* area)
{
    Edit::CommandManager::BeginMacro("Create navigation area", true);
    Editor::Entity entity = Edit::CreateEntity();
    NavigationFeature::NavArea navArea;
    navArea.area = area;
    Edit::AddComponent(entity, Game::GetComponentId<NavigationFeature::NavArea>(), &navArea);
    Util::String name = "Nav Area ";
    name.Append(area);
    name.Append(" ");
    name.AppendInt(entity.index);
    Edit::SetEntityName(entity, name);
    Edit::SetSelection({ entity });
    Edit::CommandManager::EndMacro();
}

//------------------------------------------------------------------------------
/**
    The area types of the project, to edit. The table is written to the project table and compiled when
    saved. Volumes and navmeshes refer to areas by name, one which was renamed or removed falls back to default
*/
void
AreasSection()
{
    if (!ImGui::CollapsingHeader("Areas"))
    {
        return;
    }
    if (!navState.editAreasLoaded)
    {
        LoadEditAreas();
    }

    static const char* flagNames[] = { "Walk", "Swim", "Door", "Disabled" };
    const ::Navigation::Build::AreaTable& table = NavigationFeature::GetAreaTable();
    const float swatch = ImGui::GetTextLineHeight();
    IndexT removeIndex = InvalidIndex;

    // a row of a built in area, which can't be changed
    auto builtInRow = [&](unsigned char id, const char* tooltip)
    {
        const ::Navigation::Build::AreaDefinition* area = table.Get(id);
        if (area == nullptr)
        {
            return;
        }
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%d", id);
        ImGui::TableNextColumn();
        const Math::vec4 c = table.GetColor(id);
        ImGui::PushID(id);
        ImGui::ColorButton("##color", ImVec4(c.x, c.y, c.z, id == ::Navigation::Build::NoArea ? 0.0f : 1.0f),
            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(swatch, swatch));
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::TextDisabled("%s", area->name.AsCharPtr());
        ImGui::SetItemTooltip("%s", tooltip);
        ImGui::TableNextColumn();
        ImGui::TextDisabled("%.2f", area->cost);
        ImGui::TableNextColumn();
        ImGui::TextDisabled("%s", area->flags & 1 ? "Walk" : "-");
        ImGui::TableNextColumn();
        if (id != ::Navigation::Build::NoArea)
        {
            ImGui::PushID(id);
            if (ImGui::SmallButton(ICON_ttf_PLUS))
            {
                CreateAreaVolume(area->name.AsCharPtr());
            }
            ImGui::SetItemTooltip("Create an area volume of '%s'", area->name.AsCharPtr());
            ImGui::PopID();
        }
        else
        {
            ImGui::PushID(id);
            if (ImGui::SmallButton(ICON_ttf_PLUS))
            {
                CreateAreaVolume("none");
            }
            ImGui::SetItemTooltip("Create an area volume which cuts the surface out of the navmesh");
            ImGui::PopID();
        }
    };

    if (ImGui::BeginTable("##areas", 5, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Id");
        ImGui::TableSetupColumn("Area", ImGuiTableColumnFlags_WidthFixed, 170.0f);
        ImGui::TableSetupColumn("Cost", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Flags");
        ImGui::TableSetupColumn("");
        ImGui::TableHeadersRow();

        builtInRow(::Navigation::Build::NoArea, "Built in, not walkable. A volume of it cuts the surface out of the navmesh");
        for (IndexT i = 0; i < navState.editAreas.Size(); i++)
        {
            NavigationEditorState::EditableArea& area = navState.editAreas[i];
            ImGui::PushID((int)i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%d", area.id);
            ImGui::TableNextColumn();
            navState.editAreasChanged |= ImGui::ColorEdit4("##color", &area.color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);
            navState.editAreasChanged |= ImGui::InputText("##name", area.name, sizeof(area.name));
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            navState.editAreasChanged |= ImGui::DragFloat("##cost", &area.cost, 0.05f, 0.0f, 1000.0f, "%.2f");
            ImGui::SetItemTooltip("Cost of walking a unit on the area, paths prefer cheap areas");
            ImGui::TableNextColumn();
            for (int bit = 0; bit < 4; bit++)
            {
                if (bit > 0)
                {
                    ImGui::SameLine();
                }
                bool set = (area.flags & (1 << bit)) != 0;
                ImGui::PushID(bit);
                if (ImGui::Checkbox("##flag", &set))
                {
                    area.flags = set ? (area.flags | (1 << bit)) : (area.flags & ~(1 << bit));
                    navState.editAreasChanged = true;
                }
                ImGui::PopID();
                ImGui::SetItemTooltip("%s%s", flagNames[bit], bit == 0 ? ", without any flag agents never walk the area" : "");
            }
            ImGui::TableNextColumn();
            if (ImGui::SmallButton(ICON_ttf_PLUS))
            {
                CreateAreaVolume(area.name);
            }
            ImGui::SetItemTooltip("Create an area volume of '%s'", area.name);
            ImGui::SameLine();
            if (ImGui::SmallButton(ICON_ttf_TRASH))
            {
                removeIndex = i;
            }
            ImGui::SetItemTooltip("Remove the area, volumes of it fall back to default");
            ImGui::PopID();
        }
        builtInRow(::Navigation::Build::DefaultWalkableArea, "Built in, every surface nothing marks, and the area of volumes whose area is gone");
        ImGui::EndTable();
    }
    if (removeIndex != InvalidIndex)
    {
        navState.editAreas.EraseIndex(removeIndex);
        navState.editAreasChanged = true;
    }

    // a new area takes the lowest free id, ids are stored in baked navmeshes
    ImGui::BeginDisabled(navState.editAreas.Size() >= ::Navigation::Build::DefaultWalkableArea - 1);
    if (ImGui::Button((Util::String(ICON_ttf_PLUS) + " Add area").AsCharPtr()))
    {
        NavigationEditorState::EditableArea area;
        for (unsigned char id = 1; id < ::Navigation::Build::DefaultWalkableArea; id++)
        {
            bool used = false;
            for (const NavigationEditorState::EditableArea& other : navState.editAreas)
            {
                used |= other.id == id;
            }
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
        navState.editAreas.Append(area);
        navState.editAreasChanged = true;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button((Util::String(ICON_ttf_PLUS) + " Area volume").AsCharPtr()))
    {
        CreateAreaVolume("default");
    }
    ImGui::SetItemTooltip("Create an area volume in the level, the + of a row creates one of that area");

    const Util::String error = ValidateEditAreas();
    ImGui::BeginDisabled(!navState.editAreasChanged || error.IsValid());
    if (ImGui::Button("Save areas"))
    {
        SaveEditAreas();
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("Write %s, compile and load it. Bake again so navmeshes use the changed areas", ::Navigation::Build::AreaTableSource);
    ImGui::SameLine();
    ImGui::BeginDisabled(!navState.editAreasChanged);
    if (ImGui::Button("Revert"))
    {
        LoadEditAreas();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Reload table"))
    {
        ReloadAreaTable();
        LoadEditAreas();
    }
    ImGui::SetItemTooltip("Compile %s again, after it was edited outside the editor. Unsaved changes are lost", ::Navigation::Build::AreaTableSource);
    if (error.IsValid())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", error.AsCharPtr());
    }
    else if (navState.editAreasChanged)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Unsaved changes");
    }
    else
    {
        ImGui::TextDisabled("%s", navState.areaTableMessage.AsCharPtr());
    }
    ImGui::TextDisabled("Volumes refer to areas by name, a renamed or removed area falls back to default");
    ImGui::Checkbox("Show area volumes", &navState.showAreaVolumes);
}

//------------------------------------------------------------------------------
/**
    The inspector field of NavAgent.agentKind, a choice of the agent kinds navmeshes are loaded or
    being set up for, or any name typed in
*/
void
DrawAgentKindField(Game::Entity owner, Game::ComponentId component, void* data, bool* commit)
{
    Util::StringAtom& value = *(Util::StringAtom*)data;
    Util::Array<Util::String> kinds;
    ::Navigation::StreamNavMeshCache* cache = NavigationFeature::GetNavMeshCache();
    if (cache != nullptr)
    {
        for (::Navigation::NavMeshId id : cache->GetLoadedMeshes())
        {
            if (id == ::Navigation::InvalidNavMeshId || cache->GetDetourMesh(id) == nullptr)
            {
                continue;
            }
            const Util::String name = cache->GetAgent(id).name.c_str();
            if (!name.IsEmpty() && kinds.FindIndex(name) == InvalidIndex)
            {
                kinds.Append(name);
            }
        }
    }
    if (navState.description.agent_kind != nullptr && !navState.description.agent_kind->name.empty())
    {
        const Util::String name = navState.description.agent_kind->name.c_str();
        if (kinds.FindIndex(name) == InvalidIndex)
        {
            kinds.Append(name);
        }
    }

    const Util::String current = value.IsValid() ? value.Value() : "";
    const bool known = kinds.FindIndex(current) != InvalidIndex;
    Util::String preview = current;
    if (!known)
    {
        // no navmesh for it is loaded, the agent waits for one
        preview.Append(current.IsEmpty() ? "(none)" : " (no navmesh loaded)");
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.3f, 1.0f));
    }
    ImGui::SetNextItemWidth(-FLT_MIN);
    const bool open = ImGui::BeginCombo("##agentkind", preview.AsCharPtr());
    if (!known)
    {
        ImGui::PopStyleColor();
    }
    if (open)
    {
        for (const Util::String& kind : kinds)
        {
            const bool selected = kind == current;
            if (ImGui::Selectable(kind.AsCharPtr(), selected))
            {
                value = kind;
                *commit = true;
            }
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        if (!kinds.IsEmpty())
        {
            ImGui::Separator();
        }
        static char typed[64] = "";
        ImGui::SetNextItemWidth(160.0f);
        if (ImGui::InputTextWithHint("##typed", "other kind", typed, sizeof(typed), ImGuiInputTextFlags_EnterReturnsTrue) && typed[0] != '\0')
        {
            value = Util::String(typed);
            typed[0] = '\0';
            *commit = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::TextDisabled("Kinds of the loaded navmeshes and the one being set up");
        ImGui::EndCombo();
    }
}

//------------------------------------------------------------------------------
/**
    The inspector field of NavArea.area, a choice of the areas in the area table
*/
void
DrawAreaField(Game::Entity owner, Game::ComponentId component, void* data, bool* commit)
{
    Util::StringAtom& value = *(Util::StringAtom*)data;
    const ::Navigation::Build::AreaTable& table = NavigationFeature::GetAreaTable();
    const ::Navigation::Build::AreaDefinition* current = value.IsValid() ? table.Find(value.Value()) : nullptr;
    const float swatch = ImGui::GetTextLineHeight();

    Util::String preview = value.IsValid() ? value.Value() : "";
    if (current == nullptr)
    {
        // a name the table lost keeps showing, so it can be seen and replaced
        preview.Append(" (not in the area table)");
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
    }
    ImGui::SetNextItemWidth(-FLT_MIN);
    const bool open = ImGui::BeginCombo("##area", preview.AsCharPtr());
    if (current == nullptr)
    {
        ImGui::PopStyleColor();
    }
    if (open)
    {
        for (const ::Navigation::Build::AreaDefinition& area : table.Areas())
        {
            ImGui::PushID(area.id);
            const Math::vec4 c = table.GetColor(area.id);
            ImGui::ColorButton("##color", ImVec4(c.x, c.y, c.z, area.id == ::Navigation::Build::NoArea ? 0.0f : 1.0f),
                ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(swatch, swatch));
            ImGui::SameLine();
            const bool selected = current == &area;
            if (ImGui::Selectable(area.name.AsCharPtr(), selected))
            {
                value = area.name;
                *commit = true;
            }
            if (area.id == ::Navigation::Build::NoArea)
            {
                ImGui::SetItemTooltip("Cuts the surface out of the navmesh");
            }
            else if (area.id == ::Navigation::Build::DefaultWalkableArea)
            {
                ImGui::SetItemTooltip("The area of every surface nothing marks");
            }
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
            ImGui::PopID();
        }
        ImGui::Separator();
        ImGui::TextDisabled("Areas are defined in %s", ::Navigation::Build::AreaTableSource);
        ImGui::EndCombo();
    }
}

//------------------------------------------------------------------------------
/**
    The inspector of NavArea: its fields, and the outline which replaces the box
*/
void
DrawNavAreaInspector(Game::Entity owner, Game::ComponentId component, void* data, bool* commit)
{
    Game::ComponentDrawFuncT<NavigationFeature::NavArea>(owner, component, data, commit);

    NavigationFeature::NavArea& navArea = *(NavigationFeature::NavArea*)data;
    const uint capacity = (uint)navArea.points.size();
    navArea.pointCount = Math::min(navArea.pointCount, capacity);
    const bool hasOutline = navArea.pointCount >= 3;

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Outline");
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID("navareaoutline");

    const bool editing = Tools::NavAreaPointTool::Target() == owner;
    // the label changes the moment it takes effect, a fixed id keeps the button the same item
    if (ImGui::Button(editing ? "Stop editing###editoutline" : "Edit outline###editoutline"))
    {
        if (editing)
        {
            Tools::NavAreaPointTool::End();
        }
        else
        {
            Tools::NavAreaPointTool::Begin(owner);
        }
    }
    // No hover tooltip with the instructions: the inspector sits at the edge of the editor, a wide tooltip
    // left the main window and became an OS window of its own, which took the click away from the button
    ImGui::SameLine();
    if (hasOutline)
    {
        ImGui::Text("%u points", navArea.pointCount);
        ImGui::SameLine();
        if (ImGui::Button("Reset to box"))
        {
            Tools::NavAreaPointTool::End();
            navArea.pointCount = 0;
            *commit = true;
        }
    }
    else
    {
        ImGui::TextDisabled("the box");
    }

    if (editing)
    {
        ImGui::TextDisabled("Viewport: click adds, drag moves, right click or Del removes, Esc stops");
    }

    if (hasOutline)
    {
        // the outline in the entity frame, to check its edges
        Util::Array<Math::vec3> outline;
        for (uint i = 0; i < navArea.pointCount; i++)
        {
            outline.Append(navArea.points[i]);
        }
        if (!::Navigation::Build::IsSimpleOutlineXZ(outline.Begin(), outline.Size()))
        {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Edges of the outline cross, move a point");
        }

        // the point under the mouse in the viewport is highlighted here, a hovered row highlights its point there
        const IndexT viewportHovered = editing ? Tools::NavAreaPointTool::HoveredPoint() : InvalidIndex;
        IndexT rowHovered = InvalidIndex;
        int remove = -1;
        for (uint i = 0; i < navArea.pointCount; i++)
        {
            ImGui::PushID((int)i);
            const bool highlight = (IndexT)i == viewportHovered;
            if (highlight)
            {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 0.85f, 0.2f, 0.35f));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0f, 0.85f, 0.2f, 0.5f));
            }
            // an outline keeps three points, Reset to box removes it
            ImGui::BeginDisabled(navArea.pointCount <= 3);
            if (ImGui::SmallButton(ICON_ttf_TRASH))
            {
                remove = (int)i;
            }
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                rowHovered = i;
            }
            ImGui::SetItemTooltip("Remove the point");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);
            // x and z place the point, y is the height it was put at
            if (ImGui::DragFloat3("##point", &navArea.points[i].x, 0.05f, 0.0f, 0.0f, "%.2f"))
            {
                *commit = true;
            }
            if (ImGui::IsItemHovered() || ImGui::IsItemActive())
            {
                rowHovered = i;
            }
            if (highlight)
            {
                ImGui::PopStyleColor(2);
            }
            ImGui::PopID();
        }
        Tools::NavAreaPointTool::SetHighlightedPoint(rowHovered);
        if (remove >= 0)
        {
            for (uint i = (uint)remove; i + 1 < navArea.pointCount; i++)
            {
                navArea.points[i] = navArea.points[i + 1];
            }
            navArea.pointCount--;
            *commit = true;
        }
    }
    ImGui::PopID();
}

} // namespace

//------------------------------------------------------------------------------
/**
*/
Navigation::Navigation()
{
    this->additionalFlags = ImGuiWindowFlags_(ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
    ResetDescription();
    // the area of a volume is chosen from the area table instead of typed
    Game::ComponentInspection::OverrideField(Game::GetComponentId<NavigationFeature::NavArea>(), "area", &DrawAreaField);
    Game::ComponentInspection::OverrideField(Game::GetComponentId<NavigationFeature::NavAgent>(), "agentKind", &DrawAgentKindField);
    // the points have their own list, with a tool which places them in the viewport
    Game::ComponentInspection::OverrideInspector(Game::GetComponentId<NavigationFeature::NavArea>(), &DrawNavAreaInspector);
}

//------------------------------------------------------------------------------
/**
*/
Navigation::~Navigation()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Navigation::Run(SaveMode save)
{
    if (this->defaultCamera == Graphics::GraphicsEntityId::Invalid())
    {
        Scene* sceneWindow = (Presentation::Scene*)Presentation::SceneWindow;
        this->defaultCamera = ViewGetCamera(sceneWindow->viewPort.GetView());
    }
    if (this->defaultCamera == Graphics::GraphicsEntityId::Invalid())
    {
        return;
    }

    UpdatePreview();
    if (!navState.areaTableCompiled)
    {
        ReloadAreaTable();
    }

    AssetSection();
    ImGui::Separator();
    SettingsSection();
    SourcesSection();
    AreasSection();
    ImGui::Separator();

    ImGui::BeginDisabled(navState.baking);
    if (ImGui::Button(navState.baking ? "Baking..." : "Save and bake"))
    {
        WriteAndBake();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    const bool baked = IO::FileExists(navState.asset.ExportURI());
    ImGui::BeginDisabled(navState.baking || !baked);
    const bool inLevel = baked && FindNavMeshEntity(navState.asset) != Editor::Entity::Invalid();
    if (ImGui::Button(inLevel ? "Select in level" : "Place in level"))
    {
        PlaceInLevel();
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip(inLevel ? "Select the entity which loads this navmesh in the level" :
        baked ? "Add an entity with a NavMesh component to the level, which loads this navmesh in the game" :
        "Bake the navmesh first");
    if (navState.status.IsValid())
    {
        ImGui::SameLine();
        if (navState.statusIsError)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", navState.status.AsCharPtr());
        }
        else
        {
            ImGui::TextUnformatted(navState.status.AsCharPtr());
        }
    }

    PathTestSection();

    NavigationFeature::RenderUI(this->defaultCamera);
}

//------------------------------------------------------------------------------
/**
    Runs every frame, also when the window is hidden, so the volumes are visible while they are edited
*/
void
Navigation::Update()
{
    if (!navState.showAreaVolumes || Editor::state.editorWorld == nullptr)
    {
        return;
    }
    const ::Navigation::Build::AreaTable& table = NavigationFeature::GetAreaTable();
    const Util::Array<Editor::Entity>& selection = Tools::SelectionContext::Selection();
    for (const Editor::NavAreaVolume& volume : Editor::GetNavAreaVolumes())
    {
        const ::Navigation::Build::AreaDefinition* area = table.Find(volume.area.Value());
        // an unknown area is drawn in red, it fails the bake
        Math::vec4 color = area != nullptr ? table.GetColor(area->id) : Math::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        const bool selected = selection.FindIndex(volume.entity) != InvalidIndex;
        color.w = selected ? 1.0f : 0.6f;
        if (volume.points.Size() < 3)
        {
            Im3d::Im3dContext::DrawOrientedBox(volume.transform, volume.localBox, color, Im3d::CheckDepth | Im3d::Wireframe);
        }
        else
        {
            // the outline is what gets marked, crossing edges are drawn red
            const ::Navigation::Build::AreaPrism prism = Editor::NavAreaPrism(volume);
            const bool simple = ::Navigation::Build::IsSimpleOutlineXZ(prism.outline.Begin(), prism.outline.Size());
            Editor::DrawAreaPrism(prism, simple ? color : Math::vec4(1.0f, 0.0f, 0.0f, 1.0f));
            if (selected && Tools::NavAreaPointTool::Target() != volume.entity)
            {
                const IndexT highlighted = Tools::NavAreaPointTool::HighlightedPoint();
                for (IndexT i = 0; i < volume.points.Size(); i++)
                {
                    const Math::vec3& point = volume.points[i];
                    const Math::vec4 p = volume.pointTransform * Math::vec4(point.x, point.y, point.z, 1.0f);
                    if (i == highlighted)
                    {
                        Im3d::Im3dContext::DrawPoint(Math::vec3(p.x, p.y, p.z), 16.0f, Math::vec4(1.0f, 0.85f, 0.2f, 1.0f));
                    }
                    else
                    {
                        Im3d::Im3dContext::DrawPoint(Math::vec3(p.x, p.y, p.z), 8.0f, color);
                    }
                }
            }
        }
        if (selected)
        {
            const Math::vec4 center = volume.transform.position();
            Im3d::Im3dContext::DrawText(Math::vec3(center.x, center.y, center.z), volume.area.Value(), 16.0f, color);
        }
    }
}

} // namespace Presentation
