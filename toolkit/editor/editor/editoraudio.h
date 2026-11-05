#pragma once
//------------------------------------------------------------------------------
/**
    Editor UI clips. Loaded once when the editor audio device is open.
*/
//------------------------------------------------------------------------------
#include "audio/audioclip.h"

namespace Editor
{

extern Audio::ClipId Fanfare;

/// Load the editor clip set into the global clip ids.
void LoadEditorAudioClips();
/// Release the editor clip set.
void UnloadEditorAudioClips();

} // namespace Editor
