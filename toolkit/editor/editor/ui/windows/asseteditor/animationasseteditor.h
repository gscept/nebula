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

/// a named time range of a clip, shown on the time slider while the clip is
struct ClipMarker
{
    Util::String clip;
    Util::String name;
    float start = 0.0f;     ///< in seconds
    float end = 0.0f;
    bool highlight = false;
};

/// list the clips of the animation a character entity plays, with buttons to play, pause and stop them on its first track.
/// Clips are looped while loop is set. The clip is the one last played, which play starts again once it's stopped.
/// The markers of the clip shown on the time slider are drawn on it
void AnimationClips(const Graphics::GraphicsEntityId character, bool& loop, IndexT& clip, const Util::Array<ClipMarker>& markers = Util::Array<ClipMarker>());

} // namespace Presentation
