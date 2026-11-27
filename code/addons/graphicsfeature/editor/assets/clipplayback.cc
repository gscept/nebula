//------------------------------------------------------------------------------
//  @file animationasseteditor.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "graphicsfeature/editor/assets/clipplayback.h"

#if WITH_NEBULA_EDITOR
#include "characters/charactercontext.h"
#include "coreanimation/animation.h"
#include "neweditor/base/iconregistry.h"
#include "imgui.h"

namespace GraphicsFeature
{

//------------------------------------------------------------------------------
/**
    A glyph from the editor's icon table, empty when the table has no such name
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
static void
PlayFromStart(const Graphics::GraphicsEntityId character, const IndexT clip, const bool loop, const float speed, const float fade = 0.0f)
{
    // the clip starts on the next update. With a fade it fades in over the clip playing, and out as it ends if it isn't looped
    if (fade <= 0.0f)
        Characters::CharacterContext::StopTrack(character, 0);
    Characters::CharacterContext::PlayClip(character, nullptr, clip, 0, Characters::EnqueueMode::Replace, 1.0f, loop ? 0 : 1, 0.0f, fade, loop ? 0.0f : fade, 0.0f, speed);
}

//------------------------------------------------------------------------------
/**
    Scrubbing pauses the clip, it plays on when it's let go if it was playing. A stopped clip is started paused.
    The id keeps whether to resume in the state storage
*/
static void
BeginScrub(const Graphics::GraphicsEntityId character, const IndexT clip, const bool loop, const float speed, const ImGuiID id)
{
    const IndexT playing = Characters::CharacterContext::GetPlayingClip(character, 0);
    const bool wasPlaying = playing != InvalidIndex && !Characters::CharacterContext::IsTrackPaused(character, 0);
    ImGui::GetStateStorage()->SetBool(id, wasPlaying);
    if (playing == InvalidIndex)
    {
        PlayFromStart(character, clip, loop, speed);
        Characters::CharacterContext::PauseTrack(character, 0);
    }
    else if (wasPlaying)
    {
        Characters::CharacterContext::PauseTrack(character, 0);
    }
}

//------------------------------------------------------------------------------
/**
*/
static void
EndScrub(const Graphics::GraphicsEntityId character, const ImGuiID id)
{
    ImGuiStorage* storage = ImGui::GetStateStorage();
    if (storage->GetBool(id))
    {
        storage->SetBool(id, false);
        if (Characters::CharacterContext::IsTrackPaused(character, 0))
            Characters::CharacterContext::PauseTrack(character, 0);
    }
}

//------------------------------------------------------------------------------
/**
    Draws the markers of a clip over the time slider, the last item, and a handle on each edge of a range and on each
    point, which drag it. The slider has to allow overlap for the handles to get the mouse
*/
static void
ClipMarkerHandles(const Graphics::GraphicsEntityId character, const IndexT clip, const bool loop, const float speed, const Util::Array<ClipMarker>& markers, const ClipMarkerChanged& onChange, const Util::String& clipName, const float duration)
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
    auto toTime = [&](float x) { return x1 > x0 ? Math::clamp((x - x0) / (x1 - x0), 0.0f, 1.0f) * duration : 0.0f; };

    // overlapping ranges get lanes of their own at the bottom of the slider
    static const ImU32 Colors[] = { IM_COL32(90, 170, 255, 255), IM_COL32(120, 220, 120, 255), IM_COL32(255, 170, 70, 255), IM_COL32(220, 120, 220, 255) };
    static const ImU32 PointColor = IM_COL32(255, 230, 90, 255);
    const float laneHeight = 3.0f;
    const int maxLanes = Math::max(1, (int)((max.y - min.y) * 0.5f / laneHeight));
    Util::Array<float> laneEnds;
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseHoveringRect(min, max);
    Util::String hoveredNames;
    IndexT colorIndex = 0;

