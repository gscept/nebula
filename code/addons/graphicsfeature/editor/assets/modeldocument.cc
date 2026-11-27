//------------------------------------------------------------------------------
//  @file modeldocument.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "graphicsfeature/editor/assets/modeldocument.h"

#if WITH_NEBULA_EDITOR
#include "graphicsfeature/editor/assets/assetpreview.h"
#include "graphicsfeature/editor/assets/clipplayback.h"
#include "graphicsfeature/editor/assets/skeletonview.h"
#include "editorassets/asseteditorpanel.h"
#include "editorassets/assetfield.h"
#include "neweditor/base/iconregistry.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/editcommandmanager.h"
#include "characters/charactercontext.h"
#include "coreanimation/animation.h"
#include "coreanimation/animsamplemask.h"
#include "coregraphics/meshresource.h"
#include "dynui/imguicontext.h"
#include "models/modelcontext.h"
#include "models/nodes/shaderstatenode.h"
#include "resources/resourceserver.h"
#include "visibility/visibilitycontext.h"
#include "io/ioserver.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/model.h"
#include "toolkitutil/asset/assetpackager.h"
#include "imgui.h"

namespace GraphicsFeature
{

__ImplementClass(GraphicsFeature::ModelDocument, 'GMDD', Editor::AssetDocument);

//------------------------------------------------------------------------------
/**
    The parts of a model asset the document edits, as one snapshot
*/
struct ModelEdits
{
    /// the material of each shape and each skin of the scene
    Util::Array<Util::String> shapeMaterials;
    Util::Array<Util::String> skinMaterials;
    std::vector<std::unique_ptr<ToolkitUtil::AnimClipSectionT>> sections;
    std::vector<std::unique_ptr<ToolkitUtil::AnimClipEventT>> events;
    std::vector<std::unique_ptr<ToolkitUtil::JointMaskAssetT>> masks;
};

//------------------------------------------------------------------------------
/**
    What the document holds and shows, apart from the document itself
*/
struct ModelDocumentState
{
    ToolkitUtil::ModelAssetT asset;
    /// the edited parts as the last command left them
    ModelEdits committed;
    /// a command to make once the frame has drawn, nullptr when there is none
    const char* commitName = nullptr;

    Resources::ResourceId model = Resources::InvalidResourceId;
    /// the mesh resource packaged from the asset, for the mesh section
    Resources::ResourceId mesh = Resources::InvalidResourceId;
    bool meshLoaded = false;
    Graphics::GraphicsEntityId preview = Graphics::InvalidGraphicsEntityId;
    /// what the preview's model and character load is tagged with, so the document can let go of it
    Util::StringAtom previewTag;
    bool previewLoaded = false;
    bool framed = false;
    bool shown = false;

    /// a material a field picked, dropped or pasted for a node, made into a command after the frame
    Util::String pendingNode;
    Util::String pendingMaterial;
    bool hasPendingMaterial = false;

    /// draw the skeleton of a skinned model over the preview
    bool showSkeleton = true;
    /// how the clips are played from the animation section
    ClipPlayback playback;
    /// the clip last played from the animation section
    IndexT clip = 0;
    /// the clip section played, its clip is looped within it, InvalidIndex if none is
    IndexT previewSection = InvalidIndex;
    /// an event the preview fired
    struct FiredEvent
    {
        Util::String name;
        Util::String category;
        Util::String clip;
        double time;        ///< ImGui time it fired at
    };
    /// the events the preview fired last, the latest last
    Util::Array<FiredEvent> firedEvents;
    /// the joint hovered in the skeleton section, highlighted in the preview
    IndexT hoveredJoint = InvalidIndex;
    /// the joint mask edited in the skeleton section, its weights are shown in the preview, InvalidIndex if none is
    IndexT editedMask = InvalidIndex;
    /// the joint mask hovered in the list of masks, its weights are shown in the preview instead of the edited one's
    IndexT hoveredMask = InvalidIndex;
    /// the clip played on the second track, mixed over the one played on the first on the joints of the layer mask
    IndexT layerClip = 0;
    /// the joint mask of the asset the layer is mixed with, InvalidIndex for all joints
    IndexT layerMask = InvalidIndex;
    /// the weight the layer is mixed with
    float layerWeight = 1.0f;
    /// the speed the layer was played at, it follows the speed of the clips
    float layerSpeed = 1.0f;
    /// the mask the layer is played with, a weight per joint of the skeleton. It's sized once when the document opens,
    /// the character's jobs read it while it plays, so the weights are only changed in place
    CoreAnimation::AnimSampleMask layerSampleMask;
};

//------------------------------------------------------------------------------
/**
    The parts of the asset edited here, the parts that change are the ones a snapshot holds
*/
class ModelEditCommand : public Editor::EditCommand
{
    __DeclareClass(ModelEditCommand)
public:
    const char* Name() const override { return this->name; }
    bool Execute() override
    {
        this->document->ApplyEdits(this->after);
        return true;
    }
    bool Unexecute() override
    {
        this->document->ApplyEdits(this->before);
        return true;
    }

