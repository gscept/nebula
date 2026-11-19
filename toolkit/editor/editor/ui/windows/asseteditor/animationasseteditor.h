#pragma once
//------------------------------------------------------------------------------
/**
    Editor for animation assets

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "asseteditor.h"

namespace Presentation
{

/// a named time range of a clip, or a point in it, shown on the time slider while the clip is
struct ClipMarker
{
    Util::String clip;
    Util::String name;
    float start = 0.0f;     ///< in seconds
    float end = 0.0f;       ///< the same as the start for a point
    bool highlight = false;
    bool point = false;
    /// identifies the marker to the one handling its changes
    IndexT id = InvalidIndex;
};
/// called while a marker is dragged on the time slider with its new start and end, done is set once it's let go
using ClipMarkerChanged = std::function<void(const ClipMarker& marker, float start, float end, bool done)>;

/// list the clips of the animation a character entity plays, with buttons to play, pause and stop them on its first track.
/// Clips are looped while loop is set. The clip is the one last played, which play starts again once it's stopped.
/// The markers of the clip shown on the time slider are drawn on it, and can be dragged if there's a change handler
void AnimationClips(const Graphics::GraphicsEntityId character, bool& loop, IndexT& clip, const Util::Array<ClipMarker>& markers = Util::Array<ClipMarker>(), const ClipMarkerChanged& onMarkerChanged = nullptr);

} // namespace Presentation
