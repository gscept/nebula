//------------------------------------------------------------------------------
//  @file navmeshdocument.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "navigationfeature/editor/navmeshdocument.h"

#if WITH_NEBULA_EDITOR
#include "editorassets/asseteditorpanel.h"
#include "editorassets/livebatcher.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/iconregistry.h"
#include "neweditor/commands/addcomponentcommand.h"
#include "neweditor/commands/createentitycommand.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/model/scene.h"
#include "neweditor/model/selectioncontext.h"
#include "navigationfeature/components/navigation.h"
#include "navigationfeature/navigationfeatureunit.h"
#include "navbuild/navareas.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "resources/resourceserver.h"
#include "io/ioserver.h"
#include "imgui.h"
#include <algorithm>

namespace NavigationFeature
{

__ImplementClass(NavigationFeature::NavMeshDocument, 'NVMD', Editor::AssetDocument);

/// the documents that are open, for the agent kinds they set up
static Util::Array<NavMeshDocument*> OpenDocuments;

//------------------------------------------------------------------------------
/**
    A change to the description, as it was and as it is
*/
class NavMeshEditCommand : public Editor::EditCommand
{
    __DeclareClass(NavMeshEditCommand)
public:
    const char* Name() const override { return this->name; }
    bool Execute() override
    {
        this->document->ApplyDescription(this->after);
        return true;
    }
    bool Unexecute() override
    {
        this->document->ApplyDescription(this->before);
        return true;
    }