    // the handles are items after the slider, the cursor goes back to where the slider left it
    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    const float handleWidth = 8.0f;
    ImGui::PushID("markers");
    for (IndexT m = 0; m < markers.Size(); m++)
    {
        const ClipMarker& marker = markers[m];
        if (marker.clip != clipName)
            continue;
        const float start = toX(marker.start);
        const float end = toX(marker.end);

        ImU32 color = marker.point ? PointColor : Colors[colorIndex++ % (sizeof(Colors) / sizeof(Colors[0]))];
        if (!marker.highlight)
            color = (color & ~IM_COL32_A_MASK) | IM_COL32(0, 0, 0, 170);
        const float thickness = marker.highlight ? 2.0f : 1.0f;
        if (marker.point)
        {
            // a point is a line with a triangle on top
            drawList->AddLine(ImVec2(start, min.y + 1.0f), ImVec2(start, max.y - 1.0f), color, thickness);
            drawList->AddTriangleFilled(ImVec2(start - 4.0f, min.y + 1.0f), ImVec2(start + 4.0f, min.y + 1.0f), ImVec2(start, min.y + 6.0f), color);
        }
        else
        {
            IndexT lane = 0;
            while (lane < laneEnds.Size() && laneEnds[lane] > start)
                lane++;
            if (lane == laneEnds.Size())
                laneEnds.Append(end);
            else
                laneEnds[lane] = end;
            lane = Math::min(lane, maxLanes - 1);
            const float bottom = max.y - 1.0f - lane * laneHeight;
            drawList->AddRectFilled(ImVec2(start, bottom - laneHeight + 1.0f), ImVec2(end, bottom), color);
            drawList->AddLine(ImVec2(start, min.y + 1.0f), ImVec2(start, bottom), color, thickness);
            drawList->AddLine(ImVec2(end, min.y + 1.0f), ImVec2(end, bottom), color, thickness);
        }

        if (hovered && mouse.x >= start - handleWidth * 0.5f && mouse.x <= end + handleWidth * 0.5f)
        {
            if (!hoveredNames.IsEmpty())
                hoveredNames.Append("\n");
            if (marker.point)
                hoveredNames.Append(Util::Format("%s (%.3f s)", marker.name.AsCharPtr(), marker.start));
            else
                hoveredNames.Append(Util::Format("%s (%.3f s - %.3f s)", marker.name.AsCharPtr(), marker.start, marker.end));
        }

        if (!onChange)
            continue;

        // a handle on the start edge, and on the end edge of a range
        for (int edge = 0; edge < (marker.point ? 1 : 2); edge++)
        {
            const float x = edge == 0 ? start : end;
            ImGui::PushID(m * 2 + edge);
            ImGui::SetCursorScreenPos(ImVec2(x - handleWidth * 0.5f, min.y));
            ImGui::InvisibleButton("##handle", ImVec2(handleWidth, max.y - min.y));
            const ImGuiID id = ImGui::GetItemID();
            if (ImGui::IsItemHovered() || ImGui::IsItemActive())
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

            // the handle is dragged by where it was grabbed, so it doesn't jump to the mouse
            ImGuiStorage* storage = ImGui::GetStateStorage();
            if (ImGui::IsItemActivated())
            {
                storage->SetFloat(id, mouse.x - x);
                storage->SetBool(id + 2, false);
                BeginScrub(character, clip, loop, speed, id + 1);
            }
            if (ImGui::IsItemActive())
            {
                const float minGap = 0.001f;
                const float time = toTime(mouse.x - storage->GetFloat(id));
                float newStart = marker.start, newEnd = marker.end;
                if (marker.point)
                    newStart = newEnd = time;
                else if (edge == 0)
                    newStart = Math::clamp(time, 0.0f, Math::max(0.0f, marker.end - minGap));
                else
                    newEnd = Math::clamp(time, Math::min(duration, marker.start + minGap), duration);
                if (newStart != marker.start || newEnd != marker.end)
                {
                    onChange(marker, newStart, newEnd, false);
                    storage->SetBool(id + 2, true);
                }
                Characters::CharacterContext::SeekTrack(character, 0, edge == 0 ? newStart : newEnd);
            }
            if (ImGui::IsItemDeactivated())
            {
                // a click that didn't move it isn't a change
                if (storage->GetBool(id + 2))
                    onChange(marker, marker.start, marker.end, true);
                EndScrub(character, id + 1);
            }
            ImGui::PopID();
        }
    }
    ImGui::PopID();
    ImGui::SetCursorScreenPos(cursor);

