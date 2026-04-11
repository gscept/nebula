#pragma once
//------------------------------------------------------------------------------
/**
	@file rotatetool.h

	(C) 2024 Individual contributors, see AUTHORS file
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

//------------------------------------------------------------------------------
/**
    @class  RotateTool

    @brief  Used to rotate entities using a gizmo.
*/
class RotateTool : public ToolInterface
{
public:
    /// Call before Update
    void Render(Presentation::Modules::Viewport* viewport) override;
    /// Call after render
    void Update(Presentation::Modules::Viewport* viewport) override;
    
    bool IsModifying() const override;

    void Abort() override;

private:
    struct
    {
        /// used to check if any changes should be applied to the entities
        bool isDirty = false;
        ///
        bool gizmoTransforming = false;
    } translation;
};

} // namespace Tools
