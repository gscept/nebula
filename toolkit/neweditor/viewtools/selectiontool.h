#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::SelectionTool

    The default tool: a click picks on release, ctrl-click toggles, escape clears, and a press that
    moves drags the selection along the ground plane that cuts the entity it grabbed, with left alt
    moving on the y axis. The drag is the old editor's: it arms on the move away from the press,
    grabs a selected entity under the cursor when it arms, and only commits when it both moved and
    lasted longer than a short time.

    A press that moves and finds no selected entity under the cursor is the marquee instead: the box
    from the press to the cursor becomes the selection, and the way it is dragged decides what it
    takes. Dragged to the right it is a window, which takes what it holds, and dragged to the left
    it is a crossing, which takes what it touches.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "neweditor/viewtools/toolcontext.h"
#include "neweditor/viewtools/toolinterface.h"
#include "game/entity.h"
#include "math/plane.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "timing/timer.h"
#include "util/array.h"

namespace Editor
{

class SelectionTool final : public ToolInterface
{
public:
    void Update(const ToolView& view) override;
    bool IsModifying() const override;
    void Abort() override;
    const char* Name() const override;

private:
    /// the entity of the selection that the mouse ray reaches, which is what a starting drag grabs
    Game::Entity PickSelected(const ToolView& view) const;

    /// the marquee: the box from the press to the cursor, what it takes, and the selection it puts
    /// on
    /// the release
    void UpdateMarquee(const ToolView& view);

    struct
    {
        /// used to check if any changes should be applied to the entities
        bool isDirty = false;
        /// stands in for the old selection context pause: the frame that a drag attempt ends on is
        /// not also a click pick
        bool paused = false;
        /// entity translation delta when dragging entities around. Applied and reset when mouse is
        /// released
        Math::vec3 delta = Math::vec3(0);
        /// start position of the mouse in worldspace along the XZ plane that cuts the grabbed
        /// object's
        /// origin when we start translating
        Math::vec3 startPos = Math::vec3(0);
        /// the grabbed entity's own position when the drag armed.
        Math::vec3 originStartPos = Math::vec3(0);
        /// the current translation plane
        Math::plane plane;
        /// the cursor height y mode started measuring from. Measuring the y delta from the grab
        /// height instead would snap the entity to the cursor's sight-line the moment alt goes down
        float yBase = 0.0f;
        /// true while the y axis measurement is anchored
        bool altDown = false;
        /// position of mouse when starting translation
        Math::vec2 mousePosOnStart = Math::vec2(0, 0);
        /// timer that starts when the mouse starts dragging
        Timing::Timer dragTimer;
        /// the entity that is used to calculate translation planes.
        Game::Entity originEntity = Game::Entity::Invalid();
    } drag;

    struct
    {
        /// the corner the box is drawn from, which is where the press happened
        Math::vec2 start = Math::vec2(0, 0);
        /// the other corner, the cursor as of this frame
        Math::vec2 current = Math::vec2(0, 0);
        /// true while the box is being dragged
        bool active = false;
        /// true once escape or the right button has taken the box back. The gesture is over, and it
        /// stays
        /// over until the button is pressed again, otherwise a box that is let go of while the
        /// button is
        /// still down would arm itself again on the next frame
        bool cancelled = false;
    } marquee;
};

} // namespace Editor