    if (!hoveredNames.IsEmpty())
        ImGui::SetTooltip("%s", hoveredNames.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
    Draw the fades of a clip over the time slider, the last item. The fades are in the clip's time: a ramp up from the
    start of the clip where it fades in, and down to its end where it fades out. Where it isn't at its full weight
    is shaded
*/
static void
FadeRamps(const float fadeIn, const float fadeOut, const float duration)
{
    if (duration <= 0.0f || (fadeIn <= 0.0f && fadeOut <= 0.0f))
        return;

    // where the slider puts the center of its grab for a time, like the marker handles
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const float grabPadding = 2.0f;
    const float grabSize = Math::min(style.GrabMinSize, max.x - min.x - grabPadding * 2.0f);
    const float x0 = min.x + grabPadding + grabSize * 0.5f;
    const float x1 = max.x - grabPadding - grabSize * 0.5f;
    auto toX = [&](float time) { return x0 + (x1 - x0) * Math::clamp(time / duration, 0.0f, 1.0f); };
    const float top = min.y + 2.0f;
    const float bottom = max.y - 2.0f;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImU32 shade = IM_COL32(0, 0, 0, 90);
    const ImU32 ramp = IM_COL32(230, 230, 230, 200);
    if (fadeIn > 0.0f)
    {
        const ImVec2 start(toX(0.0f), bottom);
        const ImVec2 end(toX(fadeIn), top);
        drawList->AddTriangleFilled(ImVec2(start.x, top), start, end, shade);
        drawList->AddLine(start, end, ramp, 1.5f);
    }
    if (fadeOut > 0.0f)
    {
        const ImVec2 start(toX(duration - fadeOut), top);
        const ImVec2 end(toX(duration), bottom);
        drawList->AddTriangleFilled(start, ImVec2(end.x, top), end, shade);
        drawList->AddLine(start, end, ramp, 1.5f);
    }
}

//------------------------------------------------------------------------------
/**
    Draw the weight the playing clip is faded to over the time slider, the last item: a dot at the clip's time, as
    high as its weight, with a line down to the bottom. It rides the fade ramps while the clip fades as they show
*/
static void
FadeWeightMarker(const float weight, const float time, const float duration)
{
    if (duration <= 0.0f)
    {
        return;
    }

    // where the slider puts the center of its grab for a time, like the marker handles
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const float grabPadding = 2.0f;
    const float grabSize = Math::min(style.GrabMinSize, max.x - min.x - grabPadding * 2.0f);
    const float x0 = min.x + grabPadding + grabSize * 0.5f;
    const float x1 = max.x - grabPadding - grabSize * 0.5f;
    const float x = x0 + (x1 - x0) * Math::clamp(time / duration, 0.0f, 1.0f);
    const float top = min.y + 2.0f;
    const float bottom = max.y - 2.0f;
    const ImVec2 point(x, Math::lerp(bottom, top, Math::clamp(weight, 0.0f, 1.0f)));

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImU32 color = IM_COL32(255, 210, 60, 255);
    drawList->AddLine(ImVec2(x, bottom), point, color, 1.5f);
    drawList->AddCircleFilled(point, 3.5f, color);
    drawList->AddCircle(point, 3.5f, IM_COL32(0, 0, 0, 200), 0, 1.0f);
}

//------------------------------------------------------------------------------
/**
    The times a clip has keys at in milliseconds, of all its curves, in order. A baked clip has one per frame
*/
static Util::Array<Timing::Tick>
ClipKeyTimes(const CoreAnimation::AnimationId animation, const IndexT clipIndex)
{
    const CoreAnimation::AnimClip& clip = CoreAnimation::AnimGetClip(animation, clipIndex);
    const Util::FixedArray<CoreAnimation::AnimCurve>& curves = CoreAnimation::AnimGetCurves(animation);
    const CoreAnimation::AnimKeyBuffer::Interval* intervals = CoreAnimation::AnimGetBuffer(animation)->GetIntervalBufferPointer();
    Util::Array<Timing::Tick> times;
    for (IndexT c = 0; c < clip.numCurves; c++)
    {
        const CoreAnimation::AnimCurve& curve = curves[clip.firstCurve + c];
        for (uint i = 0; i < curve.numIntervals; i++)
        {
            const CoreAnimation::AnimKeyBuffer::Interval& interval = intervals[curve.firstIntervalOffset + i];
            times.Append(interval.start);
            if (i + 1 == curve.numIntervals)
                times.Append(interval.end);
        }
    }
    times.Sort();
    Util::Array<Timing::Tick> unique;
    for (Timing::Tick time : times)
    {
        if ((unique.IsEmpty() || unique.Back() != time) && time >= 0 && time <= clip.duration)
            unique.Append(time);
    }
    return unique;
}

//------------------------------------------------------------------------------
/**
    Show the pose of a clip at a time, the clip is paused there, a stopped one is started paused
*/
static void
PauseAt(const Graphics::GraphicsEntityId character, const IndexT clip, const bool loop, const float speed, const float seconds)
{
    const IndexT playing = Characters::CharacterContext::GetPlayingClip(character, 0);
    if (playing != clip)
    {
        PlayFromStart(character, clip, loop, speed);
        Characters::CharacterContext::PauseTrack(character, 0);
    }
    else if (!Characters::CharacterContext::IsTrackPaused(character, 0))
    {
        Characters::CharacterContext::PauseTrack(character, 0);
    }
    Characters::CharacterContext::SeekTrack(character, 0, seconds);
}

//------------------------------------------------------------------------------
/**
    An icon button with a tooltip, drawn like a pressed button while it's on
*/
static bool
TransportButton(const char* icon, const char* tooltip, const bool on = false)
{
    if (on)
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    const float size = ImGui::GetFrameHeight();
    const bool pressed = ImGui::Button(icon, ImVec2(size * 1.25f, size));
    if (on)
        ImGui::PopStyleColor();
    ImGui::SetItemTooltip("%s", tooltip);
    return pressed;
}

//------------------------------------------------------------------------------
/**
*/
void
AnimationClips(const Graphics::GraphicsEntityId character, ClipPlayback& playback, IndexT& clip, const Util::Array<ClipMarker>& markers, const ClipMarkerChanged& onMarkerChanged)
{
    bool& loop = playback.loop;
    float& speed = playback.speed;
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

    // the transport: back to the start, a key back, play or pause, a key on, stop and looping
    const bool isPlaying = playing != InvalidIndex && !paused;
    const float currentTime = playing != InvalidIndex ? (float)Characters::CharacterContext::GetTrackTime(character, 0) : 0.0f;
    ImGui::PushID("transport");
    ImGui::BeginDisabled(clips.IsEmpty());
    if (TransportButton(Glyph("skip-back"), "Back to the start"))
    {
        // a playing clip plays on from the start
        if (isPlaying)
            Characters::CharacterContext::SeekTrack(character, 0, 0.0);
        else
            PauseAt(character, clip, loop, speed, 0.0f);
    }
    ImGui::SameLine(0.0f, 2.0f);
    const bool stepBack = TransportButton(Glyph("step-back"), "Step back to the previous key");
    ImGui::SameLine(0.0f, 2.0f);
    if (TransportButton(isPlaying ? Glyph("pause") : Glyph("play"), isPlaying ? "Pause" : (paused ? "Play on" : "Play")))
    {
        // a paused clip continues, a stopped one starts again
        if (playing != InvalidIndex)
            Characters::CharacterContext::PauseTrack(character, 0);
        else
            PlayFromStart(character, clip, loop, speed, playback.fade);
    }
    ImGui::SameLine(0.0f, 2.0f);
    const bool stepForward = TransportButton(Glyph("step-forward"), "Step on to the next key");
    ImGui::EndDisabled();
    ImGui::SameLine(0.0f, 2.0f);
    ImGui::BeginDisabled(playing == InvalidIndex);
    if (TransportButton(Glyph("square"), playback.fade > 0.0f ? "Stop, fading out" : "Stop"))
        Characters::CharacterContext::StopTrack(character, 0, paused ? 0.0f : playback.fade);
    ImGui::EndDisabled();
    ImGui::SameLine(0.0f, 2.0f);
    if (TransportButton(Glyph("repeat"), loop ? "Looping, applies to the clips played next" : "Not looping, applies to the clips played next", loop))
        loop = !loop;

    // the speed applies to the clip playing right away and to the ones played next
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7.0f);
    bool speedChanged = ImGui::SliderFloat("##speed", &speed, 0.1f, 4.0f, "%.2fx", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        speed = 1.0f;
        speedChanged = true;
    }
    ImGui::SetItemTooltip("Playback speed, ctrl+click to type it, right-click to reset it to 1x");
    if (speedChanged && playing != InvalidIndex)
        Characters::CharacterContext::SetTrackTimeFactor(character, 0, speed);

    // the fade applies to the clips played and stopped next
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6.0f);
    ImGui::DragFloat("##fade", &playback.fade, 0.01f, 0.0f, 5.0f, playback.fade > 0.0f ? "fade %.2f s" : "no fade", ImGuiSliderFlags_AlwaysClamp);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        playback.fade = 0.0f;
    ImGui::SetItemTooltip("Seconds a clip played fades in over the one playing, shown on the time slider, a clip that isn't looped fades out as it ends,\nand stopping fades out. Right-click to turn it off");
    ImGui::PopID();

