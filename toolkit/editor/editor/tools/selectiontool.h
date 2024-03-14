#pragma once
//------------------------------------------------------------------------------
/**
	SelectionTool

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/editor.h"
#include "math/plane.h"

namespace Editor
{
class Camera;
}

namespace Edit
{
struct CMDSetSelection;
}

namespace Tools
{

class SelectionTool
{
public:
    static Util::Array<Editor::Entity> const& Selection();
    /// Call before Update
    static void RenderGizmo(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera);
    /// Call after render
    static void Update(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera);
    static bool IsTransforming();

private:
    friend Edit::CMDSetSelection;
    
    struct State
    {
        Util::Array<Editor::Entity> selection;

        struct
        {
            float size = 0.5f;
        } grid;

        struct
        {
            /// increment delta with grid size increments
            bool useGridIncrements = false;
            /// used to check if any changes should be applied to the entities
            bool isDirty = false;
            /// will be true if the entities are being dragged around
            bool isTransforming = false;
            /// entity translation delta when dragging entities around. Applied and reset when mouse is released
            Math::vec3 delta = Math::vec3(0);
            /// start position of the the mouse in worldspace along the XZ plane that cuts the hovered objects origin when we start translating
            Math::vec3 startPos;
            /// the current translation plane
            Math::plane plane;
        } translation;

        struct
        {
            /// entities that are omitted from being selected when picking
            Util::Array<Editor::Entity> omittedEntities;
        } picking;
    };

    static State state;
};

} // namespace Tools
