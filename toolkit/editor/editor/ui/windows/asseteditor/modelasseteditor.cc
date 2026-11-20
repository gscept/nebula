//------------------------------------------------------------------------------
//  @file modelasseteditor.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "modelasseteditor.h"
#include "visibility/visibilitycontext.h"
#include "characters/charactercontext.h"
#include "models/nodes/shaderstatenode.h"
#include "editor/ui/windowserver.h"
#include "io/ioserver.h"
#include "meshasseteditor.h"
#include "skeletonasseteditor.h"
#include "animationasseteditor.h"
#include "dynui/nebula_icons.h"
#include "coreanimation/animation.h"

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/model.h"
#include "toolkitutil/asset/assetpackager.h"
#include "editor/tools/livebatcher.h"

namespace Presentation
{

//------------------------------------------------------------------------------
/**
    Gives a node of a model open in the asset editor another material, on the preview and in the asset written
    when it's saved. The item is looked up again, a picked material arrives after it may have moved or closed
*/
static void
SetNodeMaterial(AssetEditor* assetEditor, const IO::Path& model, const Util::String& nodeName, const IO::Path& res)
{
    AssetEditorItem* item = AssetEditor::GetOpenItem(model);
    if (item == nullptr || item->assetType != AssetEditor::AssetType::Model)
        return;

    const Models::ModelId mdl = item->asset.model;
    const auto& nodeTable = Models::GetModelNodeTable(mdl);
    const IndexT tableIndex = nodeTable.FindIndex(nodeName);
    if (tableIndex == InvalidIndex)
        return;
    Models::ShaderStateNode* resourceNode = static_cast<Models::ShaderStateNode*>(nodeTable.ValueAtIndex(tableIndex));
    const IndexT nodeIndex = Models::ModelContext::GetNodeIndex(item->previewObject, nodeName);

    // A bit of duplicated work, but first set the material on the resource level
    resourceNode->SetMaterial(res);

    // Then load the resource and update all models
    Resources::CreateResource(res, "editor", [mdl, nodeIndex](Resources::ResourceId id)
    {
        Models::ModelContext::ChangeMaterialOnModels(mdl, nodeIndex, id);
    });

    ModelEditorItemData* itemData = (ModelEditorItemData*)item->data;

    // Update node
    if (resourceNode->GetType() == Models::NodeType::CharacterSkinNodeType)
    {
        // each skin fragment is packaged as a node of its own
        const Util::Array<Util::String> skinNodeNames = ToolkitUtil::GetSkinNodeNames(itemData->asset.scene.get());
        for (size_t i = 0; i < itemData->asset.scene->skins.size(); i++)
        {
            if (skinNodeNames[(IndexT)i] == nodeName)
                itemData->asset.scene->skins[i]->shape->material = res.AsString();
        }
    }
    else
    {
        for (size_t i = 0; i < itemData->asset.scene->shapes.size(); i++)
        {
            auto& shape = itemData->asset.scene->shapes[i];
            if (shape->transform->name == nodeName)
            {
                shape->material = res.AsString();
            }
        }
    }

    assetEditor->Edit();
    item->editCounter++;
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
UpdateSectionPreview(const Graphics::GraphicsEntityId preview, ModelEditorItemData* itemData)
{
    const auto& sections = itemData->asset.clip_sections;
    if (itemData->previewSection == InvalidIndex)
        return;
    if (itemData->previewSection >= (IndexT)sections.size())
    {
        itemData->previewSection = InvalidIndex;
        return;
    }
    const ToolkitUtil::AnimClipSectionT& section = *sections[itemData->previewSection];
    const IndexT playing = Characters::CharacterContext::GetPlayingClip(preview, 0);
    if (playing == InvalidIndex || playing != PreviewClipIndex(preview, section.clip))
    {
        // another clip was played, or it was stopped
        itemData->previewSection = InvalidIndex;
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
    The sections the clips of the asset are cut into when it's packaged. They're edited on the asset, the clips they're
    cut into are in the animation the preview plays once the asset is saved
*/
static void
ClipSections(AssetEditor* assetEditor, AssetEditorItem* item, ModelEditorItemData* itemData)
{
    const ToolkitUtil::AnimResourceT* anim = itemData->asset.animation.get();
    if (anim == nullptr)
        return;
    auto& sections = itemData->asset.clip_sections;
    const Graphics::GraphicsEntityId preview = item->previewObject;
    auto edited = [assetEditor, item]()
    {
        assetEditor->Edit();
        item->editCounter++;
    };

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
                edited();

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
                            edited();
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
                    edited();
                ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
                ImGui::BeginDisabled(!canTakeTime);
                ImGui::PushID(id);
                if (ImGui::Button(ICON_ttf_LOCATION_ARROW, ImVec2(ImGui::GetFrameHeight(), 0.0f)))
                {
                    time = (uint)Math::clamp((float)Math::round(playingTime * 1000.0), min * 1000.0f, max * 1000.0f);
                    edited();
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
            if (ImGui::Button(ICON_ttf_PLAY))
            {
                itemData->clip = sourceIndex;
                itemData->previewSection = i;
                Characters::CharacterContext::StopTrack(preview, 0);
                Characters::CharacterContext::PlayClip(preview, nullptr, sourceIndex, 0, Characters::EnqueueMode::Replace, 1.0f, 0);
                Characters::CharacterContext::SeekTrack(preview, 0, Timing::TicksToSeconds(section.start));
            }
            ImGui::EndDisabled();
            ImGui::SetItemTooltip("Play the section, its clip is looped within it");
            ImGui::SameLine();
            if (ImGui::Button(ICON_ttf_TRASH))
                erase = i;
            ImGui::SetItemTooltip("Remove the section");
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    if (erase != InvalidIndex)
    {
        sections.erase(sections.begin() + erase);
        if (itemData->previewSection == erase)
            itemData->previewSection = InvalidIndex;
        else if (itemData->previewSection > erase)
            itemData->previewSection--;
        edited();
    }

    // a new section starts at the time the clip is played at, and runs to its end
    if (ImGui::Button(Util::Format("%s Add section", ICON_ttf_PLUS).AsCharPtr()))
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
            edited();
        }
    }
    ImGui::SetItemTooltip("Add a section of the clip played, from the time it's at to its end");
}

//------------------------------------------------------------------------------
/**
    Keep the events the preview fired in the last update, a few of the latest are kept
*/
static void
RecordFiredEvents(const Graphics::GraphicsEntityId preview, ModelEditorItemData* itemData)
{
    const CoreAnimation::AnimationId animation = Characters::CharacterContext::GetAnimation(preview);
    if (animation == CoreAnimation::InvalidAnimationId)
        return;
    const double now = ImGui::GetTime();
    for (const Characters::CharacterContext::FiredEvent& fired : Characters::CharacterContext::GetFiredEvents(preview))
    {
        const Util::StringAtom clip = CoreAnimation::AnimGetClip(animation, fired.clip).GetName();
        itemData->firedEvents.Append({
            fired.event.name.IsValid() ? fired.event.name.Value() : "",
            fired.event.category.IsValid() ? fired.event.category.Value() : "",
            clip.IsValid() ? clip.Value() : "",
            now });
    }
    const SizeT MaxFired = 8;
    while (itemData->firedEvents.Size() > MaxFired)
        itemData->firedEvents.EraseIndex(0);
}

//------------------------------------------------------------------------------
/**
    True if an event of a clip fired within the time a marker flashes for
*/
static bool
FiredRecently(const ModelEditorItemData* itemData, const Util::String& clip, const Util::String& name)
{
    const double now = ImGui::GetTime();
    for (const auto& fired : itemData->firedEvents)
    {
        if (fired.clip == clip && fired.name == name && now - fired.time < 0.3)
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    The events added to the clips and sections when the asset is packaged. Events of a clip within a section are in the
    section's clip as well
*/
static void
ClipEvents(AssetEditor* assetEditor, AssetEditorItem* item, ModelEditorItemData* itemData)
{
    const ToolkitUtil::AnimResourceT* anim = itemData->asset.animation.get();
    if (anim == nullptr)
        return;
    auto& events = itemData->asset.clip_events;
    const auto& sections = itemData->asset.clip_sections;
    const Graphics::GraphicsEntityId preview = item->previewObject;
    auto edited = [assetEditor, item]()
    {
        assetEditor->Edit();
        item->editCounter++;
    };

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
                edited();
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
                edited();

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
                        edited();
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
                edited();
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            const bool canTakeTime = playingName == event.clip;
            ImGui::BeginDisabled(!canTakeTime);
            if (ImGui::Button(ICON_ttf_LOCATION_ARROW, ImVec2(ImGui::GetFrameHeight(), 0.0f)))
            {
                event.time = Math::min((uint)Math::round(playingTime * 1000.0), durationOf(event.clip));
                edited();
            }
            ImGui::EndDisabled();
            ImGui::SetItemTooltip(canTakeTime ? "Take the time the clip is at" : "Play or scrub the clip to take its time");

            ImGui::TableNextColumn();
            if (ImGui::Button(ICON_ttf_TRASH))
                erase = i;
            ImGui::SetItemTooltip("Remove the event");
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    if (erase != InvalidIndex)
    {
        events.erase(events.begin() + erase);
        edited();
    }

    // a new event is at the time the clip is played at
    ImGui::BeginDisabled(playingName.IsEmpty() || clipNames.FindIndex(playingName) == InvalidIndex);
    if (ImGui::Button(Util::Format("%s Add event", ICON_ttf_PLUS).AsCharPtr()))
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
        edited();
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("Add an event to the clip played, at the time it's at");

    // the latest events the preview fired, the ones in the packaged clips
    if (!itemData->firedEvents.IsEmpty())
    {
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear"))
            itemData->firedEvents.Clear();
        ImGui::SetItemTooltip("Clear the fired events");
    }
    ImGui::TextDisabled("Fired by the preview:");
    if (itemData->firedEvents.IsEmpty())
    {
        ImGui::SameLine();
        ImGui::TextDisabled("none yet");
    }
    const double now = ImGui::GetTime();
    for (IndexT i = itemData->firedEvents.Size() - 1; i >= 0; i--)
    {
        const ModelEditorItemData::FiredEvent& fired = itemData->firedEvents[i];
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
    The material of each node drawing a mesh, followed by the resources packaged from the asset if they're exported:
    the mesh, and for skinned models the skeleton and animation of the character the preview is set up with,
    which are shown in the viewport
*/
void 
ModelEditor(AssetEditor* assetEditor, AssetEditorItem* item)
{
    ModelEditorItemData* itemData = (ModelEditorItemData*)item->data;
    const Graphics::GraphicsEntityId preview = item->previewObject;
    const bool isCharacter = Characters::CharacterContext::IsEntityRegistered(preview);
    const Characters::SkeletonId skeleton = isCharacter ? Characters::CharacterContext::GetSkeleton(preview) : Characters::InvalidSkeletonId;

    if (ImGui::BeginTable("Model Editor", 2, ImGuiTableFlags_Resizable))
    {
        // Column 1
        {
            ImGui::TableSetupScrollFreeze(2, 1);
            ImGui::TableNextColumn();
            assetEditor->viewport.RenderInChildWindow([assetEditor, itemData, preview, skeleton](ImDrawList* drawList)
            {
                if (!itemData->showSkeleton || skeleton == Characters::InvalidSkeletonId)
                    return;
                SkeletonOverlay(drawList, assetEditor->viewport, skeleton, Characters::CharacterContext::GetJointTransforms(preview),
                    Models::ModelContext::GetTransform(preview), itemData->hoveredJoint);
            });
        }

        // Column 2
        ImGui::TableNextColumn();
        if (ImGui::BeginChild("##sections"))
        {
            if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // a material field for each node drawing a mesh
                const auto& nodeTable = Models::GetModelNodeTable(item->asset.model);
                for (const auto& nodePair : nodeTable)
                {
                    const Models::NodeType nodeType = nodePair.Value()->GetType();
                    if (nodeType == Models::NodeType::PrimitiveNodeType || nodeType == Models::NodeType::CharacterSkinNodeType)
                    {
                        const auto node = static_cast<Models::ShaderStateNode*>(nodePair.Value());
                        const Util::String materialName = Materials::MaterialGetName(node->GetMaterial()).Value();
                        Util::String urn;
                        IO::Path material;
                        if (AssetURN(materialName, "mat", urn))
                            material = IO::Path::Parse(urn);
                        else
                            urn = Util::String(materialName).StripSubstring(IO::URI("mat:").LocalPath());

                        ImGui::Text("Node: "); ImGui::SameLine();
                        ImGui::Text(nodePair.Key().Value());
                        ImGui::Text("Material");
                        ImGui::SameLine();
                        AssetField(nodePair.Key().Value(), urn, material, "mat",
                            [assetEditor, model = item->path, nodeName = Util::String(nodePair.Key().Value())](const IO::Path& picked)
                        {
                            SetNodeMaterial(assetEditor, model, nodeName, picked);
                        });
                    }
                }
            }

            if (itemData->mesh != CoreGraphics::InvalidMeshResourceId && ImGui::CollapsingHeader("Mesh"))
            {
                ImGui::PushID("mesh");
                MeshInfo(itemData->mesh);
                ImGui::PopID();
            }

            itemData->hoveredJoint = InvalidIndex;
            if (skeleton != Characters::InvalidSkeletonId && ImGui::CollapsingHeader("Skeleton", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Show in viewport", &itemData->showSkeleton);
                ImGui::SetItemTooltip("Hover a joint to highlight it");
                itemData->hoveredJoint = SkeletonJointTree(skeleton);
            }

            if (isCharacter && ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushID("animation");
                UpdateSectionPreview(preview, itemData);
                RecordFiredEvents(preview, itemData);
                Util::Array<ClipMarker> markers;
                for (IndexT i = 0; i < (IndexT)itemData->asset.clip_sections.size(); i++)
                {
                    const ToolkitUtil::AnimClipSectionT& section = *itemData->asset.clip_sections[i];
                    markers.Append({ section.clip, section.name, section.start / 1000.0f, section.end / 1000.0f, i == itemData->previewSection, false, i });
                }
                for (IndexT i = 0; i < (IndexT)itemData->asset.clip_events.size(); i++)
                {
                    const ToolkitUtil::AnimClipEventT& event = *itemData->asset.clip_events[i];
                    markers.Append({ event.clip, event.name, event.time / 1000.0f, event.time / 1000.0f, FiredRecently(itemData, event.clip, event.name), true, i });
                }
                AnimationClips(preview, itemData->loopClips, itemData->clip, markers,
                    [assetEditor, model = item->path](const ClipMarker& marker, float start, float end, bool done)
                {
                    // the item is looked up again, a marker is dragged over several frames
                    AssetEditorItem* item = AssetEditor::GetOpenItem(model);
                    if (item == nullptr || item->data == nullptr)
                        return;
                    // points are events, ranges are sections
                    ModelEditorItemData* data = (ModelEditorItemData*)item->data;
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
                    {
                        assetEditor->Edit();
                        item->editCounter++;
                    }
                });
                ClipSections(assetEditor, item, itemData);
                ClipEvents(assetEditor, item, itemData);
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
        ImGui::EndTable();
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelSetup(AssetEditorItem* item)
{
    ModelEditorItemData* itemData = item->allocator.Alloc<ModelEditorItemData>();
    Ptr<IO::Stream> assetFileStream = IO::CreateStream(item->path.WorkURI("assets"));
    if (assetFileStream->Open())
    {
        void* data = assetFileStream->MemoryMap();

        ToolkitUtil::ModelAssetT asset;
        Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::ModelAsset>(asset, (uint8_t*)data);
        itemData->asset = asset;

        assetFileStream->MemoryUnmap();
        assetFileStream->Close();
    }
    item->data = itemData;

    // the mesh packaged from the asset, the model loads it as well
    const IO::Path mesh = IO::Path::File(item->path.GetFolder(), item->path.GetFile(), "msh");
    if (IO::FileExists(mesh.ExportURI()))
    {
        Resources::CreateResource(mesh, "preview", [model = item->path](Resources::ResourceId id)
        {
            // the item may have moved or closed by the time the mesh is loaded
            AssetEditorItem* item = AssetEditor::GetOpenItem(model);
            if (item != nullptr && item->assetType == AssetEditor::AssetType::Model && item->data != nullptr)
                ((ModelEditorItemData*)item->data)->mesh = id;
        });
    }

    Models::ModelContext::RegisterEntity(item->previewObject);
    Models::ModelContext::Setup(
        item->previewObject,
        item->path.ExportURI().LocalPath(),
        "preview",
        [gid = item->previewObject, model = item->path]()
        {
            Visibility::ObservableContext::RegisterEntity(gid);
            Visibility::ObservableContext::Setup(gid, Visibility::VisibilityEntityType::Model);
            // skinned models need a character, the skinned shaders read its joints
            Characters::CharacterContext::SetupFromModel(gid, model, "preview");
        },
        1 << 3
    );
    Models::ModelContext::SetAlwaysVisible(item->previewObject);
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelDiscard(AssetEditor* assetEditor, AssetEditorItem* item)
{
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelSave(AssetEditor* assetEditor, AssetEditorItem* item)
{
    ModelEditorItemData* itemData = (ModelEditorItemData*)item->data;

    IO::Path tempPath = item->path;
    tempPath.SetFile(tempPath.GetFile() + Util::String::FromInt((uintptr_t)itemData), tempPath.GetType());
    const IO::URI tempURI = tempPath.WorkURI("assets");
    const IO::URI destination = item->path.WorkURI("assets");

    // Save file replacing the old
    Ptr<IO::Stream> stream = IO::CreateStream(tempURI);
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (stream->Open())
    {
        Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::ModelAsset>(itemData->asset);
        stream->Write(data.GetPtr(), data.Size());
        stream->Close();

        // A bit safer approach, copy new file to old to replace it, and delete new
        if (IO::CopyFile(tempURI, destination))
        {
            IO::DeleteFile(tempURI);

            // Batch it
            Editor::LiveBatcher::BatchFile(item->path);

            assetEditor->Unedit(item->editCounter);
            item->editCounter = 0;
        }
        else
        {
            n_warning("Failed to replace asset '%s' with '%s'\n", destination.LocalPath().AsCharPtr(), tempURI.LocalPath().AsCharPtr());
        }
    }
    else
    {
        n_warning("Failed to save asset '%s'\n", tempURI.LocalPath().AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelShow(AssetEditor* assetEditor, AssetEditorItem* item, bool show)
{
    Models::ModelContext::SetStageMask(item->previewObject, show ? 1 << 3 : 0x0);
}

} // namespace Base
