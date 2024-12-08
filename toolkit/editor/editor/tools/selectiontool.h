#pragma once
//------------------------------------------------------------------------------
/**
	SelectionTool

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolinterface.h"
#include "math/plane.h"

namespace Editor
{
class Camera;
}

namespace Tools
{

class SelectionTool : public ToolInterface
{
public:
    /// Call before Update
    void Render(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera) override;
    /// Call after render
    void Update(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera) override;
    
    bool IsModifying() const override;

    void SnapToGridIncrements(bool value);
    bool SnapToGridIncrements();

private:
    
    struct
    {
        float size = 1.0f;
    } grid;

    struct
    {
        /// increment delta with grid size increments
        bool useGridIncrements = false;
        /// used to check if any changes should be applied to the entities
        bool isDirty = false;
        /// entity translation delta when dragging entities around. Applied and reset when mouse is released
        Math::vec3 delta = Math::vec3(0);
        /// start position of the the mouse in worldspace along the XZ plane that cuts the hovered objects origin when we start translating
        Math::vec3 startPos;
        /// the current translation plane
        Math::plane plane;
        /// position of mouse when starting translation
        Math::vec2 mousePosOnStart;
        /// timer that starts when the mouse starts dragging
        Timing::Timer dragTimer;
        /// the entity that is used to calculate translation planes.
        Editor::Entity originEntity = Editor::Entity::Invalid();
    } translation;
};

} // namespace Tools