    // stepping pauses the clip at the key before or after the time it's at, clips without keys step a frame at 30 fps
    if ((stepBack || stepForward) && !clips.IsEmpty())
    {
        const Timing::Tick now = (Timing::Tick)Math::round(currentTime * 1000.0f);
        const Util::Array<Timing::Tick> keys = ClipKeyTimes(animation, clip);
        Timing::Tick target = stepForward ? Math::min(now + 33, clips[clip].duration) : Math::max(now - 33, 0);
        if (keys.Size() > 1)
        {
            target = stepForward ? keys.Back() : keys.Front();
            for (IndexT i = 0; i < keys.Size(); i++)
            {
                if (stepForward && keys[i] > now)
                {
                    target = keys[i];
                    break;
                }
                if (!stepForward && keys[keys.Size() - 1 - i] < now)
                {
                    target = keys[keys.Size() - 1 - i];
                    break;
                }
            }
        }
        PauseAt(character, clip, loop, speed, (float)Timing::TicksToSeconds(target));
    }

    if (!clips.IsEmpty())
    {
        const float duration = (float)Timing::TicksToSeconds(clips[clip].duration);
        float time = playing != InvalidIndex ? (float)Characters::CharacterContext::GetTrackTime(character, 0) : 0.0f;
        // the weight the playing clip is faded to, shown while it's below its full weight
        const float fadeWeight = playing != InvalidIndex ? Characters::CharacterContext::GetTrackFadeWeight(character, 0) : 1.0f;
        const bool fading = fadeWeight < 0.999f;
        ImGui::SetNextItemWidth(-FLT_MIN);
        // the marker handles are drawn over the slider and get the mouse first
        ImGui::SetNextItemAllowOverlap();
        const bool changed = ImGui::SliderFloat("##time", &time, 0.0f, duration, fading ? Util::Format("%%.2f s, weight %.2f", fadeWeight).AsCharPtr() : "%.2f s");
        const ImGuiID resumeId = ImGui::GetItemID();
        if (ImGui::IsItemActivated())
            BeginScrub(character, clip, loop, speed, resumeId);
        if (changed || ImGui::IsItemActivated())
            Characters::CharacterContext::SeekTrack(character, 0, time);
        if (ImGui::IsItemDeactivated())
            EndScrub(character, resumeId);

        // the fades of the playback, they take as long whatever the speed, so they're longer in the clip's time the faster
        // it plays. Looped clips don't fade out
        if (playback.fade > 0.0f)
        {
            const float fade = playback.fade * speed;
            FadeRamps(fade, loop ? 0.0f : fade, duration);
        }
        if (fading)
        {
            FadeWeightMarker(fadeWeight, time, duration);
        }

        const Util::StringAtom clipName = clips[clip].GetName();
        ClipMarkerHandles(character, clip, loop, speed, markers, onMarkerChanged, clipName.IsValid() ? clipName.Value() : "", duration);
    }