    Ptr<NavMeshDocument> document;
    const char* name = "Edit navmesh";
    Util::Blob before;
    Util::Blob after;
};
__ImplementClass(NavigationFeature::NavMeshEditCommand, 'NVEC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
*/
static const char*
Glyph(const char* name)
{
    const char* glyph = Editor::IconGlyph(name);
    return glyph != nullptr ? glyph : "x";
}

//------------------------------------------------------------------------------
/**
*/
static Util::Blob
Serialize(const Navigation::NavMeshT& description)
{
    return Flat::FlatbufferInterface::SerializeFlatbuffer<Navigation::NavMesh>(description);
}

//------------------------------------------------------------------------------
/**
    A file name, without folders or the separators URNs use
*/
static bool
IsValidName(const Util::String& name)
{
    if (name.IsEmpty())
        return false;
    for (IndexT i = 0; i < name.Length(); i++)
    {
        const char c = name[i];
        if (!(isalnum((unsigned char)c) || c == '_' || c == '-'))
            return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    True if two entries rasterize the same triangles
*/
static bool
SameGeometry(const Navigation::GeometryEntryT& a, const Navigation::GeometryEntryT& b)
{
    if (memcmp(&a.transform, &b.transform, sizeof(Math::mat4)) != 0)
        return false;
    const Navigation::MeshSourceT* meshA = a.geometry.AsMeshSource();
    const Navigation::MeshSourceT* meshB = b.geometry.AsMeshSource();
    if (meshA != nullptr && meshB != nullptr)
        return meshA->resource == meshB->resource && meshA->mesh_index == meshB->mesh_index && meshA->prim_group == meshB->prim_group;
    const Navigation::HeightFieldSourceT* fieldA = a.geometry.AsHeightFieldSource();
    const Navigation::HeightFieldSourceT* fieldB = b.geometry.AsHeightFieldSource();
    if (fieldA != nullptr && fieldB != nullptr)
        return fieldA->resource == fieldB->resource && fieldA->height_range == fieldB->height_range && fieldA->size_x == fieldB->size_x && fieldA->size_z == fieldB->size_z;
    return false;
}

//------------------------------------------------------------------------------
/**
*/
NavMeshDocument::NavMeshDocument() :
    statusIsError(false),
    baking(false),
    preview(Resources::InvalidResourceId)
{
    OpenDocuments.Append(this);
}

//------------------------------------------------------------------------------
/**
*/
NavMeshDocument::~NavMeshDocument()
{
    const IndexT index = OpenDocuments.FindIndex(this);
    if (index != InvalidIndex)
        OpenDocuments.EraseIndex(index);
}

//------------------------------------------------------------------------------
/**
*/
void
NavMeshDocument::OpenAsset(const IO::Path& asset)
{
    Ptr<NavMeshDocument> document = NavMeshDocument::Create();
    document->asset = asset;
    Editor::AssetEditorPanel::Open(document.upcast<Editor::AssetDocument>());
}

//------------------------------------------------------------------------------
/**
    A new navmesh has the default agent and settings and no sources, it is set up in the document
*/
bool
NavMeshDocument::CreateAsset(const IO::Path& asset, const IO::URI& workFile, const Util::String& variant, Util::String& outError)
{
    Ptr<NavMeshDocument> document = NavMeshDocument::Create();
    document->ResetDescription();
    document->description.name = asset.GetFile().AsCharPtr();
    const Util::Blob data = Serialize(document->description);

    IO::CreateDirectory(IO::URI(workFile.LocalPath().ExtractDirName()));
    Ptr<IO::Stream> stream = IO::CreateStream(workFile);
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (!stream->Open())
    {
        outError = Util::Format("could not write %s", workFile.LocalPath().AsCharPtr());
        return false;
    }
    stream->Write(data.GetPtr(), data.Size());
    stream->Close();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
NavMeshDocument::OpenAgentKind()
{
    for (NavMeshDocument* document : OpenDocuments)
    {
        if (document->description.agent_kind != nullptr && !document->description.agent_kind->name.empty())
            return document->description.agent_kind->name.c_str();
    }
    return Util::String();
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceId
NavMeshDocument::GetPreview() const
{
    return this->preview;
}

//------------------------------------------------------------------------------
/**
*/
bool
NavMeshDocument::CanSave() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
NavMeshDocument::ResetDescription()
{
    this->description = Navigation::NavMeshT();
    this->description.agent_kind = std::make_unique<Navigation::AgentT>();
    this->description.agent_kind->name = "default";
    this->description.agent_kind->agent_radius = 0.5f;
    this->description.agent_kind->agent_height = 2.0f;
    this->description.agent_kind->agent_max_climb = 0.5f;
    this->description.navmesh_settings = std::make_unique<Navigation::NavMeshSettingsT>();
}

//------------------------------------------------------------------------------
/**
*/
bool
NavMeshDocument::LoadDescription(Util::String& outError)
{
    const IO::URI uri = this->asset.WorkURI("assets");
    if (!IO::FileExists(uri))
    {
        outError = Util::Format("%s doesn't exist", this->asset.GetWorkFile().AsCharPtr());
        return false;
    }
    Ptr<IO::Stream> stream = IO::CreateStream(uri);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (!stream->Open())
    {
        outError = Util::Format("could not open %s", this->asset.GetWorkFile().AsCharPtr());
        return false;
    }
    bool loaded = false;
    const uint8_t* data = (const uint8_t*)stream->MemoryMap();
    flatbuffers::Verifier verifier(data, (size_t)stream->GetSize());
    if (data != nullptr && verifier.VerifyBuffer<Navigation::NavMesh>(nullptr))
    {
        this->ResetDescription();
        Flat::FlatbufferInterface::DeserializeFlatbuffer<Navigation::NavMesh>(this->description, data);
        if (this->description.agent_kind == nullptr)
            this->description.agent_kind = std::make_unique<Navigation::AgentT>();
        if (this->description.navmesh_settings == nullptr)
            this->description.navmesh_settings = std::make_unique<Navigation::NavMeshSettingsT>();
        loaded = true;
    }
    else
    {
        outError = Util::Format("%s is not a navmesh work file", this->asset.GetWorkFile().AsCharPtr());
    }
    stream->MemoryUnmap();
    stream->Close();
    return loaded;
}

//------------------------------------------------------------------------------
/**
    The description is read right away, it is small. What was baked before is loaded so it shows
*/
bool
NavMeshDocument::OnOpen()
{
    Util::String error;
    if (!this->LoadDescription(error))
    {
        this->SetFailed(error);
        return true;
    }
    this->committed = Serialize(this->description);
    if (IO::FileExists(this->asset.ExportURI()))
        this->preview = Resources::CreateResource(this->asset, "editor", nullptr, nullptr, { .immediate = true });
    this->SetReady();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
NavMeshDocument::OnClose()
{
    this->DiscardPreview();
}

//------------------------------------------------------------------------------
/**
*/
bool
NavMeshDocument::OnSave(Util::String& outError)
{
    if (!IsValidName(this->asset.GetFile()))
    {
        outError = Util::Format("'%s' is not a valid name, use letters, digits, '_' and '-'", this->asset.GetFile().AsCharPtr());
        return false;
    }
    this->description.name = this->asset.GetFile().AsCharPtr();

    const IO::URI uri = this->asset.WorkURI("assets");
    IO::CreateDirectory(IO::URI(uri.LocalPath().ExtractDirName()));
    const Util::Blob data = Serialize(this->description);
    Ptr<IO::Stream> stream = IO::CreateStream(uri);
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (!stream->Open())
    {
        outError = Util::Format("could not write %s", uri.LocalPath().AsCharPtr());
        return false;
    }
    stream->Write(data.GetPtr(), data.Size());
    stream->Close();
    this->committed = data;

    // the asset document batches it, which is the bake
    this->baking = true;
    this->SetStatus(false, Util::Format("Baking %s", this->asset.AsString().AsCharPtr()));
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
NavMeshDocument::OnRevert()
{
    Util::String error;
    if (this->LoadDescription(error))
        this->committed = Serialize(this->description);
    this->skipped.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
NavMeshDocument::ApplyDescription(const Util::Blob& data)
{
    this->ResetDescription();
    Flat::FlatbufferInterface::DeserializeFlatbuffer<Navigation::NavMesh>(this->description, (const uint8_t*)data.GetPtr());
    if (this->description.agent_kind == nullptr)
        this->description.agent_kind = std::make_unique<Navigation::AgentT>();
    if (this->description.navmesh_settings == nullptr)
        this->description.navmesh_settings = std::make_unique<Navigation::NavMeshSettingsT>();
    this->committed = data;
}

//------------------------------------------------------------------------------
/**
    A change only becomes a command when it differs from what the last one left, so a widget that
    reports a change of nothing makes no entry
*/
void
NavMeshDocument::Commit(const char* name)
{
    const Util::Blob current = Serialize(this->description);
    if (current == this->committed)
        return;

    Ptr<NavMeshEditCommand> command = NavMeshEditCommand::Create();
    command->document = this;
    command->name = name;
    command->before = this->committed;
    command->after = current;
    this->commands->Execute(command.upcast<Editor::EditCommand>());
}

//------------------------------------------------------------------------------
/**
*/
void
NavMeshDocument::SetStatus(bool error, const Util::String& status)
{
    this->status = status;
    this->statusIsError = error;
}

//------------------------------------------------------------------------------
/**
*/
void
NavMeshDocument::DiscardPreview()
{
    if (this->preview != Resources::InvalidResourceId)
    {
        Resources::DiscardResource(this->preview);
        this->preview = Resources::InvalidResourceId;
    }
}

//------------------------------------------------------------------------------
/**
    A loaded navmesh is reloaded by the batcher itself, a first bake is loaded here
*/
void
NavMeshDocument::UpdatePreview()
{
    if (!this->baking || Editor::LiveBatcher::IsBusy())
        return;
    this->baking = false;
    if (!IO::FileExists(this->asset.ExportURI()))
    {
        this->SetStatus(true, Util::Format("Baking %s failed, see the batcher's log", this->asset.AsString().AsCharPtr()));
        return;
    }
    // a failed load is never reloaded, it is loaded again from scratch
    if (this->preview != Resources::InvalidResourceId && Resources::ResourceServer::Instance()->GetState(this->preview) == Resources::Resource::Failed)
        this->DiscardPreview();
    if (this->preview == Resources::InvalidResourceId)
        this->preview = Resources::CreateResource(this->asset, "editor", nullptr, nullptr, { .immediate = true });
    this->SetStatus(false, Util::Format("Baked %s", this->asset.AsString().AsCharPtr()));
}

//------------------------------------------------------------------------------
/**
*/
void
NavMeshDocument::GatherEntities(const Util::Array<Game::Entity>& entities, bool replaceAll)
{
    Editor::Scene* scene = this->shell->GetScene();
    std::vector<std::unique_ptr<Navigation::GeometryEntryT>> gathered;
    Util::Array<Util::String> gatheredGuids;
    Util::String lastGathered;
    this->skipped.Clear();
    for (const Game::Entity& entity : entities)
    {
        Util::String reason;
        const Util::String name = EntityName(scene, entity);
        const size_t before = gathered.size();
        if (GatherNavMeshSources(scene, entity, this->options, gathered, reason))
        {
            gatheredGuids.Append(scene->GetEditable(entity).guid.AsString());
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
            this->skipped.Append({ name, reason });
        }
    }
    gathered.erase(std::remove(gathered.begin(), gathered.end(), nullptr), gathered.end());

    if (this->skipped.Size() == 1 && entities.Size() == 1)
        this->SetStatus(true, Util::Format("%s was skipped: %s", this->skipped[0].name.AsCharPtr(), this->skipped[0].reason.AsCharPtr()));
    else if (!this->skipped.IsEmpty())
        this->SetStatus(true, Util::Format("%d of %d entities were skipped, see the list below", this->skipped.Size(), entities.Size()));
    else if (entities.Size() == 1)
        this->SetStatus(false, Util::Format("Added %s, %d sources", lastGathered.AsCharPtr(), (int)gathered.size()));
    else
        this->SetStatus(false, Util::Format("Added %d entities, %d sources", entities.Size(), (int)gathered.size()));

    auto& sources = this->description.sources;
    if (replaceAll)
    {
        sources.clear();
    }
    else
    {
        // what the gathered entities gave before goes, so gathering again doesn't add them twice
        sources.erase(std::remove_if(sources.begin(), sources.end(),
            [&](const std::unique_ptr<Navigation::GeometryEntryT>& entry)
            {
                return !entry->entity_guid.empty() && gatheredGuids.FindIndex(entry->entity_guid.c_str()) != InvalidIndex;
            }), sources.end());
    }
    for (auto& entry : gathered)
        sources.push_back(std::move(entry));
    this->Commit("Gather navmesh sources");
}

//------------------------------------------------------------------------------
/**
    Entities which are gone are dropped, sources placed by hand stay
*/
void
NavMeshDocument::GatherAgain()
{
    Editor::Scene* scene = this->shell->GetScene();
    Util::Array<Util::String> guids;
    for (const auto& entry : this->description.sources)
    {
        if (!entry->entity_guid.empty() && guids.FindIndex(entry->entity_guid.c_str()) == InvalidIndex)
            guids.Append(entry->entity_guid.c_str());
    }

    Util::Array<Game::Entity> entities;
    SizeT missing = 0;
    for (const Util::String& guid : guids)
    {
        const Game::Entity entity = FindEntityByGuid(scene, Util::Guid::FromString(guid));
        if (entity == Game::Entity::Invalid())
            missing++;
        else
            entities.Append(entity);
    }

    auto& sources = this->description.sources;
    sources.erase(std::remove_if(sources.begin(), sources.end(),
        [](const std::unique_ptr<Navigation::GeometryEntryT>& entry)
        {
            return !entry->entity_guid.empty();
        }), sources.end());
    this->GatherEntities(entities, false);
    if (missing > 0)
        this->skipped.Append({ Util::Format("%d entities", missing), "they are no longer in the scene" });
}

//------------------------------------------------------------------------------
/**
    Area volumes are not chosen, every NavArea of the scene is part of every navmesh. Volumes
    outside of a navmesh don't touch it
*/
void
NavMeshDocument::GatherAreaVolumes()
{
    Editor::Scene* scene = this->shell->GetScene();
    auto& modifiers = this->description.area_modifiers;
    modifiers.erase(std::remove_if(modifiers.begin(), modifiers.end(),
        [](const std::unique_ptr<Navigation::NavAreaModifierT>& modifier)
        {
            return !modifier->entity_guid.empty();
        }), modifiers.end());
    for (const NavAreaVolume& volume : GetNavAreaVolumes(scene))
        modifiers.push_back(MakeAreaModifier(scene, volume));
}

//------------------------------------------------------------------------------
/**
    Saves when there is something to save, the save batches it, and bakes what is there otherwise
*/
bool
NavMeshDocument::Bake()
{
    if (this->description.sources.empty())
    {
        this->SetStatus(true, "The navmesh has no sources, select static models or colliders and add them");
        return false;
    }
    this->GatherAreaVolumes();

    const Navigation::Build::AreaTable& table = GetAreaTable();
    for (const auto& modifier : this->description.area_modifiers)
    {
        if (!modifier->area.empty() && table.Find(modifier->area.c_str()) == nullptr)
        {
            this->SetStatus(true, Util::Format("An area volume uses '%s', which isn't in %s", modifier->area.c_str(), Navigation::Build::AreaTableSource));
            return false;
        }
    }
    this->Commit("Gather area volumes");

    if (this->IsDirty())
        return this->Save();

    Editor::LiveBatcher::BatchFile(this->asset);
    this->baking = true;
    this->SetStatus(false, Util::Format("Baking %s", this->asset.AsString().AsCharPtr()));
    return true;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
NavMeshDocument::FindNavMeshEntity() const
{
    Editor::Scene* scene = this->shell->GetScene();
    if (scene == nullptr)
        return Game::Entity::Invalid();
    const Util::StringAtom urn = this->asset.AsString();
    Game::World* world = scene->GetAuthoringWorld();
    Game::Filter filter = Game::FilterBuilder().Including<const Game::Entity, const NavigationFeature::NavMesh>().Build();
    Game::Dataset dataset = world->Query(filter);
    Game::Entity found = Game::Entity::Invalid();
    for (IndexT v = 0; v < dataset.numViews && found == Game::Entity::Invalid(); v++)
    {
        Game::Dataset::View const* view = dataset.views + v;
        for (IndexT i = 0; i < view->numInstances; i++)
        {
            if (((const NavigationFeature::NavMesh*)view->buffers[1] + i)->resource == urn)
            {
                found = *((const Game::Entity*)view->buffers[0] + i);
                break;
            }
        }
    }
    Game::DestroyFilter(filter);
    return found;
}

//------------------------------------------------------------------------------
/**
*/
void
NavMeshDocument::PlaceInScene()
{
    Ptr<Editor::Scene> scene = this->shell->GetScene();
    const Game::Entity existing = this->FindNavMeshEntity();
    Util::Array<Game::Entity> selection;
    if (existing != Game::Entity::Invalid())
    {
        selection.Append(existing);
        this->shell->GetSelection()->SetSelection(selection);
        this->SetStatus(false, Util::Format("%s is already in the scene", this->asset.AsString().AsCharPtr()));
        return;
    }

    Ptr<Editor::EditCommandManager> manager = scene->GetCommandManager();
    manager->BeginMacro("Place navmesh");
    Ptr<Editor::CreateEntityCommand> create = Editor::CreateEntityCommand::Create();
    create->scene = scene;
    create->record.name = scene->UniqueEntityName(Util::String("NavMesh ") + this->asset.GetFile());
    create->record.collection = scene->GetActiveCollection();
    manager->Execute(create);
    if (scene->HasInstance(create->record.entity))
    {
        NavigationFeature::NavMesh navMesh;
        navMesh.resource = this->asset.AsString();
        Ptr<Editor::AddComponentCommand> add = Editor::AddComponentCommand::Create();
        add->scene = scene;
        add->entity = create->record.entity;
        add->component = Game::GetComponentId<NavigationFeature::NavMesh>();
        add->value = Util::Blob(&navMesh, sizeof(navMesh));
        manager->Execute(add);
        selection.Append(create->record.entity);
        this->shell->GetSelection()->SetSelection(selection);
    }
    manager->EndMacro();
    this->SetStatus(false, Util::Format("Placed %s in the scene", this->asset.AsString().AsCharPtr()));
}

//------------------------------------------------------------------------------
/**
*/
void
NavMeshDocument::Draw()
{
    this->UpdatePreview();

    ImGui::TextDisabled("%s", this->asset.AsString().AsCharPtr());
    this->DrawSettings();
    this->DrawSources();
    ImGui::Separator();
    this->DrawBake();

    // The widgets write the description directly, and what they wrote becomes a command once no
    // widget is held, so a drag or a name typed in is one change
    if (!ImGui::IsAnyItemActive())
        this->Commit("Edit navmesh");
}

//------------------------------------------------------------------------------
/**
*/
void
NavMeshDocument::DrawSettings()
{
    Navigation::AgentT& agent = *this->description.agent_kind;
    Navigation::NavMeshSettingsT& settings = *this->description.navmesh_settings;

    if (ImGui::CollapsingHeader("Agent", ImGuiTreeNodeFlags_DefaultOpen))
    {
        char agentName[64];
        Util::String(agent.name.c_str()).CopyToBuffer(agentName, sizeof(agentName));
        if (ImGui::InputText("Name##agent", agentName, sizeof(agentName)))
            agent.name = agentName;
        ImGui::SetItemTooltip("The agent kind the navmesh is for. A NavAgent of that kind walks it");
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
            settings = Navigation::NavMeshSettingsT();
            this->Commit("Reset navmesh settings");
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
NavMeshDocument::DrawSources()
{
    if (!ImGui::CollapsingHeader("Sources", ImGuiTreeNodeFlags_DefaultOpen))
        return;
    Editor::Scene* scene = this->shell->GetScene();

    ImGui::Checkbox("Only static entities", &this->options.onlyStatic);
    ImGui::SetItemTooltip("Use entities marked Static, or below a static body");
    ImGui::SameLine();
    ImGui::Checkbox("Models", &this->options.models);
    ImGui::SameLine();
    ImGui::Checkbox("Colliders", &this->options.colliders);

    SizeT meshes = 0, heightFields = 0;
    Util::Array<Util::String> groups;
    for (const auto& entry : this->description.sources)
    {
        if (entry->geometry.AsMeshSource() != nullptr)
            meshes++;
        else if (entry->geometry.AsHeightFieldSource() != nullptr)
            heightFields++;
        if (groups.FindIndex(entry->entity_guid.c_str()) == InvalidIndex)
            groups.Append(entry->entity_guid.c_str());
    }
    SizeT entities = 0;
    for (const Util::String& guid : groups)
        entities += guid.IsEmpty() ? 0 : 1;
    ImGui::Text("%d meshes and %d height fields from %d entities", (int)meshes, (int)heightFields, (int)entities);

    const Util::Array<Game::Entity> selection = this->shell->GetSelection()->GetSelection();
    ImGui::BeginDisabled(selection.IsEmpty());
    if (ImGui::Button(Util::Format("Set from selection (%d)", selection.Size()).AsCharPtr()))
        this->GatherEntities(selection, true);
    ImGui::SetItemTooltip("Replace the sources with the selected entities");
    ImGui::SameLine();
    if (ImGui::Button("Add selection"))
        this->GatherEntities(selection, false);
    ImGui::SetItemTooltip("Add the selected entities, or update them if they are already sources");
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(entities == 0);
    if (ImGui::Button("Gather again"))
        this->GatherAgain();
    ImGui::SetItemTooltip("Gather the sources of the same entities again, after they moved or changed");
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(this->description.sources.empty());
    if (ImGui::Button("Clear##sources"))
    {
        this->description.sources.clear();
        this->skipped.Clear();
        this->Commit("Clear navmesh sources");
    }
    ImGui::EndDisabled();

    if (!this->description.sources.empty() && ImGui::TreeNodeEx("##baked", ImGuiTreeNodeFlags_DefaultOpen, "Geometry being baked"))
    {
        // grouped by the entity each source was gathered from, sources without one were placed by hand
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
                const Game::Entity entity = FindEntityByGuid(scene, Util::Guid::FromString(guid));
                name = entity != Game::Entity::Invalid() ? EntityName(scene, entity) : Util::String("(not in the scene)");
            }
            int count = 0;
            for (const auto& entry : this->description.sources)
                count += guid == entry->entity_guid.c_str() ? 1 : 0;

            ImGui::PushID(g);
            if (ImGui::SmallButton(Glyph("trash-2")))
            {
                removeGroup = guid;
                removeRequested = true;
            }
            ImGui::SetItemTooltip("Remove from the navmesh");
            ImGui::SameLine();
            if (ImGui::TreeNode("##group", "%s (%d)", name.AsCharPtr(), count))
            {
                for (const auto& entry : this->description.sources)
                {
                    if (!(guid == entry->entity_guid.c_str()))
                        continue;
                    const Math::vec4& p = entry->transform.position();
                    if (const Navigation::MeshSourceT* mesh = entry->geometry.AsMeshSource())
                    {
                        ImGui::BulletText("%s", mesh->resource.c_str());
                        ImGui::SameLine();
                        ImGui::TextDisabled("mesh %u, group %u, at %.1f %.1f %.1f", mesh->mesh_index, mesh->prim_group, p.x, p.y, p.z);
                    }
                    else if (const Navigation::HeightFieldSourceT* field = entry->geometry.AsHeightFieldSource())
                    {
                        ImGui::BulletText("%s", field->resource.c_str());
                        ImGui::SameLine();
                        ImGui::TextDisabled("height field %.1f x %.1f m, %.1f m high, at %.1f %.1f %.1f", field->size_x, field->size_z, field->height_range, p.x, p.y, p.z);
                    }
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        if (removeRequested)
        {
            auto& sources = this->description.sources;
            sources.erase(std::remove_if(sources.begin(), sources.end(),
                [&](const std::unique_ptr<Navigation::GeometryEntryT>& entry)
                {
                    return removeGroup == entry->entity_guid.c_str();
                }), sources.end());
            this->Commit("Remove navmesh sources");
        }
        ImGui::TreePop();
    }

    // the volumes are gathered on every bake, the scene is what is listed
    const Util::Array<NavAreaVolume> volumes = GetNavAreaVolumes(scene);
    if (ImGui::TreeNode("##volumes", "Area volumes (%d)", volumes.Size()))
    {
        if (volumes.IsEmpty())
            ImGui::TextDisabled("Create them in the navigation panel's area table");
        const Navigation::Build::AreaTable& table = GetAreaTable();
        for (const NavAreaVolume& volume : volumes)
        {
            const Navigation::Build::AreaDefinition* area = table.Find(volume.area.Value());
            const Util::String name = EntityName(scene, volume.entity);
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

    if (!this->skipped.IsEmpty() && ImGui::TreeNode("##skipped", "%d entities were skipped", this->skipped.Size()))
    {
        for (const SkippedEntity& entity : this->skipped)
            ImGui::BulletText("%s: %s", entity.name.AsCharPtr(), entity.reason.AsCharPtr());
        ImGui::TreePop();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
NavMeshDocument::DrawBake()
{
    ImGui::BeginDisabled(this->baking);
    if (ImGui::Button(this->baking ? "Baking..." : "Save and bake"))
        this->Bake();
    ImGui::EndDisabled();
    ImGui::SameLine();

    const bool baked = IO::FileExists(this->asset.ExportURI());
    const bool inScene = baked && this->FindNavMeshEntity() != Game::Entity::Invalid();
    ImGui::BeginDisabled(this->baking || !baked);
    if (ImGui::Button(inScene ? "Select in scene" : "Place in scene"))
        this->PlaceInScene();
    ImGui::EndDisabled();
    ImGui::SetItemTooltip(inScene ? "Select the entity which loads this navmesh in the scene" :
        baked ? "Add an entity with a NavMesh component to the scene, which loads this navmesh in the game" :
        "Bake the navmesh first");

    if (this->status.IsValid())
    {
        if (this->statusIsError)
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", this->status.AsCharPtr());
        else
            ImGui::TextDisabled("%s", this->status.AsCharPtr());
    }
    ImGui::TextDisabled("The navigation panel shows the navmesh in the viewport and tests paths on it");
}

} // namespace NavigationFeature
#endif // WITH_NEBULA_EDITOR
