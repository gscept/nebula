//------------------------------------------------------------------------------
//  @file terraineditor.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "graphicsfeature/editor/terrain/terraineditor.h"

#if WITH_NEBULA_EDITOR
#include "graphicsfeature/components/terrain.h"
#include "graphicsfeature/terrain/terrainfile.h"
#include "graphicsfeature/editor/graphicseditor.h"
#include "editorassets/assetfield.h"
#include "neweditor/api/editorfeature.h"
#include "neweditor/api/shell.h"
#include "neweditor/console/editorcommands.h"
#include "neweditor/model/scene.h"
#include "neweditor/model/selectioncontext.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/commands/addcomponentcommand.h"
#include "neweditor/viewtools/toolview.h"
#include "memdb/attributeregistry.h"
#include "dynui/im3d/im3dcontext.h"
#include "dynui/imguicontext.h"
#include "imgui.h"

namespace GraphicsFeature
{

__ImplementClass(GraphicsFeature::TerrainPanel, 'TRNP', Editor::EditorPanel);

TerrainPanel* TerrainPanel::instance = nullptr;

namespace
{

/// every terrain edited in an open scene
Util::Array<Ptr<TerrainSession>> Sessions;
/// the previews of the heightmap and of the mask being painted
Ids::Id32 HeightPreview = Ids::InvalidId32;
Ids::Id32 MaskPreview = Ids::InvalidId32;

const char* ToolNames[TerrainBrush::NumTools] = { "Raise", "Lower", "Flatten", "Smooth", "Paint", "Erase" };

//------------------------------------------------------------------------------
/**
*/
void
CopyText(char* to, const char* from, size_t size)
{
    strncpy(to, from, size - 1);
    to[size - 1] = '\0';
}

//------------------------------------------------------------------------------
/**
*/
Editor::Shell*
ActiveShell()
{
    return Editor::EditorFeatureRegistry::GetShell();
}

//------------------------------------------------------------------------------
/**
*/
bool
IsSculpt(int tool)
{
    return tool == TerrainBrush::Raise || tool == TerrainBrush::Lower || tool == TerrainBrush::Flatten || tool == TerrainBrush::Smooth;
}

//------------------------------------------------------------------------------
/**
*/
TerrainSession*
FindSession(Editor::Scene* scene, Game::Entity entity)
{
    for (const Ptr<TerrainSession>& session : Sessions)
    {
        if (session->GetScene() == scene && session->GetEntity() == entity)
            return session.get();
    }
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
DrawPreview(Ids::Id32& id, CoreGraphics::TextureId texture, float size, float rangeMax)
{
    if (texture == CoreGraphics::InvalidTextureId)
        return;
    Dynui::ImguiTextureId info;
    info.nebulaHandle = texture;
    info.splat = 1;
    info.useAlpha = 0;
    info.useRange = rangeMax > 0.0f;
    info.rangeMin = 0.0f;
    info.rangeMax = rangeMax;
    if (id == Ids::InvalidId32)
        id = Dynui::AllocateImguiTextureId(info);
    else
        Dynui::SetImguiTextureIdData(id, info);
    ImGui::Image(ImTextureRef(id), ImVec2(size, size));
}

//------------------------------------------------------------------------------
/**
    A texture URN field for one of the strings of the terrain file
*/
void
TextureField(const char* id, const Util::String& value, const char* fallback, const std::function<void(const IO::Path&)>& onChange, float width)
{
    Editor::AssetField(id, TerrainTexture(value, fallback), "tex", onChange, width);
}

} // namespace

//------------------------------------------------------------------------------
/**
    The panel starts closed, the panels menu opens it
*/
TerrainPanel::TerrainPanel() :
    editing(false),
    paintMap(TerrainSession::Map::Biome),
    paintIndex(0),
    selectedBiome(0),
    selectedPlant(0),
    hover(0.0f),
    hoverValid(false)
{
    this->name = "terrain";
    this->open = false;
    this->saveFile[0] = '\0';
    TerrainPanel::instance = this;
}

//------------------------------------------------------------------------------
/**
*/
TerrainPanel::~TerrainPanel()
{
    if (TerrainPanel::instance == this)
        TerrainPanel::instance = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainPanel::Register()
{
    Editor::EditorFeatureRegistry::RegisterPanel(TerrainPanel::Create());
    Editor::EditorFeatureRegistry::RegisterViewportInput(Util::Delegate<bool(const Editor::ToolView&)>::FromFunction<&TerrainPanel::ViewportInput>());
    // the core calls its popups every frame whatever is shown, the sessions keep the renderer's
    // terrain in step from there
    Editor::EditorFeatureRegistry::RegisterModal(Util::Delegate<void()>::FromFunction<&TerrainPanel::UpdateSessions>());
    Editor::EditorFeatureRegistry::RegisterUnsavedWork(
        Util::Delegate<void(Util::Array<Util::String>&)>::FromFunction<&TerrainPanel::ListUnsaved>(),
        Util::Delegate<bool()>::FromFunction<&TerrainPanel::SaveUnsaved>());
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainPanel::Clear()
{
    for (const Ptr<TerrainSession>& session : Sessions)
        session->Close(false);
    Sessions.Clear();
    if (HeightPreview != Ids::InvalidId32)
        Dynui::DeallocateImguiTextureId(HeightPreview);
    if (MaskPreview != Ids::InvalidId32)
        Dynui::DeallocateImguiTextureId(MaskPreview);
    HeightPreview = MaskPreview = Ids::InvalidId32;
}

//------------------------------------------------------------------------------
/**
*/
bool
TerrainPanel::OnSetup()
{
    this->core->GetCommands().Register("terrain",
        "terrain [status|edit on|off|tool <raise|lower|flatten|smooth|paint|erase>|brush <radius> [hardness] [strength]|biome <n>|plant <n>|stroke <x> <z> [frames]|height <x> <z>|add biome|add plant|apply|save [folder:file:ter]] - the terrain editor, on the selected terrain",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<TerrainPanel, &TerrainPanel::CommandTerrain>(this));
    return true;
}

//------------------------------------------------------------------------------
/**
    A session ends with its scene, with its entity or its Terrain, and when the component is given
    another file. Only the shown scene's are updated, the sim world is that scene's
*/
void
TerrainPanel::UpdateSessions()
{
    Editor::Shell* shell = ActiveShell();
    if (shell == nullptr)
        return;
    const Ptr<Editor::Scene> active = shell->GetScene();
    const Util::Array<Ptr<Editor::Scene>>& scenes = shell->GetScenes();
    for (IndexT i = Sessions.Size() - 1; i >= 0; i--)
    {
        const Ptr<TerrainSession>& session = Sessions[i];
        bool sceneOpen = false;
        for (const Ptr<Editor::Scene>& scene : scenes)
            sceneOpen |= scene.get() == session->GetScene();
        if (!sceneOpen)
        {
            session->Close(false);
            Sessions.EraseIndex(i);
            continue;
        }
        if (session->GetScene() != active.get())
            continue;
        if (!session->Update())
        {
            if (TerrainPanel::instance != nullptr && TerrainPanel::instance->strokeSession == session)
                TerrainPanel::instance->strokeSession = nullptr;
            session->Close(false);
            Sessions.EraseIndex(i);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainPanel::ListUnsaved(Util::Array<Util::String>& out)
{
    for (const Ptr<TerrainSession>& session : Sessions)
    {
        if (!session->IsDirty())
            continue;
        const Util::String file = session->GetFile().IsEmpty() ? session->SuggestFile() : session->GetFile();
        out.Append(Util::Format("Terrain %s", file.AsCharPtr()));
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
TerrainPanel::SaveUnsaved()
{
    for (const Ptr<TerrainSession>& session : Sessions)
    {
        if (!session->IsDirty())
            continue;
        Util::String error;
        if (!session->Save(session->GetFile().IsEmpty() ? session->SuggestFile() : session->GetFile(), error))
        {
            n_warning("Terrain editor: %s\n", error.AsCharPtr());
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
TerrainSession*
TerrainPanel::SelectedSession(bool create)
{
    const Ptr<Editor::Scene> scene = this->core->GetScene();
    if (!scene.isvalid())
        return nullptr;
    const Util::Array<Game::Entity>& selection = this->core->GetSelection()->GetSelection();
    if (selection.Size() != 1)
        return nullptr;
    const Game::Entity entity = selection[0];
    if (!scene->HasInstance(entity) || !scene->GetAuthoringWorld()->HasComponent(entity, Game::GetComponentId<Terrain>()))
        return nullptr;

    TerrainSession* session = FindSession(scene.get(), entity);
    if (session == nullptr && create)
    {
        Ptr<TerrainSession> made = TerrainSession::Create();
        if (!made->Open(scene.get(), entity))
            return nullptr;
        Sessions.Append(made);
        session = made.get();
        // the terrain is built from the session on the next update
        session->Update();
        if (this->saveFile[0] == '\0' || session->GetFile().IsEmpty())
            CopyText(this->saveFile, session->SuggestFile().AsCharPtr(), sizeof(this->saveFile));
        this->selectedBiome = this->selectedPlant = this->paintIndex = 0;
    }
    return session;
}

//------------------------------------------------------------------------------
/**
*/
bool
TerrainPanel::SaveSession(TerrainSession* session)
{
    Util::String error;
    const Util::String target = session->GetFile().IsEmpty() ? Util::String(this->saveFile) : session->GetFile();
    if (!session->Save(target, error))
    {
        this->core->Log(Editor::LogMessageType::N_WARNING, Util::Format("Terrain not saved: %s", error.AsCharPtr()));
        return false;
    }
    this->core->Log(Editor::LogMessageType::N_SYSTEM, Util::Format("Saved terrain %s", session->GetFile().AsCharPtr()));
    this->SetStatus(Util::Format("Saved terrain %s", session->GetFile().AsCharPtr()), 3.0f);
    return true;
}

//------------------------------------------------------------------------------
/**
    Ctrl+S while the panel is the one last worked in saves the terrain
*/
bool
TerrainPanel::OnEditAction(Editor::EditAction action)
{
    if (action != Editor::EditAction::Save)
        return false;
    TerrainSession* session = this->SelectedSession(false);
    if (session == nullptr)
        return false;
    this->SaveSession(session);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainPanel::Draw()
{
    TerrainSession* session = this->SelectedSession(true);
    if (session == nullptr)
    {
        this->editing = false;
        ImGui::TextDisabled("Select an entity with a Terrain to edit it.");
        const Ptr<Editor::Scene> scene = this->core->GetScene();
        if (scene.isvalid() && ImGui::Button("Create a terrain"))
        {
            const Game::Entity entity = this->core->CreateEntity("Terrain", Util::Guid());
            if (entity != Game::Entity::Invalid())
            {
                const Game::ComponentId component = Game::GetComponentId<Terrain>();
                Ptr<Editor::AddComponentCommand> add = Editor::AddComponentCommand::Create();
                add->scene = scene;
                add->entity = entity;
                add->component = component;
                add->value = Util::Blob(MemDb::AttributeRegistry::DefaultValue(component), MemDb::AttributeRegistry::TypeSize(component));
                scene->GetCommandManager()->Execute(add);
            }
        }
        return;
    }

    // the file
    if (session->GetFile().IsEmpty())
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Save as");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-90.0f);
        ImGui::InputText("##savefile", this->saveFile, sizeof(this->saveFile));
        ImGui::SetItemTooltip("folder:file:ter. The maps are saved next to it, the component gets the file");
    }
    else
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s%s", session->GetFile().AsCharPtr(), session->IsDirty() ? " *" : "");
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!session->IsDirty() && !session->GetFile().IsEmpty());
    if (ImGui::Button("Save"))
        this->SaveSession(session);
    ImGui::EndDisabled();

    if (!GraphicsViewportPanel::ShowsFullPipeline())
    {
        ImGui::TextDisabled("The terrain is drawn in the viewport's Rendering view.");
        ImGui::SameLine();
        if (ImGui::SmallButton("Show it"))
            this->core->ExecuteCommand("view rendering");
    }
    if (session->GetGraphicsEntity() == Graphics::InvalidGraphicsEntityId)
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "The terrain isn't set up in the renderer yet");

    ImGui::Separator();
    this->DrawTools(session);
    if (ImGui::CollapsingHeader("Terrain"))
        this->DrawSettings(session);
    if (ImGui::CollapsingHeader("Biomes", ImGuiTreeNodeFlags_DefaultOpen))
        this->DrawBiomes(session);
    if (ImGui::CollapsingHeader("Plants"))
        this->DrawPlants(session);
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainPanel::DrawTools(TerrainSession* session)
{
    ImGui::Checkbox("Edit in the viewport", &this->editing);
    ImGui::SetItemTooltip("Left button paints, shift does the opposite, escape stops");

    for (int tool = 0; tool < TerrainBrush::NumTools; tool++)
    {
        if (tool != 0 && tool != TerrainBrush::Paint)
            ImGui::SameLine();
        if (ImGui::RadioButton(ToolNames[tool], &this->brush.tool, tool))
            this->editing = true;
    }

    if (!IsSculpt(this->brush.tool))
    {
        // what Paint and Erase work on
        const Render::TerrainResourceT& res = session->GetResource();
        Util::String preview = "Nothing";
        if (this->paintMap == TerrainSession::Map::Biome && this->paintIndex < (IndexT)res.biomes.size())
            preview = Util::Format("Biome: %s", res.biomes[this->paintIndex]->name.AsCharPtr());
        else if (this->paintMap == TerrainSession::Map::Plant && this->paintIndex < (IndexT)res.vegetation.size())
            preview = Util::Format("Plant: %s", res.vegetation[this->paintIndex]->name.AsCharPtr());
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::BeginCombo("Mask", preview.AsCharPtr()))
        {
            for (IndexT i = 0; i < (IndexT)res.biomes.size(); i++)
            {
                ImGui::PushID(i);
                if (ImGui::Selectable(Util::Format("Biome: %s", res.biomes[i]->name.AsCharPtr()).AsCharPtr(), this->paintMap == TerrainSession::Map::Biome && this->paintIndex == i))
                {
                    this->paintMap = TerrainSession::Map::Biome;
                    this->paintIndex = i;
                }
                ImGui::PopID();
            }
            for (IndexT i = 0; i < (IndexT)res.vegetation.size(); i++)
            {
                ImGui::PushID(1000 + i);
                if (ImGui::Selectable(Util::Format("Plant: %s", res.vegetation[i]->name.AsCharPtr()).AsCharPtr(), this->paintMap == TerrainSession::Map::Plant && this->paintIndex == i))
                {
                    this->paintMap = TerrainSession::Map::Plant;
                    this->paintIndex = i;
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
    }

    ImGui::SetNextItemWidth(120.0f);
    ImGui::Combo("Shape", &this->brush.shape, "Circle\0Square\0");
    ImGui::SetNextItemWidth(220.0f);
    ImGui::SliderFloat("Radius", &this->brush.radius, 0.5f, 256.0f, "%.1f m", ImGuiSliderFlags_Logarithmic);
    ImGui::SetNextItemWidth(220.0f);
    ImGui::SliderFloat("Hardness", &this->brush.hardness, 0.0f, 0.99f, "%.2f");
    ImGui::SetNextItemWidth(220.0f);
    ImGui::SliderFloat("Strength", &this->brush.strength, 0.01f, 1.0f, "%.2f");
    if (this->brush.tool == TerrainBrush::Raise || this->brush.tool == TerrainBrush::Lower)
    {
        ImGui::SetNextItemWidth(220.0f);
        ImGui::SliderFloat("Rate", &this->brush.rate, 0.1f, 100.0f, "%.1f m/s", ImGuiSliderFlags_Logarithmic);
        ImGui::SetItemTooltip("How fast the ground moves at full strength");
    }

    // the map the brush works on
    if (IsSculpt(this->brush.tool))
        DrawPreview(HeightPreview, session->GetMapTexture(TerrainSession::Map::Height, 0), 128.0f, 1.0f);
    else
        DrawPreview(MaskPreview, session->GetMapTexture(this->paintMap, this->paintIndex), 128.0f, 0.0f);
    if (this->hoverValid)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("%.1f, %.1f, %.1f", this->hover.x, this->hover.y, this->hover.z);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainPanel::DrawSettings(TerrainSession* session)
{
    Render::TerrainResourceT& res = session->GetResource();
    ImGui::SetNextItemWidth(220.0f);
    ImGui::DragFloatRange2("Height", &res.min_height, &res.max_height, 0.5f, -4096.0f, 4096.0f, "Min %.1f m", "Max %.1f m");
    ImGui::SetItemTooltip("The heightmap goes from the lowest to the highest point, painted heights keep their share of it");
    float size[2] = { res.world_size_x, res.world_size_z };
    ImGui::SetNextItemWidth(220.0f);
    ImGui::DragFloat2("World size", size, 16.0f, 64.0f, 8192.0f, "%.0f m");
    res.world_size_x = size[0];
    res.world_size_z = size[1];
    float tile[2] = { res.tile_width, res.tile_height };
    ImGui::SetNextItemWidth(220.0f);
    ImGui::DragFloat2("Tile size", tile, 1.0f, 16.0f, 512.0f, "%.0f m");
    res.tile_width = tile[0];
    res.tile_height = tile[1];
    float quads[2] = { res.quads_per_tile_x, res.quads_per_tile_z };
    ImGui::SetNextItemWidth(220.0f);
    ImGui::DragFloat2("Quads per tile", quads, 1.0f, 1.0f, 128.0f, "%.0f");
    res.quads_per_tile_x = quads[0];
    res.quads_per_tile_z = quads[1];
    TextureField("decisionmap", res.decision_map, "system:white:tex", [session](const IO::Path& path)
    {
        session->GetResource().decision_map = path.AsString();
        session->ApplyStructure();
    }, 320.0f);
    if (ImGui::Button("Apply"))
        session->ApplyStructure();
    ImGui::SameLine();
    ImGui::TextDisabled("builds the terrain again, a new size resamples the maps");
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainPanel::DrawBiomes(TerrainSession* session)
{
    Render::TerrainResourceT& res = session->GetResource();
    if (ImGui::SmallButton("Add biome"))
    {
        session->AddBiome();
        this->selectedBiome = (IndexT)res.biomes.size() - 1;
    }
    if (res.biomes.empty())
        return;
    this->selectedBiome = Math::clamp(this->selectedBiome, 0, (IndexT)res.biomes.size() - 1);
    for (IndexT i = 0; i < (IndexT)res.biomes.size(); i++)
    {
        ImGui::SameLine();
        ImGui::PushID(i);
        if (ImGui::RadioButton(res.biomes[i]->name.AsCharPtr(), this->selectedBiome == i))
            this->selectedBiome = i;
        ImGui::PopID();
    }

    const IndexT b = this->selectedBiome;
    Render::BiomeT& biome = *res.biomes[b];
    ImGui::PushID("biome");
    char name[64];
    CopyText(name, biome.name.AsCharPtr(), sizeof(name));
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::InputText("Name", name, sizeof(name)))
    {
        biome.name = name;
        session->MarkDirty();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Paint its mask"))
    {
        this->brush.tool = TerrainBrush::Paint;
        this->paintMap = TerrainSession::Map::Biome;
        this->paintIndex = b;
        this->editing = true;
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(res.biomes.size() <= 1);
    if (ImGui::SmallButton("Remove"))
    {
        session->RemoveBiome(b);
        ImGui::EndDisabled();
        ImGui::PopID();
        return;
    }
    ImGui::EndDisabled();

    bool rules = false;
    ImGui::SetNextItemWidth(220.0f);
    rules |= ImGui::SliderFloat("Slope threshold", &biome.slope_threshold, 0.0f, 1.0f);
    ImGui::SetNextItemWidth(220.0f);
    rules |= ImGui::SliderFloat("Height threshold", &biome.height_threshold, 0.0f, 1.0f);
    ImGui::SetNextItemWidth(220.0f);
    rules |= ImGui::SliderFloat("UV scale", &biome.uv_scale_factor, 1.0f, 256.0f, "%.1f", ImGuiSliderFlags_Logarithmic);
    if (rules)
        session->ApplyBiome(b);

    static const char* Layers[4] = { "Flat", "Slope", "Height", "Height and slope" };
    for (int layer = 0; layer < 4 && layer < (int)biome.materials.size(); layer++)
    {
        if (!ImGui::TreeNode(Layers[layer]))
            continue;
        Render::BiomeMaterialT& material = *biome.materials[layer];
        Ptr<TerrainSession> held = session;
        auto set = [held, b, layer](int which, const IO::Path& path)
        {
            Render::TerrainResourceT& r = held->GetResource();
            if (b >= (IndexT)r.biomes.size() || layer >= (int)r.biomes[b]->materials.size())
                return;
            Render::BiomeMaterialT& m = *r.biomes[b]->materials[layer];
            (which == 0 ? m.albedo : which == 1 ? m.normals : m.material) = path.AsString();
            held->ApplyBiome(b);
        };
        ImGui::TextUnformatted("Albedo");
        TextureField("albedo", material.albedo, "system:white:tex", [set](const IO::Path& p) { set(0, p); }, 320.0f);
        ImGui::TextUnformatted("Normals");
        TextureField("normals", material.normals, "system:nobump:tex", [set](const IO::Path& p) { set(1, p); }, 320.0f);
        ImGui::TextUnformatted("Material");
        TextureField("material", material.material, "system:default_material:tex", [set](const IO::Path& p) { set(2, p); }, 320.0f);
        ImGui::TreePop();
    }
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainPanel::DrawPlants(TerrainSession* session)
{
    Render::TerrainResourceT& res = session->GetResource();
    if (ImGui::SmallButton("Add plant"))
    {
        session->AddPlant();
        this->selectedPlant = (IndexT)res.vegetation.size() - 1;
    }
    if (res.vegetation.empty())
    {
        ImGui::TextDisabled("No plants. A plant is a model scattered where its mask is painted");
        return;
    }
    this->selectedPlant = Math::clamp(this->selectedPlant, 0, (IndexT)res.vegetation.size() - 1);
    for (IndexT i = 0; i < (IndexT)res.vegetation.size(); i++)
    {
        ImGui::SameLine();
        ImGui::PushID(i);
        if (ImGui::RadioButton(res.vegetation[i]->name.AsCharPtr(), this->selectedPlant == i))
            this->selectedPlant = i;
        ImGui::PopID();
    }

    const IndexT p = this->selectedPlant;
    Render::VegetationMeshT& plant = *res.vegetation[p];
    ImGui::PushID("plant");
    char name[64];
    CopyText(name, plant.name.AsCharPtr(), sizeof(name));
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::InputText("Name", name, sizeof(name)))
    {
        plant.name = name;
        session->MarkDirty();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Paint its mask"))
    {
        this->brush.tool = TerrainBrush::Paint;
        this->paintMap = TerrainSession::Map::Plant;
        this->paintIndex = p;
        this->editing = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Remove"))
    {
        session->RemovePlant(p);
        ImGui::PopID();
        return;
    }

    ImGui::TextUnformatted("Model");
    Ptr<TerrainSession> held = session;
    Editor::AssetField("model", TerrainModel(plant.model, "system:box:mdl"), "mdl", [held, p](const IO::Path& path)
    {
        Render::TerrainResourceT& r = held->GetResource();
        if (p >= (IndexT)r.vegetation.size())
            return;
        r.vegetation[p]->model = path.AsString();
        held->ApplyPlant(p);
    }, 320.0f);
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::SliderFloat("Density", &plant.density, 0.001f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic))
        session->ApplyPlant(p);
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::SliderFloat("Slope threshold", &plant.slope_threshold, 0.0f, 1.0f))
        session->MarkDirty();
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::SliderFloat("Height threshold", &plant.height_threshold, 0.0f, 1.0f))
        session->MarkDirty();
    if (ImGui::IsItemDeactivatedAfterEdit())
        session->ApplyPlant(p);
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
    The brush follows the ground under the cursor and is drawn on it. It takes the frame from the
    tools while it paints, and the click that starts a stroke, so the selection stays
*/
bool
TerrainPanel::ViewportInput(const Editor::ToolView& view)
{
    TerrainPanel* panel = TerrainPanel::instance;
    if (panel == nullptr || !panel->editing || !panel->IsOpen())
        return false;
    TerrainSession* session = panel->SelectedSession(false);
    if (session == nullptr)
        return false;

    if (view.escape)
    {
        if (panel->strokeSession.isvalid())
            panel->strokeSession->EndStroke();
        panel->strokeSession = nullptr;
        panel->editing = false;
        return false;
    }

    Math::line ray;
    panel->hoverValid = view.hovered && view.CanvasRay(view.mouse, ray) && session->Raycast(ray, panel->hover);

    // the tool the stroke uses, shift does the opposite
    TerrainBrush brush = panel->brush;
    if (view.shift)
    {
        if (brush.tool == TerrainBrush::Raise)
            brush.tool = TerrainBrush::Lower;
        else if (brush.tool == TerrainBrush::Lower)
            brush.tool = TerrainBrush::Raise;
        else if (brush.tool == TerrainBrush::Paint)
            brush.tool = TerrainBrush::Erase;
        else if (brush.tool == TerrainBrush::Erase)
            brush.tool = TerrainBrush::Paint;
    }

    bool taken = false;
    if (!panel->strokeSession.isvalid() && view.pressed && panel->hoverValid && !view.gizmoUsed)
    {
        if (IsSculpt(brush.tool))
            session->BeginStroke(TerrainSession::Map::Height, 0);
        else
            session->BeginStroke(panel->paintMap, panel->paintIndex);
        if (session->IsStroking())
            panel->strokeSession = session;
        taken = true;
    }
    if (panel->strokeSession.isvalid())
    {
        if (view.held && panel->hoverValid)
            panel->strokeSession->Stroke(brush, panel->hover, ImGui::GetIO().DeltaTime);
        if (view.released || !view.held)
        {
            panel->strokeSession->EndStroke();
            panel->strokeSession = nullptr;
        }
        taken = true;
    }

    // the brush, around the point on the ground, a little above it
    if (panel->hoverValid)
    {
        const Math::vec4 color = IsSculpt(brush.tool)
            ? (brush.tool == TerrainBrush::Lower ? Math::vec4(1.0f, 0.5f, 0.3f, 1.0f) : Math::vec4(1.0f, 0.9f, 0.3f, 1.0f))
            : (brush.tool == TerrainBrush::Erase ? Math::vec4(1.0f, 0.4f, 0.4f, 1.0f) : Math::vec4(0.4f, 1.0f, 0.5f, 1.0f));
        const float r = brush.radius;
        const int segments = 64;
        auto ground = [session](float x, float z) -> Math::point
        {
            float h = 0.0f;
            session->HeightAt(x, z, h);
            return Math::point(x, h + 0.25f, z);
        };
        for (int ring = 0; ring < 2; ring++)
        {
            const float radius = ring == 0 ? r : r * Math::clamp(brush.hardness, 0.0f, 0.99f);
            if (radius < 0.25f)
                continue;
            Math::point previous;
            for (int i = 0; i <= segments; i++)
            {
                float x, z;
                if (brush.shape == TerrainBrush::Square)
                {
                    // the square's outline, walked edge by edge
                    const float t = (i % segments) / float(segments) * 4.0f;
                    const int edge = (int)t;
                    const float f = t - edge;
                    const float a = -radius + 2.0f * radius * f;
                    x = edge == 0 ? a : edge == 1 ? radius : edge == 2 ? -a : -radius;
                    z = edge == 0 ? -radius : edge == 1 ? a : edge == 2 ? radius : -a;
                }
                else
                {
                    const float angle = i / float(segments) * 2.0f * PI;
                    x = Math::cos(angle) * radius;
                    z = Math::sin(angle) * radius;
                }
                const Math::point p = ground(panel->hover.x + x, panel->hover.z + z);
                if (i > 0)
                    Im3d::Im3dContext::DrawLine(Math::line(previous, p), ring == 0 ? 2.0f : 1.0f, color, Im3d::AlwaysOnTop);
                previous = p;
            }
        }
        Im3d::Im3dContext::DrawPoint(Math::vec3(panel->hover.x, panel->hover.y + 0.25f, panel->hover.z), 6.0f, color, Im3d::AlwaysOnTop);
    }
    return taken || (view.pressed && panel->hoverValid);
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainPanel::CommandTerrain(const Util::Array<Util::String>& args)
{
    TerrainSession* session = this->SelectedSession(true);
    if (session == nullptr)
    {
        this->core->Log(Editor::LogMessageType::N_WARNING, "terrain: select one entity with a Terrain");
        return;
    }
    const Util::String verb = args.IsEmpty() ? "status" : args[0];
    if (verb == "edit" && args.Size() == 2)
        this->editing = args[1] == "on";
    else if (verb == "tool" && args.Size() == 2)
    {
        for (int tool = 0; tool < TerrainBrush::NumTools; tool++)
        {
            Util::String name = ToolNames[tool];
            name.ToLower();
            if (name == args[1])
                this->brush.tool = tool;
        }
    }
    else if (verb == "brush" && args.Size() >= 2)
    {
        this->brush.radius = args[1].AsFloat();
        if (args.Size() >= 3)
            this->brush.hardness = args[2].AsFloat();
        if (args.Size() >= 4)
            this->brush.strength = args[3].AsFloat();
    }
    else if (verb == "biome" && args.Size() == 2)
    {
        this->paintMap = TerrainSession::Map::Biome;
        this->paintIndex = args[1].AsInt();
    }
    else if (verb == "plant" && args.Size() == 2)
    {
        this->paintMap = TerrainSession::Map::Plant;
        this->paintIndex = args[1].AsInt();
    }
    else if (verb == "stroke" && args.Size() >= 3)
    {
        // a stroke held still for some frames, as the viewport would paint it
        const float x = args[1].AsFloat(), z = args[2].AsFloat();
        const int frames = args.Size() >= 4 ? args[3].AsInt() : 30;
        float h = 0.0f;
        session->HeightAt(x, z, h);
        if (IsSculpt(this->brush.tool))
            session->BeginStroke(TerrainSession::Map::Height, 0);
        else
            session->BeginStroke(this->paintMap, this->paintIndex);
        for (int i = 0; i < frames; i++)
            session->Stroke(this->brush, Math::vec3(x, h, z), 1.0f / 60.0f);
        session->EndStroke();
    }
    else if (verb == "add" && args.Size() == 2 && args[1] == "biome")
        session->AddBiome();
    else if (verb == "add" && args.Size() == 2 && args[1] == "plant")
        session->AddPlant();
    else if (verb == "apply")
        session->ApplyStructure();
    else if (verb == "save")
    {
        if (args.Size() >= 2)
            CopyText(this->saveFile, args[1].AsCharPtr(), sizeof(this->saveFile));
        this->SaveSession(session);
    }
    else if (verb == "height" && args.Size() == 3)
    {
        float h = 0.0f;
        if (session->HeightAt(args[1].AsFloat(), args[2].AsFloat(), h))
            this->core->Log(Editor::LogMessageType::N_SYSTEM, Util::Format("terrain height at %s %s: %.3f", args[1].AsCharPtr(), args[2].AsCharPtr(), h));
        else
            this->core->Log(Editor::LogMessageType::N_WARNING, "terrain height: outside the terrain");
        return;
    }
    else if (verb != "status")
    {
        this->core->Log(Editor::LogMessageType::N_WARNING, "terrain [status|edit on|off|tool <name>|brush <radius> [hardness] [strength]|biome <n>|plant <n>|stroke <x> <z> [frames]|height <x> <z>|add biome|add plant|apply|save [file]]");
        return;
    }

    const Render::TerrainResourceT& res = session->GetResource();
    this->core->Log(Editor::LogMessageType::N_SYSTEM, Util::Format("terrain %s%s: %dx%d m, height %.1f to %.1f, %d biomes, %d plants, tool %s, radius %.1f, %s, renderer %s",
        session->GetFile().IsEmpty() ? "(no file)" : session->GetFile().AsCharPtr(), session->IsDirty() ? " *" : "",
        session->GetMapWidth(), session->GetMapHeight(), res.min_height, res.max_height, (int)res.biomes.size(), (int)res.vegetation.size(),
        ToolNames[this->brush.tool], this->brush.radius, this->editing ? "editing" : "not editing",
        session->GetGraphicsEntity() != Graphics::InvalidGraphicsEntityId ? "attached" : "not attached"));
}

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
