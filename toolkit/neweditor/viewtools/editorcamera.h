#pragma once
//------------------------------------------------------------------------------
/**
    @struct
    Editor::EditorCamera The camera a scene is viewed with. It holds the state a view is
    built from. It does not read the keyboard or the mouse, because the panel owns them while it has
    focus, so one frame of input is handed to Update.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "math/mat4.h"
#include "math/vec2.h"
#include "math/vec3.h"

namespace Editor
{

/// the orbit distance a camera starts at, in units
static const float DefaultPivotDistance = 10.0f;

/// the closest the wheel can bring the camera to the pivot
static const float MinPivotDistance = 0.5f;

//------------------------------------------------------------------------------
/**
    One frame of camera input, as the panel read it. moveDir is units per second, the drags are
    window pixels with y down, and zoom is units applied at once. The camera turns them into angles
    and world movement.
*/
struct EditorCameraInput
{
    /// movement in units per second
    Math::vec3 moveDir = Math::vec3(0);

    /// the drag on the right button
    Math::vec2 look = Math::vec2(0);
    /// the drag on the middle button
    Math::vec2 orbit = Math::vec2(0);
    /// the drag on shift plus the middle button
    Math::vec2 pan = Math::vec2(0);

    /// the wheel, in units. Positive is in.
    float zoom = 0;
};

//------------------------------------------------------------------------------
/**
    The camera state. A feedback loop pulls the position towards where the camera wants to be, so a
    focus glides instead of landing.
*/
struct EditorCamera
{
    /// where the camera is
    Math::vec3 position = Math::vec3(0, 0, 0);
    /// where the camera wants to be. A focus puts it somewhere else.
    Math::vec3 targetPosition = Math::vec3(0, 0, 0);
    /// the pitch and the yaw the camera looks with, in radians
    float rotx = 0;
    float roty = 0;
    /// the distance from the camera to the pivot. The wheel changes it.
    float distance = DefaultPivotDistance;
    /// the camera's z axis, which points away from what it looks at
    Math::vec3 forward = Math::vec3(0, 0, 1);

    /// the view matrix the state produces, world to camera. The render side turns it into a camera.
    Math::mat4 view = Math::mat4();

    /// the point the camera orbits around
    Math::vec3 Pivot() const;

    /// Apply one frame of input to the state and write the view matrix.
    void Update(const EditorCameraInput& input, float dt);

    /// Set the pivot and the distance. A focus does this.
    void SetPivot(const Math::vec3& point, float d);
    /// put the camera on one side of the pivot, at the distance it has, looking at the pivot. The
    /// direction is from the pivot to the camera: +y looks down from the top
    void LookFrom(const Math::vec3& direction);
};

//------------------------------------------------------------------------------
/**
*/
inline Math::vec3
Editor::EditorCamera::Pivot() const
{
    return this->targetPosition - this->forward * this->distance;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Editor::EditorCamera::SetPivot(const Math::vec3& point, float d)
{
    this->distance = d;

    // forward points away from what the camera looks at, so standing back from the pivot adds it
    this->targetPosition = point + this->forward * d;
}

} // namespace Editor
