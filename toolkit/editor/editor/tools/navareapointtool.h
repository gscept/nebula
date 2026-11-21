#pragma once
//------------------------------------------------------------------------------
/**
    @file navareapointtool.h

    Edits the outline of a navigation area volume in the viewport.

    Starting it turns the footprint of the box into outline points, if the volume has no
    outline yet. Then, in the viewport:
    - a click on the surface adds a point to the edge closest to it, between the two
      points of that edge
    - dragging a point moves it along the surface
    - ctrl and a click on a point, Delete while hovering it, or Delete point in the menu a
      right click on it opens, removes it

    Points land on the loaded navmeshes, or on a level plane where the mouse misses them.
    Escape, or selecting something else, ends the tool.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolinterface.h"

namespace Tools
{

class NavAreaPointTool : public ToolInterface
{
public:
    void Render(Presentation::Modules::Viewport* viewport) override;
    void Update(Presentation::Modules::Viewport* viewport) override;
    bool IsModifying() const override;
    void Abort() override;

    /// start editing the outline of a NavArea entity
    static void Begin(Editor::Entity entity);
    /// stop editing
    static void End();
    /// the entity whose outline is edited, invalid if the tool isn't running
    static Editor::Entity Target();
    /// the outline point under the mouse in the viewport, or -1
    static IndexT HoveredPoint();
    /// highlight an outline point from elsewhere, like the row of the point in a list, -1 for none
    static void SetHighlightedPoint(IndexT index);
    /// the point highlighted from elsewhere, or -1
    static IndexT HighlightedPoint();

private:
    /// check the target and refresh the points, without the mouse. Returns false if the tool ended
    bool Refresh();
    /// the mouse ray, returns false if the mouse is outside the viewport
    bool MouseRay(Presentation::Modules::Viewport* viewport, Math::line& ray) const;
    /// where the ray meets the surface, a level plane at planeY if it misses the navmeshes
    bool SurfacePoint(const Math::line& ray, float planeY, Math::vec3& point) const;
    /// the outline point closest to the ray, within a radius which grows with the distance, or -1
    IndexT PickPoint(const Math::line& ray, const Util::Array<Math::vec3>& worldPoints) const;

    /// world positions of the outline points being edited
    Util::Array<Math::vec3> worldPoints;
    /// the outline in the entity frame, a copy while a point is dragged
    Util::Array<Math::vec3> localPoints;
    Math::mat4 frame;

    IndexT hovered = -1;
    IndexT dragged = -1;
    /// the point a right click opened the menu for
    IndexT menuPoint = -1;
    /// the last frame the menu was shown, the click which closes it, on an item or outside, doesn't place a point
    int menuVisibleFrame = -10;
    bool hasSurfacePoint = false;
    Math::vec3 surfacePoint;
};

} // namespace Tools
