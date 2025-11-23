#pragma once
//------------------------------------------------------------------------------
/**
    @file toolinterface.h

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/editor.h"
#include "editor/ui/modules/viewport.h"


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
    virtual void Render(Presentation::Modules::Viewport* viewport) = 0;
    /// Call after render.
    virtual void Update(Presentation::Modules::Viewport* viewport) = 0;
    /// Override to return true if the tool is currently modifying something (transforming, etc.)
    virtual bool IsModifying() const = 0;
    /// Override to abort any active actions
    virtual void Abort() {}

    virtual ~ToolInterface() {};
};

} // namespace Tools
