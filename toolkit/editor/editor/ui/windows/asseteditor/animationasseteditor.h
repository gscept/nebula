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

/// how the clips are played from the clip list
struct ClipPlayback
{
    /// loop the clips played
    bool loop = true;
    /// the speed the clips are played at
    float speed = 1.0f;
    /// the seconds a clip played fades in over the clip playing, a clip that isn't looped fades out as long as it
    /// ends, and stopping fades out as well
    float fade = 0.0f;
};

/// list the clips of the animation a character entity plays, with buttons to play, pause and stop them on its first track.
/// Clips are played as the playback says. The clip is the one last played, which play starts again once it's stopped.
/// The markers of the clip shown on the time slider are drawn on it, and can be dragged if there's a change handler
void AnimationClips(const Graphics::GraphicsEntityId character, ClipPlayback& playback, IndexT& clip, const Util::Array<ClipMarker>& markers = Util::Array<ClipMarker>(), const ClipMarkerChanged& onMarkerChanged = nullptr);

} // namespace Presentation
