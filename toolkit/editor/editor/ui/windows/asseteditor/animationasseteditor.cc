//------------------------------------------------------------------------------
//  @file animationasseteditor.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "animationasseteditor.h"
#include "characters/charactercontext.h"
#include "coreanimation/animation.h"

namespace Presentation
{

//------------------------------------------------------------------------------
/**
*/
static void
PlayFromStart(const Graphics::GraphicsEntityId character, const IndexT clip, const bool loop)
{
    // the clip starts on the next update
    Characters::CharacterContext::StopTrack(character, 0);
    Characters::CharacterContext::PlayClip(character, nullptr, clip, 0, Characters::EnqueueMode::Replace, 1.0f, loop ? 0 : 1);
}

//------------------------------------------------------------------------------
/**
*/
static void
DrawClipMarkers(const Util::Array<ClipMarker>& markers, const Util::String& clipName, const float duration)
{
    if (duration <= 0.0f)
        return;

    // where the slider puts the center of its grab for a time, like ImGui computes it
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const float grabPadding = 2.0f;
    const float grabSize = Math::min(style.GrabMinSize, max.x - min.x - grabPadding * 2.0f);
    const float x0 = min.x + grabPadding + grabSize * 0.5f;
    const float x1 = max.x - grabPadding - grabSize * 0.5f;
    auto toX = [&](float time) { return x0 + (x1 - x0) * Math::clamp(time / duration, 0.0f, 1.0f); };

    // overlapping ranges get lanes of their own at the bottom of the slider
    static const ImU32 Colors[] = { IM_COL32(90, 170, 255, 255), IM_COL32(120, 220, 120, 255), IM_COL32(255, 170, 70, 255), IM_COL32(220, 120, 220, 255) };
    const float laneHeight = 3.0f;
    const int maxLanes = Math::max(1, (int)((max.y - min.y) * 0.5f / laneHeight));
    Util::Array<float> laneEnds;
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const float mouseX = ImGui::GetIO().MousePos.x;
    const bool hovered = ImGui::IsItemHovered();
    Util::String hoveredNames;
    IndexT colorIndex = 0;
    for (const ClipMarker& marker : markers)
    {
        if (marker.clip != clipName)
            continue;
        const float start = toX(marker.start);
        const float end = toX(marker.end);
        IndexT lane = 0;
        while (lane < laneEnds.Size() && laneEnds[lane] > start)
            lane++;
        if (lane == laneEnds.Size())
            laneEnds.Append(end);
        else
            laneEnds[lane] = end;
        lane = Math::min(lane, maxLanes - 1);

        ImU32 color = Colors[colorIndex++ % (sizeof(Colors) / sizeof(Colors[0]))];
        if (!marker.highlight)
            color = (color & ~IM_COL32_A_MASK) | IM_COL32(0, 0, 0, 150);
        const float bottom = max.y - 1.0f - lane * laneHeight;
        drawList->AddRectFilled(ImVec2(start, bottom - laneHeight + 1.0f), ImVec2(end, bottom), color);
        drawList->AddLine(ImVec2(start, min.y + 1.0f), ImVec2(start, bottom), color, marker.highlight ? 2.0f : 1.0f);
        drawList->AddLine(ImVec2(end, min.y + 1.0f), ImVec2(end, bottom), color, marker.highlight ? 2.0f : 1.0f);

        if (hovered && mouseX >= start && mouseX <= end)
        {
            if (!hoveredNames.IsEmpty())
                hoveredNames.Append("\n");
            hoveredNames.Append(Util::Format("%s (%.3f s - %.3f s)", marker.name.AsCharPtr(), marker.start, marker.end));
        }
    }
    if (!hoveredNames.IsEmpty())
        ImGui::SetTooltip("%s", hoveredNames.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
void
AnimationClips(const Graphics::GraphicsEntityId character, bool& loop, IndexT& clip, const Util::Array<ClipMarker>& markers)
{
    const CoreAnimation::AnimationId animation = Characters::CharacterContext::GetAnimation(character);
    if (animation == CoreAnimation::InvalidAnimationId)
    {
        ImGui::TextDisabled("Loading...");
        return;
    }

    const Util::FixedArray<CoreAnimation::AnimClip>& clips = CoreAnimation::AnimGetClips(animation);
    const IndexT playing = Characters::CharacterContext::GetPlayingClip(character, 0);
    const bool paused = playing != InvalidIndex && Characters::CharacterContext::IsTrackPaused(character, 0);
    if (playing != InvalidIndex)
        clip = playing;
    if (clip < 0 || clip >= clips.Size())
        clip = 0;

    ImGui::BeginDisabled(clips.IsEmpty());
    if (ImGui::Button(playing != InvalidIndex && !paused ? "Pause" : "Play", ImVec2(ImGui::GetFontSize() * 4.0f, 0.0f)))
    {
        // a paused clip continues, a stopped one starts again
        if (playing != InvalidIndex)
            Characters::CharacterContext::PauseTrack(character, 0);
        else
            PlayFromStart(character, clip, loop);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(playing == InvalidIndex);
    if (ImGui::Button("Stop"))
        Characters::CharacterContext::StopTrack(character, 0);
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::Checkbox("Loop", &loop);
    ImGui::SetItemTooltip("Applies to the clips played next");

    // scrubbing pauses the clip, it plays on when the slider is let go if it was playing. A stopped clip is started paused
    if (!clips.IsEmpty())
    {
        const float duration = (float)Timing::TicksToSeconds(clips[clip].duration);
        float time = playing != InvalidIndex ? (float)Characters::CharacterContext::GetTrackTime(character, 0) : 0.0f;
        ImGui::SetNextItemWidth(-FLT_MIN);
        const bool changed = ImGui::SliderFloat("##time", &time, 0.0f, duration, "%.2f s");
        const ImGuiID resumeId = ImGui::GetItemID();
        const Util::StringAtom clipName = clips[clip].GetName();
        DrawClipMarkers(markers, clipName.IsValid() ? clipName.Value() : "", duration);
        ImGuiStorage* storage = ImGui::GetStateStorage();
        if (ImGui::IsItemActivated())
        {
            const bool wasPlaying = playing != InvalidIndex && !paused;
            storage->SetBool(resumeId, wasPlaying);
            if (playing == InvalidIndex)
            {
                PlayFromStart(character, clip, loop);
                Characters::CharacterContext::PauseTrack(character, 0);
            }
            else if (wasPlaying)
            {
                Characters::CharacterContext::PauseTrack(character, 0);
            }
        }
        if (changed || ImGui::IsItemActivated())
            Characters::CharacterContext::SeekTrack(character, 0, time);
        if (ImGui::IsItemDeactivated() && storage->GetBool(resumeId))
        {
            storage->SetBool(resumeId, false);
            if (Characters::CharacterContext::IsTrackPaused(character, 0))
                Characters::CharacterContext::PauseTrack(character, 0);
        }
    }

    if (ImGui::BeginTable("Clips", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Clip");
        ImGui::TableSetupColumn("Duration");
        ImGui::TableSetupColumn("Curves");
        ImGui::TableSetupColumn("Events");
        ImGui::TableHeadersRow();
        for (IndexT i = 0; i < clips.Size(); i++)
        {
            const CoreAnimation::AnimClip& info = clips[i];
            ImGui::PushID(i);
            ImGui::TableNextColumn();
            const char* name = info.GetName().IsValid() ? info.GetName().Value() : "";
            if (ImGui::Selectable(name, i == clip, ImGuiSelectableFlags_SpanAllColumns))
            {
                clip = i;
                PlayFromStart(character, i, loop);
            }
            ImGui::SetItemTooltip("Click to play it");
            ImGui::TableNextColumn();
            ImGui::Text("%.3f s", Timing::TicksToSeconds(info.duration));
            ImGui::TableNextColumn();
            ImGui::Text("%d", info.numCurves);
            ImGui::TableNextColumn();
            ImGui::Text("%d", info.numEvents);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

} // namespace Presentation