    // the clips with their events under them, clicking a clip plays it, clicking an event shows the pose at it
    const Util::FixedArray<CoreAnimation::AnimEvent>& events = CoreAnimation::AnimGetEvents(animation);

    // when the events and the clips they're in fired last is kept in the state storage, they're highlighted for a while
    ImGuiStorage* storage = ImGui::GetStateStorage();
    const float now = (float)ImGui::GetTime();
    const float highlightTime = 1.0f;
    ImGui::PushID("fired");
    ImGui::PushID((int)character.HashCode());
    auto clipFiredId = [](IndexT clipIndex) { return ImGui::GetID(Util::Format("clip%d", clipIndex).AsCharPtr()); };
    auto eventFiredId = [](IndexT clipIndex, IndexT eventIndex) { return ImGui::GetID(Util::Format("event%d/%d", clipIndex, eventIndex).AsCharPtr()); };
    for (const Characters::CharacterContext::FiredEvent& fired : Characters::CharacterContext::GetFiredEvents(character))
    {
        if (fired.clip < 0 || fired.clip >= clips.Size())
            continue;
        const CoreAnimation::AnimClip& info = clips[fired.clip];
        storage->SetFloat(clipFiredId(fired.clip), now);
        for (IndexT e = 0; e < info.numEvents; e++)
        {
            const CoreAnimation::AnimEvent& event = events[info.firstEvent + e];
            if (event.name == fired.event.name && event.time == fired.event.time)
                storage->SetFloat(eventFiredId(fired.clip, e), now);
        }
    }
    // the table has an id of its own, so the ids are made here
    Util::FixedArray<ImGuiID> clipIds(clips.Size());
    Util::FixedArray<ImGuiID> eventIds(events.Size());
    for (IndexT i = 0; i < clips.Size(); i++)
    {
        clipIds[i] = clipFiredId(i);
        for (IndexT e = 0; e < clips[i].numEvents && clips[i].firstEvent + e < events.Size(); e++)
            eventIds[clips[i].firstEvent + e] = eventFiredId(i, e);
    }
    ImGui::PopID();
    ImGui::PopID();

