#pragma once
//------------------------------------------------------------------------------
/**
    @file toolinterface.h

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/editor.h"

namespace Editor
{
class Camera;
}

namespace Tools
{

class ToolInterface
{
public:
    /// Call before Update
    virtual void Render(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera) = 0;
    /// Call after render.
    virtual void Update(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera) = 0;
    /// Override to return true if the tool is currently modifying something (transforming, etc.)
    virtual bool IsModifying() const = 0;
    /// Override to abort any active actions
    virtual void Abort() {}
};

} // namespace Tools
