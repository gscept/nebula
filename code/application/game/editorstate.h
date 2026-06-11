#pragma once
//------------------------------------------------------------------------------
/**
    Game::EditorState

    A state singleton that keeps track of the state of the level editor.
    
    @note Though the editor is an external library, we still need to keep
          track of the editors state from the game layer in some way, which
          is why this exists.

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/singleton.h"
#include "util/string.h"

namespace Game
{

class EditorState
{
    __DeclareSingleton(EditorState)
public:
    EditorState();
    ~EditorState();

    /// is true if the editor is running
    bool isRunning = false;
    /// is true if the editor is currently playing/simulating the game (play-in-editor)
    bool isPlaying = false;
    /// snapshot path used to restore editor state after a hot reload
    Util::String reloadSnapshotPath;
    /// if true, recreate the editor level from reloadSnapshotPath on next activate
    bool reloadSnapshotPending = false;
    /// true once editor bootstrap scripts have been initialized for this process
    bool pythonBootstrapInitialized = false;
};

} // namespace Game
