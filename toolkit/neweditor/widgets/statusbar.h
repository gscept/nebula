#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::StatusBar

    The bar at the bottom of the editor. It is not a panel, because it is not a window a user can
    move, close or dock into.

    It is a strip that is always at the bottom of the editor, and what it shows is what was put on
    it: a message from a panel, the frame time, and the numbers a feature pushed through
    EditorFeatureRegistry.

    A message stays until it is replaced. When a duration is given it goes away after that, which is
    what keeps a message about a single event from sitting on the bar for the rest of the session.

    The bar is on or off through the ed_show_status_bar cvar.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/cvar.h"
#include "core/refcounted.h"
#include "timing/time.h"
#include "util/string.h"

namespace Editor
{

class Shell;

class StatusBar : public Core::RefCounted
{
    __DeclareClass(StatusBar)
public:
    StatusBar();
    virtual ~StatusBar();

    /// resolve the cvar and the core. Called by the core at Open
    bool Setup(Shell* core);

    /// put a message on the bar. An empty message clears it. Without a duration the message stays
    /// until it is
    /// replaced
    void SetStatus(const Util::String& msg, float duration = 0.0f);
    /// clear the message
    void ClearStatus();
    /// the message the bar shows
    const Util::String& Message() const;

    /// true when the bar is on
    bool IsVisible() const;
    /// the strip the bar takes at the bottom of the editor, and 0 when it is off. This is what the
    /// dockspace is
    /// inset by, so that the panels do not draw under the bar
    float Height() const;

    /// draw the bar. Called by the core after the panels, so a message a panel put on it shows on
    /// this frame
    void Draw();

private:
    /// the numbers, on the right of the same line as the message
    void DrawNumbers();

    Shell* core;
    Core::CVar* showBarCVar;
    Util::String message;
    Timing::Time expires;
    float averageFrameTime;
};

} // namespace Editor