    // the row of something that fired is colored, the color fades out
    auto highlightRow = [storage, now, highlightTime](ImGuiID id)
    {
        const float age = now - storage->GetFloat(id, -1000.0f);
        if (age >= 0.0f && age < highlightTime)
        {
            const float alpha = 0.55f * (1.0f - age / highlightTime);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImVec4(1.0f, 0.85f, 0.3f, alpha)));
        }
    };
    if (ImGui::BeginTable("Clips", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Clip");
        ImGui::TableSetupColumn("Time");
        ImGui::TableSetupColumn("Curves");
        ImGui::TableSetupColumn("Events");
        ImGui::TableHeadersRow();
        for (IndexT i = 0; i < clips.Size(); i++)
        {
            const CoreAnimation::AnimClip& info = clips[i];
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            const char* name = info.GetName().IsValid() ? info.GetName().Value() : "";
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_OpenOnArrow;
            if (info.numEvents == 0)
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (i == clip)
                flags |= ImGuiTreeNodeFlags_Selected;
            highlightRow(clipIds[i]);
            const bool open = ImGui::TreeNodeEx("##clip", flags, "%s", name);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            {
                clip = i;
                PlayFromStart(character, i, loop, speed, playback.fade);
            }
            ImGui::SetItemTooltip(info.numEvents > 0 ? "Click to play it, open it for its events" : "Click to play it");
            ImGui::TableNextColumn();
            ImGui::Text("%.3f s", Timing::TicksToSeconds(info.duration));
            ImGui::TableNextColumn();
            ImGui::Text("%d", info.numCurves);
            ImGui::TableNextColumn();
            ImGui::Text("%d", info.numEvents);

            if (open && info.numEvents > 0)
            {
                for (IndexT e = 0; e < info.numEvents; e++)
                {
                    const CoreAnimation::AnimEvent& event = events[info.firstEvent + e];
                    ImGui::PushID(e);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    highlightRow(eventIds[info.firstEvent + e]);
                    ImGui::TreeNodeEx("##event", ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet,
                        "%s", event.name.IsValid() ? event.name.Value() : "");
                    if (ImGui::IsItemClicked())
                    {
                        clip = i;
                        PauseAt(character, i, loop, speed, (float)Timing::TicksToSeconds(event.time));
                    }
                    ImGui::SetItemTooltip("Click to show the pose at it");
                    ImGui::TableNextColumn();
                    ImGui::TextDisabled("%.3f s", Timing::TicksToSeconds(event.time));
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    if (event.category.IsValid() && event.category.Value()[0] != '\0')
                        ImGui::TextDisabled("%s", event.category.Value());
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