    Ptr<ModelDocument> document;
    const char* name = "Edit model";
    ModelEdits before;
    ModelEdits after;
};
__ImplementClass(GraphicsFeature::ModelEditCommand, 'GMEC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
    The documents by the preview entity they set up. The model context keeps the callback a setup is
    given for as long as its slot lives, so the callback looks its document up here rather than
    holding it, and a document that closed is gone from here
*/
static Util::Dictionary<Ids::Id32, ModelDocument*> PreviewOwners;

//------------------------------------------------------------------------------
/**
    A glyph from the editor's icon table, a question mark when the table has no such name
*/
static const char*
Glyph(const char* name)
{
    const char* glyph = Editor::IconGlyph(name);
    return glyph != nullptr ? glyph : "?";
}

//------------------------------------------------------------------------------
/**
*/
template<class T>
static void
CopyList(const std::vector<std::unique_ptr<T>>& from, std::vector<std::unique_ptr<T>>& to)
{
    to.clear();
    to.reserve(from.size());
    for (const auto& item : from)
        to.push_back(item != nullptr ? std::make_unique<T>(*item) : nullptr);
}

//------------------------------------------------------------------------------
/**
*/
static void
CaptureEdits(const ToolkitUtil::ModelAssetT& asset, ModelEdits& out)
{
    out.shapeMaterials.Clear();
    out.skinMaterials.Clear();
    if (asset.scene != nullptr)
    {
        for (const auto& shape : asset.scene->shapes)
            out.shapeMaterials.Append(shape != nullptr ? shape->material : Util::String());
        for (const auto& skin : asset.scene->skins)
            out.skinMaterials.Append(skin != nullptr && skin->shape != nullptr ? skin->shape->material : Util::String());
    }
    CopyList(asset.clip_sections, out.sections);
    CopyList(asset.clip_events, out.events);
    CopyList(asset.joint_masks, out.masks);
}

//------------------------------------------------------------------------------
/**
*/
static void
CopyEdits(const ModelEdits& from, ModelEdits& to)
{
    to.shapeMaterials = from.shapeMaterials;
    to.skinMaterials = from.skinMaterials;
    CopyList(from.sections, to.sections);
    CopyList(from.events, to.events);
    CopyList(from.masks, to.masks);
}

//------------------------------------------------------------------------------
/**
    The index of a clip in the animation a character plays, InvalidIndex if it has none of that name
*/
static IndexT
PreviewClipIndex(const Graphics::GraphicsEntityId preview, const Util::String& name)
{
    const CoreAnimation::AnimationId animation = Characters::CharacterContext::GetAnimation(preview);
    if (animation == CoreAnimation::InvalidAnimationId)
        return InvalidIndex;
    const Util::FixedArray<CoreAnimation::AnimClip>& clips = CoreAnimation::AnimGetClips(animation);
    for (IndexT i = 0; i < clips.Size(); i++)
    {
        if (clips[i].GetName().IsValid() && name == clips[i].GetName().Value())
            return i;
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
static const ToolkitUtil::AnimClipT*
FindAssetClip(const ToolkitUtil::AnimResourceT* anim, const Util::String& name)
{
    for (const auto& instance : anim->animations)
        for (const auto& clip : instance->clips)
            if (clip->name == name)
                return clip.get();
    return nullptr;
}

//------------------------------------------------------------------------------
/**
    Keeps the clip played for a section preview within the section, the clip is looped so it plays past its end
*/
static void
UpdateSectionPreview(const Graphics::GraphicsEntityId preview, ModelDocumentState* state)
{
    const auto& sections = state->asset.clip_sections;
    if (state->previewSection == InvalidIndex)
        return;
    if (state->previewSection >= (IndexT)sections.size())
    {
        state->previewSection = InvalidIndex;
        return;
    }
    const ToolkitUtil::AnimClipSectionT& section = *sections[state->previewSection];
    const IndexT playing = Characters::CharacterContext::GetPlayingClip(preview, 0);
    if (playing == InvalidIndex || playing != PreviewClipIndex(preview, section.clip))
    {
        // another clip was played, or it was stopped
        state->previewSection = InvalidIndex;
        return;
    }
    if (Characters::CharacterContext::IsTrackPaused(preview, 0))
        return;
    const Timing::Time time = Characters::CharacterContext::GetTrackTime(preview, 0);
    if (time < Timing::TicksToSeconds(section.start) || time >= Timing::TicksToSeconds(section.end))
        Characters::CharacterContext::SeekTrack(preview, 0, Timing::TicksToSeconds(section.start));
}

//------------------------------------------------------------------------------
/**
    Keep the events the preview fired in the last update, a few of the latest are kept
*/
static void
RecordFiredEvents(const Graphics::GraphicsEntityId preview, ModelDocumentState* state)
{
    const CoreAnimation::AnimationId animation = Characters::CharacterContext::GetAnimation(preview);
    if (animation == CoreAnimation::InvalidAnimationId)
        return;
    const double now = ImGui::GetTime();
    for (const Characters::CharacterContext::FiredEvent& fired : Characters::CharacterContext::GetFiredEvents(preview))
    {
        const Util::StringAtom clip = CoreAnimation::AnimGetClip(animation, fired.clip).GetName();
        state->firedEvents.Append({
            fired.event.name.IsValid() ? fired.event.name.Value() : "",
            fired.event.category.IsValid() ? fired.event.category.Value() : "",
            clip.IsValid() ? clip.Value() : "",
            now });
    }
    const SizeT MaxFired = 8;
    while (state->firedEvents.Size() > MaxFired)
        state->firedEvents.EraseIndex(0);
}

//------------------------------------------------------------------------------
/**
    True if an event of a clip fired within the time a marker flashes for
*/
static bool
FiredRecently(const ModelDocumentState* state, const Util::String& clip, const Util::String& name)
{
    const double now = ImGui::GetTime();
    for (const auto& fired : state->firedEvents)
    {
        if (fired.clip == clip && fired.name == name && now - fired.time < 0.3)
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    The joints of the skeleton of the asset the character plays, the first one. Masks name its joints, their weights
    are in its order once packaged
*/
static const std::vector<std::unique_ptr<ToolkitUtil::JointInstanceT>>*
AssetJoints(const ModelDocumentState* state)
{
    const ToolkitUtil::SkeletonResourceT* skeleton = state->asset.skeleton.get();
    if (skeleton == nullptr || skeleton->skeletons.empty())
        return nullptr;
    return &skeleton->skeletons[0]->joints;
}

//------------------------------------------------------------------------------
/**
    The weight of a joint in a mask, the joints it leaves out weigh 0
*/
static float
JointMaskWeight(const ToolkitUtil::JointMaskAssetT& mask, const Util::String& joint)
{
    for (const auto& weight : mask.joints)
    {
        if (weight->joint == joint)
            return weight->weight;
    }
    return 0.0f;
}

//------------------------------------------------------------------------------
/**
    Joints that weigh 0 are left out of the mask
*/
static void
SetJointMaskWeight(ToolkitUtil::JointMaskAssetT& mask, const Util::String& joint, const float value)
{
    for (auto it = mask.joints.begin(); it != mask.joints.end(); it++)
    {
        if ((*it)->joint == joint)
        {
            if (value <= 0.0f)
                mask.joints.erase(it);
            else
                (*it)->weight = value;
            return;
        }
    }
    if (value > 0.0f)
    {
        auto weight = std::make_unique<ToolkitUtil::JointMaskWeightT>();
        weight->joint = joint;
        weight->weight = value;
        mask.joints.push_back(std::move(weight));
    }
}

//------------------------------------------------------------------------------
/**
    Set the weight of a joint and all joints under it
*/
static void
SetJointMaskSubtree(ToolkitUtil::JointMaskAssetT& mask, const std::vector<std::unique_ptr<ToolkitUtil::JointInstanceT>>& joints, const IndexT joint, const float value)
{
    SetJointMaskWeight(mask, joints[joint]->name, value);
    for (IndexT i = 0; i < (IndexT)joints.size(); i++)
    {
        if (joints[i]->parent == joint && i != joint)
            SetJointMaskSubtree(mask, joints, i, value);
    }
}

//------------------------------------------------------------------------------
/**
    The weights of a mask per joint of the skeleton, like it's packaged
*/
static void
JointMaskWeights(const ToolkitUtil::JointMaskAssetT* mask, const std::vector<std::unique_ptr<ToolkitUtil::JointInstanceT>>& joints, float* outWeights, const SizeT count)
{
    for (IndexT i = 0; i < count; i++)
    {
        if (mask == nullptr)
            outWeights[i] = 1.0f;
        else
            outWeights[i] = i < (IndexT)joints.size() ? Math::clamp(JointMaskWeight(*mask, joints[i]->name), 0.0f, 1.0f) : 0.0f;
    }
}

//------------------------------------------------------------------------------
/**
    A row of the joint weight tree, returns true once an edit is done
*/
static bool
JointMaskNode(ToolkitUtil::JointMaskAssetT& mask, const std::vector<std::unique_ptr<ToolkitUtil::JointInstanceT>>& joints, const IndexT joint, IndexT& hovered)
{
    bool done = false;
    bool hasChildren = false;
    for (IndexT i = 0; i < (IndexT)joints.size() && !hasChildren; i++)
        hasChildren = joints[i]->parent == joint && i != joint;

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
    if (!hasChildren)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    const Util::String& name = joints[joint]->name;
    const bool open = ImGui::TreeNodeEx((void*)(intptr_t)joint, flags, "%s", name.IsEmpty() ? Util::Format("Joint %d", joint).AsCharPtr() : name.AsCharPtr());
    if (ImGui::IsItemHovered())
        hovered = joint;
    ImGui::SetItemTooltip("Right-click to set the joints under it too");

    float weight = JointMaskWeight(mask, name);
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Include it and the joints under it"))
        {
            SetJointMaskSubtree(mask, joints, joint, 1.0f);
            done = true;
        }
        if (ImGui::MenuItem("Leave it and the joints under it out"))
        {
            SetJointMaskSubtree(mask, joints, joint, 0.0f);
            done = true;
        }
        if (ImGui::MenuItem(Util::Format("Set the joints under it to %.2f", weight).AsCharPtr(), nullptr, false, hasChildren))
        {
            SetJointMaskSubtree(mask, joints, joint, weight);
            done = true;
        }
        ImGui::EndPopup();
    }

    ImGui::TableNextColumn();
    ImGui::PushID(joint);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::SliderFloat("##weight", &weight, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
        SetJointMaskWeight(mask, name, weight);
    if (ImGui::IsItemHovered())
        hovered = joint;
    done |= ImGui::IsItemDeactivatedAfterEdit();
    ImGui::PopID();

    if (open && hasChildren)
    {
        for (IndexT i = 0; i < (IndexT)joints.size(); i++)
        {
            if (joints[i]->parent == joint && i != joint)
                done |= JointMaskNode(mask, joints, i, hovered);
        }
        ImGui::TreePop();
    }
    return done;
}

//------------------------------------------------------------------------------
/**
    Show the meshes of a mesh resource with their vertex layout and primitive groups
*/
static void
MeshInfo(const CoreGraphics::MeshResourceId resource)
{
    SizeT numMeshes = CoreGraphics::MeshResourceGetNumMeshes(resource);
    for (IndexT i = 0; i < numMeshes; i++)
    {
        if (ImGui::CollapsingHeader(Util::Format("Mesh %d", i).AsCharPtr(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            CoreGraphics::MeshId mesh = CoreGraphics::MeshResourceGetMesh(resource, i);
            CoreGraphics::MeshIdLock _0(mesh);
            CoreGraphics::VertexLayoutId vlo = CoreGraphics::MeshGetVertexLayout(mesh);
            ImGui::Text("Vertex Format: %s", CoreGraphics::VertexLayoutGetName(vlo).Value());
            const Util::Array<CoreGraphics::PrimitiveGroup>& groups = CoreGraphics::MeshGetPrimitiveGroups(mesh);
            for (IndexT j = 0; j < groups.Size(); j++)
            {
                ImGui::PushFont(Dynui::ImguiBoldFont, 0.0f);
                ImGui::Text("Primitive Group %d", j);
                ImGui::PopFont();
                ImGui::Indent();
                ImGui::Text("Triangles: %d", groups[j].GetNumPrimitives(CoreGraphics::PrimitiveTopology::TriangleList));
                if (groups[j].GetNumIndices() == 0)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(240, 100, 100, 255));
                    ImGui::TextUnformatted("Primitive Group is not reusing vertices, please check the mesh");
                    ImGui::PopStyleColor();
                }
                ImGui::Unindent();
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
ModelDocument::ModelDocument() :
    state(new ModelDocumentState())
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ModelDocument::~ModelDocument()
{
    delete this->state;
}

//------------------------------------------------------------------------------
/**
*/
void
ModelDocument::OpenAsset(const IO::Path& asset)
{
    Ptr<ModelDocument> document = ModelDocument::Create();
    document->asset = asset;
    Editor::AssetEditorPanel::Open(document.upcast<Editor::AssetDocument>());
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelDocument::CanSave() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
    The asset is read whole, the work file is what saving writes back. The model is loaded the way
    the scene loads it, so a material change shows on its instances as well.
*/
bool
ModelDocument::OnOpen()
{
    if (!Resources::ResourceServer::Instance()->HasStreamLoader(this->asset))
        return false;

    ModelDocumentState* state = this->state;
    Ptr<IO::Stream> stream = IO::CreateStream(this->asset.WorkURI("assets"));
    stream->SetAccessMode(IO::Stream::AccessMode::ReadAccess);
    if (!stream->Open())
        return false;
    const void* data = stream->MemoryMap();
    Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::ModelAsset>(state->asset, (const uint8_t*)data);
    stream->MemoryUnmap();
    stream->Close();
    CaptureEdits(state->asset, state->committed);

    // the layer's mask has a weight per joint, it's only changed in place from here on
    const auto* joints = AssetJoints(state);
    state->layerSampleMask.name = "preview";
    state->layerSampleMask.weights.Resize(joints != nullptr ? (SizeT)joints->size() : 0);
    state->layerSampleMask.weights.Fill(1.0f);

    Ptr<ModelDocument> self = this;

    // the mesh packaged from the asset, for the mesh section
    const IO::Path mesh = IO::Path::File(this->asset.GetFolder(), this->asset.GetFile(), "msh");
    if (IO::FileExists(mesh.ExportURI()))
    {
        state->mesh = Resources::CreateResource(mesh, "preview", [self](Resources::ResourceId id)
        {
            if (!self->IsClosed())
                self->state->meshLoaded = true;
        });
    }

    state->model = Resources::CreateResource(this->asset, "editor",
        [self](Resources::ResourceId id)
        {
            if (self->IsClosed())
                return;
            self->state->model = id;
            self->Loaded();
        },
        [self](Resources::ResourceId id)
        {
            self->SetFailed("the model could not be loaded");
        });
    return true;
}

//------------------------------------------------------------------------------
/**
    The preview gets an instance of its own, in the preview's stage. A skinned model gets a character
    once the instance is loaded, the skinned shaders read its joints.
*/
void
ModelDocument::Loaded()
{
    ModelDocumentState* state = this->state;
    state->preview = Graphics::CreateEntity();
    Models::ModelContext::RegisterEntity(state->preview);
    PreviewOwners.Add(state->preview.id, this);

    // the character context does not release what it loads, so the preview's resources get a tag of
    // their own, which the document discards when it closes
    static uint previewCount = 0;
    state->previewTag = Util::Format("assetpreview%u", ++previewCount);

    const Ids::Id32 previewId = state->preview.id;
    Models::ModelContext::Setup(state->preview, this->asset, state->previewTag,
        [previewId]()
        {
            const IndexT owner = PreviewOwners.FindIndex(previewId);
            if (owner != InvalidIndex)
                PreviewOwners.ValueAtIndex(owner)->PreviewLoaded();
        },
        state->shown ? AssetPreview::Stage : 0);
    Models::ModelContext::SetAlwaysVisible(state->preview);
    this->SetReady();
}

//------------------------------------------------------------------------------
/**
*/
void
ModelDocument::PreviewLoaded()
{
    ModelDocumentState* state = this->state;
    state->previewLoaded = true;
    Visibility::ObservableContext::RegisterEntity(state->preview);
    Visibility::ObservableContext::Setup(state->preview, Visibility::VisibilityEntityType::Model);
    Characters::CharacterContext::SetupFromModel(state->preview, this->asset, state->previewTag);
}

//------------------------------------------------------------------------------
/**
    The character goes before the model it animates, the layer's mask it reads goes with the state
    after both.
*/
void
ModelDocument::OnClose()
{
    ModelDocumentState* state = this->state;
    if (state->preview != Graphics::InvalidGraphicsEntityId)
    {
        if (Characters::CharacterContext::IsEntityRegistered(state->preview))
            Characters::CharacterContext::DeregisterEntityImmediate(state->preview);
        if (Visibility::ObservableContext::IsEntityRegistered(state->preview))
            Visibility::ObservableContext::DeregisterEntityImmediate(state->preview);
        if (Models::ModelContext::IsEntityRegistered(state->preview))
            Models::ModelContext::DeregisterEntityImmediate(state->preview);
        const IndexT owner = PreviewOwners.FindIndex(state->preview.id);
        if (owner != InvalidIndex)
            PreviewOwners.EraseAtIndex(owner);
        Graphics::DestroyEntity(state->preview);
        state->preview = Graphics::InvalidGraphicsEntityId;
        Resources::ResourceServer::Instance()->DiscardResources(state->previewTag);
    }
    if (state->mesh != Resources::InvalidResourceId)
    {
        Resources::DiscardResource(state->mesh);
        state->mesh = Resources::InvalidResourceId;
    }
    if (state->model != Resources::InvalidResourceId)
    {
        Resources::DiscardResource(state->model);
        state->model = Resources::InvalidResourceId;
    }
    if (state->shown)
    {
        AssetPreview* preview = AssetPreview::Peek();
        if (preview != nullptr)
            preview->Hide();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ModelDocument::OnShown(bool shown)
{
    ModelDocumentState* state = this->state;
    state->shown = shown;
    if (state->preview != Graphics::InvalidGraphicsEntityId)
        Models::ModelContext::SetStageMask(state->preview, shown ? AssetPreview::Stage : 0);
    if (!shown)
    {
        AssetPreview* preview = AssetPreview::Peek();
        if (preview != nullptr)
            preview->Hide();
    }
}

//------------------------------------------------------------------------------
/**
    A material that differs from what the last command left shows on the model right away. The
    rest is the asset's, it is what saving writes.
*/
void
ModelDocument::ApplyEdits(const ModelEdits& edits)
{
    ModelDocumentState* state = this->state;
    ToolkitUtil::SceneResourceT* scene = state->asset.scene.get();
    if (scene != nullptr)
    {
        for (IndexT i = 0; i < (IndexT)scene->shapes.size() && i < edits.shapeMaterials.Size(); i++)
        {
            auto& shape = scene->shapes[i];
            const Util::String& material = edits.shapeMaterials[i];
            if (shape == nullptr)
                continue;
            if (i < state->committed.shapeMaterials.Size() && state->committed.shapeMaterials[i] != material && shape->transform != nullptr)
                this->ApplyNodeMaterial(shape->transform->name, material);
            shape->material = material;
        }

        // each skin fragment is packaged as a node of its own
        const Util::Array<Util::String> skinNodeNames = ToolkitUtil::GetSkinNodeNames(scene);
        for (IndexT i = 0; i < (IndexT)scene->skins.size() && i < edits.skinMaterials.Size(); i++)
        {
            auto& skin = scene->skins[i];
            const Util::String& material = edits.skinMaterials[i];
            if (skin == nullptr || skin->shape == nullptr)
                continue;
            if (i < state->committed.skinMaterials.Size() && state->committed.skinMaterials[i] != material && i < skinNodeNames.Size())
                this->ApplyNodeMaterial(skinNodeNames[i], material);
            skin->shape->material = material;
        }
    }

    CopyList(edits.sections, state->asset.clip_sections);
    CopyList(edits.events, state->asset.clip_events);
    CopyList(edits.masks, state->asset.joint_masks);
    CopyEdits(edits, state->committed);

    // an index into a list an undo made shorter is dropped
    if (state->previewSection >= (IndexT)state->asset.clip_sections.size())
        state->previewSection = InvalidIndex;
    if (state->editedMask >= (IndexT)state->asset.joint_masks.size())
        state->editedMask = InvalidIndex;
    if (state->layerMask >= (IndexT)state->asset.joint_masks.size())
        state->layerMask = InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    The node of the model resource gets the material, so instances made from now on have it, and the
    instances there are change theirs once it is loaded
*/
void
ModelDocument::ApplyNodeMaterial(const Util::String& node, const Util::String& material)
{
    ModelDocumentState* state = this->state;
    if (state->model == Resources::InvalidResourceId)
        return;

    const Models::ModelId model = state->model;
    const auto& nodeTable = Models::GetModelNodeTable(model);
    const IndexT tableIndex = nodeTable.FindIndex(node);
    if (tableIndex == InvalidIndex)
        return;
    Models::ShaderStateNode* resourceNode = static_cast<Models::ShaderStateNode*>(nodeTable.ValueAtIndex(tableIndex));
    const IO::Path path = IO::Path::Parse(material);
    resourceNode->SetMaterial(path);

    if (!state->previewLoaded)
        return;
    const IndexT nodeIndex = Models::ModelContext::GetNodeIndex(state->preview, node);
    if (nodeIndex == InvalidIndex)
        return;
    Resources::CreateResource(path, "editor", [model, nodeIndex](Resources::ResourceId id)
    {
        Models::ModelContext::ChangeMaterialOnModels(model, nodeIndex, id);
    });
}

//------------------------------------------------------------------------------
/**
*/
void
ModelDocument::Commit(const char* name)
{
    this->state->commitName = name;
}

//------------------------------------------------------------------------------
/**
*/
void
ModelDocument::CommitNow(const char* name)
{
    Ptr<ModelEditCommand> command = ModelEditCommand::Create();
    command->document = this;
    command->name = name;
    CopyEdits(this->state->committed, command->before);
    CaptureEdits(this->state->asset, command->after);
    this->commands->Execute(command.upcast<Editor::EditCommand>());
}

//------------------------------------------------------------------------------
/**
*/
void
ModelDocument::RunPendingMaterial()
{
    ModelDocumentState* state = this->state;
    if (!state->hasPendingMaterial)
        return;
    state->hasPendingMaterial = false;

    ToolkitUtil::SceneResourceT* scene = state->asset.scene.get();
    if (scene == nullptr)
        return;

    bool changed = false;
    for (auto& shape : scene->shapes)
    {
        if (shape != nullptr && shape->transform != nullptr && shape->transform->name == state->pendingNode && shape->material != state->pendingMaterial)
        {
            shape->material = state->pendingMaterial;
            changed = true;
        }
    }
    const Util::Array<Util::String> skinNodeNames = ToolkitUtil::GetSkinNodeNames(scene);
    for (IndexT i = 0; i < (IndexT)scene->skins.size() && i < skinNodeNames.Size(); i++)
    {
        auto& skin = scene->skins[i];
        if (skin != nullptr && skin->shape != nullptr && skinNodeNames[i] == state->pendingNode && skin->shape->material != state->pendingMaterial)
        {
            skin->shape->material = state->pendingMaterial;
            changed = true;
        }
    }
    if (changed)
        this->CommitNow("Set node material");
}

//------------------------------------------------------------------------------
/**
    The material of each node drawing a mesh, followed by the resources packaged from the asset if they're exported:
    the mesh, and for skinned models the skeleton and animation of the character the preview is set up with,
    which are shown in the preview
*/
void
ModelDocument::Draw()
{
    this->RunPendingMaterial();

    ModelDocumentState* state = this->state;
    const bool isCharacter = state->previewLoaded && Characters::CharacterContext::IsEntityRegistered(state->preview);
    const Characters::SkeletonId skeleton = isCharacter ? Characters::CharacterContext::GetSkeleton(state->preview) : Characters::InvalidSkeletonId;

    AssetPreview* preview = AssetPreview::Get();
    if (preview != nullptr && state->previewLoaded && !state->framed)
    {
        // the camera looks at the whole model once it has a size
        const Math::bbox box = Models::ModelContext::ComputeBoundingBox(state->preview);
        if (box.pmin != box.pmax)
        {
            preview->Frame(box);
            state->framed = true;
        }
    }

    // the weights of the joint mask hovered in the list, or else the one edited, are shown on the skeleton
    Util::FixedArray<float> maskWeights;
    const auto* assetJoints = AssetJoints(state);
    const IndexT shownMask = state->hoveredMask != InvalidIndex ? state->hoveredMask : state->editedMask;
    if (assetJoints != nullptr && shownMask >= 0 && shownMask < (IndexT)state->asset.joint_masks.size())
    {
        maskWeights.Resize((SizeT)assetJoints->size());
        JointMaskWeights(state->asset.joint_masks[shownMask].get(), *assetJoints, maskWeights.Begin(), maskWeights.Size());
    }

    // a table cell has no height of its own, so both columns get what is left of the window
    const float height = Math::max(ImGui::GetContentRegionAvail().y, 64.0f);
    if (ImGui::BeginTable("model", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableNextColumn();
        if (preview != nullptr)
        {
            const Graphics::GraphicsEntityId entity = state->preview;
            preview->Draw(ImVec2(ImGui::GetContentRegionAvail().x, height),
                [state, entity, skeleton, &maskWeights](ImDrawList* drawList, const Math::mat4& viewProjection, const ImVec2& min, const ImVec2& max)
                {
                    if (!state->showSkeleton || skeleton == Characters::InvalidSkeletonId)
                        return;
                    SkeletonOverlay(drawList, viewProjection, min, max, skeleton, Characters::CharacterContext::GetJointTransforms(entity),
                        Models::ModelContext::GetTransform(entity), state->hoveredJoint, maskWeights.IsEmpty() ? nullptr : &maskWeights);
                });
        }
        else
        {
            ImGui::TextDisabled("The preview could not be made");
        }

        ImGui::TableNextColumn();
        if (ImGui::BeginChild("sections", ImVec2(0.0f, height)))
        {
            if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen))
                this->DrawMaterials();

            if (state->meshLoaded && ImGui::CollapsingHeader("Mesh"))
                this->DrawMesh();

            state->hoveredJoint = InvalidIndex;
            state->hoveredMask = InvalidIndex;
            if (skeleton != Characters::InvalidSkeletonId && ImGui::CollapsingHeader("Skeleton", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Show in preview", &state->showSkeleton);
                ImGui::SetItemTooltip("Hover a joint to highlight it");
                state->hoveredJoint = SkeletonJointTree(skeleton);
                this->DrawJointMasks();
            }

            if (isCharacter && ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushID("animation");
                UpdateSectionPreview(state->preview, state);
                RecordFiredEvents(state->preview, state);
                Util::Array<ClipMarker> markers;
                for (IndexT i = 0; i < (IndexT)state->asset.clip_sections.size(); i++)
                {
                    const ToolkitUtil::AnimClipSectionT& section = *state->asset.clip_sections[i];
                    markers.Append({ section.clip, section.name, section.start / 1000.0f, section.end / 1000.0f, i == state->previewSection, false, i });
                }
                for (IndexT i = 0; i < (IndexT)state->asset.clip_events.size(); i++)
                {
                    const ToolkitUtil::AnimClipEventT& event = *state->asset.clip_events[i];
                    markers.Append({ event.clip, event.name, event.time / 1000.0f, event.time / 1000.0f, FiredRecently(state, event.clip, event.name), true, i });
                }
                Ptr<ModelDocument> self = this;
                AnimationClips(state->preview, state->playback, state->clip, markers,
                    [self](const ClipMarker& marker, float start, float end, bool done)
                    {
                        // points are events, ranges are sections
                        ModelDocumentState* data = self->state;
                        if (marker.point)
                        {
                            auto& events = data->asset.clip_events;
                            if (marker.id < 0 || marker.id >= (IndexT)events.size())
                                return;
                            events[marker.id]->time = (uint)Math::round(start * 1000.0f);
                        }
                        else
                        {
                            auto& sections = data->asset.clip_sections;
                            if (marker.id < 0 || marker.id >= (IndexT)sections.size())
                                return;
                            sections[marker.id]->start = (uint)Math::round(start * 1000.0f);
                            sections[marker.id]->end = (uint)Math::round(end * 1000.0f);
                        }
                        if (done)
                            self->Commit(marker.point ? "Move clip event" : "Move clip section");
                    });
                this->DrawAnimationLayer();
                this->DrawClipSections();
                this->DrawClipEvents();
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
        ImGui::EndTable();
    }

    // the edits done while the sections drew become one command, now that nothing walks the asset's lists
    if (state->commitName != nullptr)
    {
        const char* name = state->commitName;
        state->commitName = nullptr;
        this->CommitNow(name);
    }
}

//------------------------------------------------------------------------------
/**
    A material field for each node drawing a mesh. A picked material arrives after the frame, so it
    becomes a command when the document draws next.
*/
void
ModelDocument::DrawMaterials()
{
    ModelDocumentState* state = this->state;
    const ToolkitUtil::SceneResourceT* scene = state->asset.scene.get();
    if (scene == nullptr)
    {
        ImGui::TextDisabled("The asset has no scene");
        return;
    }

    Ptr<ModelDocument> self = this;
    auto materialRow = [&self](const Util::String& node, const Util::String& material)
    {
        ImGui::PushID(node.AsCharPtr());
        ImGui::TextUnformatted(node.AsCharPtr());
        Editor::AssetField("material", IO::Path::Parse(material), "mat", [self, node](const IO::Path& picked)
        {
            if (self->IsClosed())
                return;
            self->state->pendingNode = node;
            self->state->pendingMaterial = picked.AsString();
            self->state->hasPendingMaterial = true;
        });
        ImGui::PopID();
    };

    for (const auto& shape : scene->shapes)
    {
        if (shape != nullptr && shape->transform != nullptr)
            materialRow(shape->transform->name, shape->material);
    }
    const Util::Array<Util::String> skinNodeNames = ToolkitUtil::GetSkinNodeNames(scene);
    for (IndexT i = 0; i < (IndexT)scene->skins.size() && i < skinNodeNames.Size(); i++)
    {
        const auto& skin = scene->skins[i];
        if (skin != nullptr && skin->shape != nullptr)
            materialRow(skinNodeNames[i], skin->shape->material);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ModelDocument::DrawMesh()
{
    ImGui::PushID("mesh");
    MeshInfo(CoreGraphics::MeshResourceId(this->state->mesh));
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
    The joint masks of the asset, packaged into the model. The mask edited gets a weight per joint, which the preview
    shows, and which the layer played with it uses right away
*/
void
ModelDocument::DrawJointMasks()
{
    ModelDocumentState* state = this->state;
    const auto* joints = AssetJoints(state);
    if (joints == nullptr)
        return;
    auto& masks = state->asset.joint_masks;

    ImGui::SeparatorText("Joint masks");
    ImGui::TextDisabled("The joints a clip layered over another is mixed on, packaged into the model");

    IndexT erase = InvalidIndex;
    if (!masks.empty() && ImGui::BeginTable("Masks", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed);
        for (IndexT i = 0; i < (IndexT)masks.size(); i++)
        {
            ToolkitUtil::JointMaskAssetT& mask = *masks[i];
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            char name[64];
            mask.name.CopyToBuffer(name, sizeof(name));
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputText("##name", name, sizeof(name)))
                mask.name = name;
            if (ImGui::IsItemDeactivatedAfterEdit())
                this->Commit("Rename joint mask");
            bool rowHovered = ImGui::IsItemHovered();

            // the reason the mask isn't packaged, and the joints it has that the skeleton hasn't
            Util::String error;
            Util::String unknown;
            if (!ToolkitUtil::ValidateJointMask(state->asset.skeleton.get(), masks, i, error))
            {
                ImGui::SameLine(0.0f, 0.0f);
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), " (!)");
                ImGui::SetItemTooltip("It isn't packaged, %s", error.AsCharPtr());
                rowHovered |= ImGui::IsItemHovered();
            }
            for (const auto& weight : mask.joints)
            {
                bool found = false;
                for (const auto& joint : *joints)
                    found |= joint->name == weight->joint;
                if (!found)
                    unknown.Append(Util::Format("\n%s", weight->joint.AsCharPtr()));
            }
            if (!unknown.IsEmpty())
            {
                ImGui::SameLine(0.0f, 0.0f);
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), " (?)");
                ImGui::SetItemTooltip("Joints the skeleton hasn't are left out, edit the mask to remove them:%s", unknown.AsCharPtr());
                rowHovered |= ImGui::IsItemHovered();
            }

            ImGui::TableNextColumn();
            const bool editing = state->editedMask == i;
            if (editing)
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            if (ImGui::Button(Glyph("pencil")))
                state->editedMask = editing ? InvalidIndex : i;
            rowHovered |= ImGui::IsItemHovered();
            if (editing)
                ImGui::PopStyleColor();
            ImGui::SetItemTooltip(editing ? "Stop editing the weights" : "Edit the weights of the joints, they're shown in the preview");
            ImGui::SameLine();
            if (ImGui::Button(Glyph("trash-2")))
                erase = i;
            rowHovered |= ImGui::IsItemHovered();
            ImGui::SetItemTooltip("Remove the mask");
            if (rowHovered)
                state->hoveredMask = i;
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    if (erase != InvalidIndex)
    {
        masks.erase(masks.begin() + erase);
        state->hoveredMask = InvalidIndex;
        auto shift = [erase](IndexT& index)
        {
            if (index == erase)
                index = InvalidIndex;
            else if (index > erase)
                index--;
        };
        shift(state->editedMask);
        shift(state->layerMask);
        this->Commit("Remove joint mask");
    }

    if (ImGui::Button(Util::Format("%s Add mask", Glyph("plus")).AsCharPtr()))
    {
        auto mask = std::make_unique<ToolkitUtil::JointMaskAssetT>();
        for (int n = (int)masks.size() + 1;; n++)
        {
            mask->name = Util::Format("Mask_%d", n);
            bool unique = true;
            for (const auto& other : masks)
                unique &= other->name != mask->name;
            if (unique)
                break;
        }
        masks.push_back(std::move(mask));
        state->editedMask = (IndexT)masks.size() - 1;
        this->Commit("Add joint mask");
    }
    ImGui::SetItemTooltip("Add a mask without joints, right-click a joint to include it and the joints under it");

    if (state->editedMask < 0 || state->editedMask >= (IndexT)masks.size())
    {
        state->editedMask = InvalidIndex;
        return;
    }

    ToolkitUtil::JointMaskAssetT& mask = *masks[state->editedMask];
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Weights of '%s'", mask.name.AsCharPtr());
    bool done = false;

    // shortcuts for all joints of the skeleton, joints it hasn't are kept
    ImGui::SameLine();
    if (ImGui::SmallButton("All 1"))
    {
        for (const auto& joint : *joints)
            SetJointMaskWeight(mask, joint->name, 1.0f);
        done = true;
    }
    ImGui::SetItemTooltip("Include all joints");
    ImGui::SameLine();
    if (ImGui::SmallButton("All 0"))
    {
        for (const auto& joint : *joints)
            SetJointMaskWeight(mask, joint->name, 0.0f);
        done = true;
    }
    ImGui::SetItemTooltip("Leave all joints out");
    if (ImGui::BeginTable("JointWeights", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Joint", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableSetupColumn("Weight", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableHeadersRow();
        for (IndexT i = 0; i < (IndexT)joints->size(); i++)
        {
            const int parent = (*joints)[i]->parent;
            if (parent < 0 || parent >= (int)joints->size() || parent == i)
                done |= JointMaskNode(mask, *joints, i, state->hoveredJoint);
        }
        ImGui::EndTable();
    }

    // the joints the skeleton hasn't, from a reimport that renamed joints
    SizeT numUnknown = 0;
    for (const auto& weight : mask.joints)
    {
        bool found = false;
        for (const auto& joint : *joints)
            found |= joint->name == weight->joint;
        numUnknown += found ? 0 : 1;
    }
    if (numUnknown > 0 && ImGui::Button(Util::Format("Remove the %d joints the skeleton hasn't", numUnknown).AsCharPtr()))
    {
        for (size_t w = mask.joints.size(); w-- > 0;)
        {
            bool found = false;
            for (const auto& joint : *joints)
                found |= joint->name == mask.joints[w]->joint;
            if (!found)
                mask.joints.erase(mask.joints.begin() + w);
        }
        done = true;
    }
    if (done)
        this->Commit("Edit joint mask");
}

//------------------------------------------------------------------------------
/**
    A clip played on the second track over the one the transport plays, mixed on the joints of a mask of the asset.
    The mask is the one edited, so changes show right away without saving
*/
void
ModelDocument::DrawAnimationLayer()
{
    ModelDocumentState* state = this->state;
    const Graphics::GraphicsEntityId preview = state->preview;
    const CoreAnimation::AnimationId animation = Characters::CharacterContext::GetAnimation(preview);
    if (animation == CoreAnimation::InvalidAnimationId)
        return;
    const Util::FixedArray<CoreAnimation::AnimClip>& clips = CoreAnimation::AnimGetClips(animation);
    if (clips.IsEmpty())
        return;
    const auto* joints = AssetJoints(state);
    const auto& masks = state->asset.joint_masks;
    if (state->layerClip < 0 || state->layerClip >= clips.Size())
        state->layerClip = 0;
    if (state->layerMask >= (IndexT)masks.size())
        state->layerMask = InvalidIndex;

    const IndexT LayerTrack = 1;
    const bool playing = Characters::CharacterContext::GetPlayingClip(preview, LayerTrack) != InvalidIndex;

    // the mask is kept up to date in place while the layer plays
    CoreAnimation::AnimSampleMask& sampleMask = state->layerSampleMask;
    if (joints != nullptr)
        JointMaskWeights(state->layerMask != InvalidIndex ? masks[state->layerMask].get() : nullptr, *joints, sampleMask.weights.Begin(), sampleMask.weights.Size());
    else
        JointMaskWeights(nullptr, {}, sampleMask.weights.Begin(), sampleMask.weights.Size());

    ImGui::SeparatorText("Layer");
    ImGui::TextDisabled("A clip mixed over the one played, on the joints of a mask");
    ImGui::PushID("layer");
    bool restart = false;

    auto clipName = [&clips](IndexT i) { return clips[i].GetName().IsValid() ? clips[i].GetName().Value() : ""; };
    const float width = ImGui::GetContentRegionAvail().x;
    ImGui::SetNextItemWidth(width * 0.35f);
    if (ImGui::BeginCombo("##clip", clipName(state->layerClip)))
    {
        for (IndexT i = 0; i < clips.Size(); i++)
        {
            if (ImGui::Selectable(clipName(i), i == state->layerClip))
            {
                restart = playing && i != state->layerClip;
                state->layerClip = i;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("The clip layered");

    ImGui::SameLine();
    ImGui::SetNextItemWidth(width * 0.3f);
    const char* maskName = state->layerMask != InvalidIndex ? masks[state->layerMask]->name.AsCharPtr() : "All joints";
    if (ImGui::BeginCombo("##mask", maskName))
    {
        if (ImGui::Selectable("All joints", state->layerMask == InvalidIndex))
            state->layerMask = InvalidIndex;
        for (IndexT i = 0; i < (IndexT)masks.size(); i++)
        {
            ImGui::PushID(i);
            if (ImGui::Selectable(masks[i]->name.AsCharPtr(), i == state->layerMask))
                state->layerMask = i;
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("The joint mask it's mixed with");

    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5.0f);
    if (ImGui::SliderFloat("##weight", &state->layerWeight, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp) && playing)
        Characters::CharacterContext::SetTrackWeight(preview, LayerTrack, state->layerWeight);
    ImGui::SetItemTooltip("The weight it's mixed with");

    ImGui::SameLine();
    if (ImGui::Button(playing ? Glyph("square") : Glyph("play")))
    {
        if (playing)
            Characters::CharacterContext::StopTrack(preview, LayerTrack, state->playback.fade);
        else
            restart = true;
    }
    ImGui::SetItemTooltip(playing ? "Stop the layer, it fades out with the fade of the clips" : "Play the layer, looped. It fades in with the fade of the clips");
    ImGui::PopID();

    if (restart)
    {
        // with a fade a clip picked while the layer plays fades in over the one it plays
        const float fade = state->playback.fade;
        if (fade <= 0.0f)
            Characters::CharacterContext::StopTrack(preview, LayerTrack);
        Characters::CharacterContext::PlayClip(preview, &sampleMask, state->layerClip, LayerTrack, Characters::EnqueueMode::Replace, state->layerWeight, 0, 0.0f, fade, 0.0f, 0.0f, state->playback.speed);
        state->layerSpeed = state->playback.speed;
    }
    else if (playing && state->layerSpeed != state->playback.speed)
    {
        // the layer plays at the speed of the clips
        Characters::CharacterContext::SetTrackTimeFactor(preview, LayerTrack, state->playback.speed);
        state->layerSpeed = state->playback.speed;
    }
}

//------------------------------------------------------------------------------
/**
    The sections the clips of the asset are cut into when it's packaged. They're edited on the asset, the clips they're
    cut into are in the animation the preview plays once the asset is saved
*/
void
ModelDocument::DrawClipSections()
{
    ModelDocumentState* state = this->state;
    const ToolkitUtil::AnimResourceT* anim = state->asset.animation.get();
    if (anim == nullptr)
        return;
    auto& sections = state->asset.clip_sections;
    const Graphics::GraphicsEntityId preview = state->preview;

    // the clip the preview plays, if it's one sections can be cut from, and its time
    Util::String playingName;
    Timing::Time playingTime = 0.0;
    const IndexT playing = Characters::CharacterContext::GetPlayingClip(preview, 0);
    const CoreAnimation::AnimationId animation = Characters::CharacterContext::GetAnimation(preview);
    if (playing != InvalidIndex && animation != CoreAnimation::InvalidAnimationId)
    {
        const Util::StringAtom name = CoreAnimation::AnimGetClip(animation, playing).GetName();
        if (name.IsValid() && FindAssetClip(anim, name.Value()) != nullptr)
        {
            playingName = name.Value();
            playingTime = Characters::CharacterContext::GetTrackTime(preview, 0);
        }
    }

    ImGui::SeparatorText("Sections");
    ImGui::TextDisabled("Parts of clips packaged as clips of their own, they're playable once the asset is saved");

    IndexT erase = InvalidIndex;
    if (!sections.empty() && ImGui::BeginTable("Sections", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Clip", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Start", ImGuiTableColumnFlags_WidthStretch, 1.5f);
        ImGui::TableSetupColumn("End", ImGuiTableColumnFlags_WidthStretch, 1.5f);
        ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();

        for (IndexT i = 0; i < (IndexT)sections.size(); i++)
        {
            ToolkitUtil::AnimClipSectionT& section = *sections[i];
            const ToolkitUtil::AnimClipT* source = FindAssetClip(anim, section.clip);
            const float duration = source != nullptr ? source->duration / 1000.0f : 0.0f;
            ImGui::PushID(i);

            ImGui::TableNextColumn();
            char name[64];
            section.name.CopyToBuffer(name, sizeof(name));
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputText("##name", name, sizeof(name)))
                section.name = name;
            if (ImGui::IsItemDeactivatedAfterEdit())
                this->Commit("Rename clip section");

            // the reason the section can't be cut, with sections of the same name as well
            Util::String error;
            bool valid = ToolkitUtil::ValidateClipSection(anim, section, error);
            for (IndexT j = 0; j < (IndexT)sections.size() && valid; j++)
            {
                if (j != i && sections[j]->name == section.name)
                {
                    error.Format("there's another section named '%s'", section.name.AsCharPtr());
                    valid = false;
                }
            }
            if (!valid)
            {
                ImGui::SameLine(0.0f, 0.0f);
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), " (!)");
                ImGui::SetItemTooltip("It isn't packaged, %s", error.AsCharPtr());
            }

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::BeginCombo("##clip", section.clip.AsCharPtr()))
            {
                for (const auto& instance : anim->animations)
                {
                    for (const auto& clip : instance->clips)
                    {
                        if (ImGui::Selectable(clip->name.AsCharPtr(), clip->name == section.clip))
                        {
                            section.clip = clip->name;
                            section.end = Math::min(section.end, clip->duration);
                            section.start = Math::min(section.start, section.end);
                            this->Commit("Change section clip");
                        }
                    }
                }
                ImGui::EndCombo();
            }

            // the times are edited in seconds, the buttons take the time the clip is played at
            const bool canTakeTime = playingName == section.clip;
            auto timeField = [&](const char* id, uint& time, float min, float max)
            {
                float seconds = time / 1000.0f;
                ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() - ImGui::GetStyle().ItemInnerSpacing.x);
                if (ImGui::DragFloat(id, &seconds, 0.01f, min, max, "%.3f s", ImGuiSliderFlags_AlwaysClamp))
                    time = (uint)Math::round(seconds * 1000.0f);
                if (ImGui::IsItemDeactivatedAfterEdit())
                    this->Commit("Change section time");
                ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
                ImGui::BeginDisabled(!canTakeTime);
                ImGui::PushID(id);
                if (ImGui::Button(Glyph("crosshair"), ImVec2(ImGui::GetFrameHeight(), 0.0f)))
                {
                    time = (uint)Math::clamp((float)Math::round(playingTime * 1000.0), min * 1000.0f, max * 1000.0f);
                    this->Commit("Change section time");
                }
                ImGui::PopID();
                ImGui::EndDisabled();
                ImGui::SetItemTooltip(canTakeTime ? "Take the time the clip is at" : "Play or scrub the clip to take its time");
            };
            ImGui::TableNextColumn();
            timeField("##start", section.start, 0.0f, section.end / 1000.0f);
            ImGui::TableNextColumn();
            timeField("##end", section.end, section.start / 1000.0f, duration);

            ImGui::TableNextColumn();
            const IndexT sourceIndex = PreviewClipIndex(preview, section.clip);
            ImGui::BeginDisabled(sourceIndex == InvalidIndex || section.start >= section.end);
            if (ImGui::Button(Glyph("play")))
            {
                state->clip = sourceIndex;
                state->previewSection = i;
                Characters::CharacterContext::StopTrack(preview, 0);
                Characters::CharacterContext::PlayClip(preview, nullptr, sourceIndex, 0, Characters::EnqueueMode::Replace, 1.0f, 0, 0.0f, 0.0f, 0.0f, 0.0f, state->playback.speed);
                Characters::CharacterContext::SeekTrack(preview, 0, Timing::TicksToSeconds(section.start));
            }
            ImGui::EndDisabled();
            ImGui::SetItemTooltip("Play the section, its clip is looped within it");
            ImGui::SameLine();
            if (ImGui::Button(Glyph("trash-2")))
                erase = i;
            ImGui::SetItemTooltip("Remove the section");
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    if (erase != InvalidIndex)
    {
        sections.erase(sections.begin() + erase);
        if (state->previewSection == erase)
            state->previewSection = InvalidIndex;
        else if (state->previewSection > erase)
            state->previewSection--;
        this->Commit("Remove clip section");
    }

    // a new section starts at the time the clip is played at, and runs to its end
    if (ImGui::Button(Util::Format("%s Add section", Glyph("plus")).AsCharPtr()))
    {
        Util::String clipName = playingName;
        if (clipName.IsEmpty() && !anim->animations.empty() && !anim->animations[0]->clips.empty())
            clipName = anim->animations[0]->clips[0]->name;
        const ToolkitUtil::AnimClipT* source = FindAssetClip(anim, clipName);
        if (source != nullptr)
        {
            auto section = std::make_unique<ToolkitUtil::AnimClipSectionT>();
            section->clip = clipName;
            section->start = playingName.IsEmpty() ? 0 : (uint)Math::min((uint)Math::round(playingTime * 1000.0), source->duration);
            section->end = source->duration;
            for (int n = (int)sections.size() + 1;; n++)
            {
                section->name = Util::Format("%s_%d", clipName.AsCharPtr(), n);
                bool unique = FindAssetClip(anim, section->name) == nullptr;
                for (const auto& other : sections)
                    unique &= other->name != section->name;
                if (unique)
                    break;
            }
            sections.push_back(std::move(section));
            this->Commit("Add clip section");
        }
    }
    ImGui::SetItemTooltip("Add a section of the clip played, from the time it's at to its end");
}

//------------------------------------------------------------------------------
/**
    The events added to the clips and sections when the asset is packaged. Events of a clip within a section are in the
    section's clip as well
*/
void
ModelDocument::DrawClipEvents()
{
    ModelDocumentState* state = this->state;
    const ToolkitUtil::AnimResourceT* anim = state->asset.animation.get();
    if (anim == nullptr)
        return;
    auto& events = state->asset.clip_events;
    const auto& sections = state->asset.clip_sections;
    const Graphics::GraphicsEntityId preview = state->preview;

    // the clip the preview plays, a clip of the asset or one cut from a section, and its time
    Util::String playingName;
    Timing::Time playingTime = 0.0;
    const IndexT playing = Characters::CharacterContext::GetPlayingClip(preview, 0);
    const CoreAnimation::AnimationId animation = Characters::CharacterContext::GetAnimation(preview);
    if (playing != InvalidIndex && animation != CoreAnimation::InvalidAnimationId)
    {
        const Util::StringAtom name = CoreAnimation::AnimGetClip(animation, playing).GetName();
        if (name.IsValid())
        {
            playingName = name.Value();
            playingTime = Characters::CharacterContext::GetTrackTime(preview, 0);
        }
    }

    // the clips events can be added to, and how long they are
    Util::Array<Util::String> clipNames;
    Util::Array<uint> clipDurations;
    for (const auto& instance : anim->animations)
    {
        for (const auto& clip : instance->clips)
        {
            clipNames.Append(clip->name);
            clipDurations.Append(clip->duration);
        }
    }
    for (const auto& section : sections)
    {
        clipNames.Append(section->name);
        clipDurations.Append(section->end > section->start ? section->end - section->start : 0);
    }
    auto durationOf = [&](const Util::String& clip) -> uint
    {
        const IndexT i = clipNames.FindIndex(clip);
        return i != InvalidIndex ? clipDurations[i] : 0;
    };

    ImGui::SeparatorText("Events");
    ImGui::TextDisabled("Events of the clips and sections, they're in the clips once the asset is saved");

    IndexT erase = InvalidIndex;
    if (!events.empty() && ImGui::BeginTable("Events", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthStretch, 1.5f);
        ImGui::TableSetupColumn("Clip", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthStretch, 1.5f);
        ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();

        for (IndexT i = 0; i < (IndexT)events.size(); i++)
        {
            ToolkitUtil::AnimClipEventT& event = *events[i];
            ImGui::PushID(i);

            ImGui::TableNextColumn();
            char name[64];
            event.name.CopyToBuffer(name, sizeof(name));
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputText("##name", name, sizeof(name)))
                event.name = name;
            if (ImGui::IsItemDeactivatedAfterEdit())
                this->Commit("Rename clip event");
            Util::String error;
            if (!ToolkitUtil::ValidateClipEvent(anim, sections, event, error))
            {
                ImGui::SameLine(0.0f, 0.0f);
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), " (!)");
                ImGui::SetItemTooltip("It isn't packaged, %s", error.AsCharPtr());
            }

            ImGui::TableNextColumn();
            char category[32];
            event.category.CopyToBuffer(category, sizeof(category));
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputTextWithHint("##category", "none", category, sizeof(category)))
                event.category = category;
            if (ImGui::IsItemDeactivatedAfterEdit())
                this->Commit("Change event category");

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::BeginCombo("##clip", event.clip.AsCharPtr()))
            {
                for (IndexT c = 0; c < clipNames.Size(); c++)
                {
                    if (ImGui::Selectable(clipNames[c].AsCharPtr(), clipNames[c] == event.clip))
                    {
                        event.clip = clipNames[c];
                        event.time = Math::min(event.time, clipDurations[c]);
                        this->Commit("Change event clip");
                    }
                }
                ImGui::EndCombo();
            }

            // the time is edited in seconds, the button takes the time the clip is played at
            ImGui::TableNextColumn();
            float seconds = event.time / 1000.0f;
            ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() - ImGui::GetStyle().ItemInnerSpacing.x);
            if (ImGui::DragFloat("##time", &seconds, 0.01f, 0.0f, durationOf(event.clip) / 1000.0f, "%.3f s", ImGuiSliderFlags_AlwaysClamp))
                event.time = (uint)Math::round(seconds * 1000.0f);
            if (ImGui::IsItemDeactivatedAfterEdit())
                this->Commit("Change event time");
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            const bool canTakeTime = playingName == event.clip;
            ImGui::BeginDisabled(!canTakeTime);
            if (ImGui::Button(Glyph("crosshair"), ImVec2(ImGui::GetFrameHeight(), 0.0f)))
            {
                event.time = Math::min((uint)Math::round(playingTime * 1000.0), durationOf(event.clip));
                this->Commit("Change event time");
            }
            ImGui::EndDisabled();
            ImGui::SetItemTooltip(canTakeTime ? "Take the time the clip is at" : "Play or scrub the clip to take its time");

            ImGui::TableNextColumn();
            if (ImGui::Button(Glyph("trash-2")))
                erase = i;
            ImGui::SetItemTooltip("Remove the event");
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    if (erase != InvalidIndex)
    {
        events.erase(events.begin() + erase);
        this->Commit("Remove clip event");
    }

    // a new event is at the time the clip is played at
    ImGui::BeginDisabled(playingName.IsEmpty() || clipNames.FindIndex(playingName) == InvalidIndex);
    if (ImGui::Button(Util::Format("%s Add event", Glyph("plus")).AsCharPtr()))
    {
        auto event = std::make_unique<ToolkitUtil::AnimClipEventT>();
        event->clip = playingName;
        event->time = Math::min((uint)Math::round(playingTime * 1000.0), durationOf(playingName));
        for (int n = (int)events.size() + 1;; n++)
        {
            event->name = Util::Format("event_%d", n);
            bool unique = true;
            for (const auto& other : events)
                unique &= other->name != event->name || other->clip != event->clip;
            if (unique)
                break;
        }
        events.push_back(std::move(event));
        this->Commit("Add clip event");
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("Add an event to the clip played, at the time it's at");

    // the latest events the preview fired, the ones in the packaged clips
    if (!state->firedEvents.IsEmpty())
    {
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear"))
            state->firedEvents.Clear();
        ImGui::SetItemTooltip("Clear the fired events");
    }
    ImGui::TextDisabled("Fired by the preview:");
    if (state->firedEvents.IsEmpty())
    {
        ImGui::SameLine();
        ImGui::TextDisabled("none yet");
    }
    const double now = ImGui::GetTime();
    for (IndexT i = state->firedEvents.Size() - 1; i >= 0; i--)
    {
        const ModelDocumentState::FiredEvent& fired = state->firedEvents[i];
        // a just fired event is bright, and fades
        const float fade = Math::clamp(1.0f - (float)(now - fired.time) / 1.5f, 0.0f, 1.0f);
        const ImVec4 color(0.6f + 0.4f * fade, 0.6f + 0.3f * fade, 0.6f - 0.25f * fade, 1.0f);
        if (fired.category.IsEmpty())
            ImGui::TextColored(color, "  %s in %s", fired.name.AsCharPtr(), fired.clip.AsCharPtr());
        else
            ImGui::TextColored(color, "  %s [%s] in %s", fired.name.AsCharPtr(), fired.category.AsCharPtr(), fired.clip.AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    Written to a file next to the work file first and copied over it, so a write that fails halfway
    leaves the asset as it was
*/
bool
ModelDocument::OnSave(Util::String& outError)
{
    IO::Path tempPath = this->asset;
    tempPath.SetFile(this->asset.GetFile() + "_saving", this->asset.GetType());
    const IO::URI tempURI = tempPath.WorkURI("assets");
    const IO::URI destination = this->asset.WorkURI("assets");

    Ptr<IO::Stream> stream = IO::CreateStream(tempURI);
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (!stream->Open())
    {
        outError.Format("could not write '%s'", tempURI.LocalPath().AsCharPtr());
        return false;
    }
    Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::ModelAsset>(this->state->asset);
    stream->Write(data.GetPtr(), data.Size());
    stream->Close();

    if (!IO::CopyFile(tempURI, destination))
    {
        IO::DeleteFile(tempURI);
        outError.Format("could not replace '%s'", destination.LocalPath().AsCharPtr());
        return false;
    }
    IO::DeleteFile(tempURI);
    return true;
}

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
