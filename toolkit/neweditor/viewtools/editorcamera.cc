//------------------------------------------------------------------------------
//  @file editorcamera.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "core/cvar.h"
#include "neweditor/viewtools/editorcamera.h"
#include "math/mat4.h"

namespace Editor
{

static Core::CVar* ed_camera_gain = Core::CVarCreate(Core::CVarType::CVar_Float, "ed_camera_gain", "10.0",
    "Proportional gain of the editor camera position feedback loop");

// The drags arrive as window pixels, so these are what one pixel of drag is worth. The pan is worth
// more at a
// longer orbit distance, which is what keeps a drag the same size on the screen whatever the view
// is zoomed to
static const float LookSensitivity = 0.0024f;    // radians per pixel
static const float OrbitSensitivity = 0.0024f;   // radians per pixel
static const float PanSensitivity = 0.01f;       // units per pixel, at the orbit distance

//------------------------------------------------------------------------------
/**
    The input is applied to the camera state here, so that whoever gathers the input does not have
    to know how the camera is built. This is also where the directions of the drags are decided, and
    they are not the same for every gesture, because a gesture is named by what it moves: A look
    drag turns the view the way the cursor goes, so the camera turns up when the cursor goes up. An
    orbit drag moves the camera the way the cursor goes, which turns the view the other way: the
    camera comes over the top when the cursor goes up. A pan drag moves the world the way the cursor
    goes, which moves the camera the other way again. The window reports the cursor with y down,
    which is the other half of why the signs come out as they do here.

    A drag answers on the frame it is made on, because it is direct manipulation. The keys and the
    wheel put the point the camera wants to be at somewhere and let the loop come to it over the
    frames after, which is what gives those parts the inertia the drags do not have.
*/
void
Editor::EditorCamera::Update(const EditorCameraInput& input, float dt)
{
    // The pivot has to be taken before the angles move, because an orbit is the one gesture that
    // keeps it
    const Math::vec3 pivot = this->Pivot();

    this->rotx += -input.look.y * LookSensitivity + input.orbit.y * OrbitSensitivity;
    this->roty += -input.look.x * LookSensitivity + input.orbit.x * OrbitSensitivity;
    this->rotx = Math::clamp(this->rotx, Math::deg2rad(-89.0f), Math::deg2rad(89.0f));

    Math::mat4 matPitch = Math::rotationx(this->rotx);
    Math::mat4 matYaw = Math::rotationy(this->roty);
    Math::mat4 rotate = matYaw * matPitch;

    Math::vec3 forward(rotate.m[2][0], rotate.m[2][1], rotate.m[2][2]);
    Math::vec3 up(rotate.m[1][0], rotate.m[1][1], rotate.m[1][2]);
    Math::vec3 strafe(rotate.m[0][0], rotate.m[0][1], rotate.m[0][2]);

    // A drag is direct manipulation: it moves the camera on the frame it is made on, rather than
    // putting the
    // point it wants to be at somewhere for the loop to come to over the frames after. A look turns
    // in place
    // and moves nothing, an orbit swings the point it wants to be at back to the same pivot at the
    // new angle,
    // and a pan slides it across the plane the camera looks through
    Math::vec3 drag = Math::vec3(0);

    if (input.orbit.x != 0.0f || input.orbit.y != 0.0f)
        drag += (pivot + forward * this->distance) - this->targetPosition;

    const float panVelocity = PanSensitivity;
    drag += (up * input.pan.y - strafe * input.pan.x) * panVelocity;

    this->targetPosition += drag;
    this->position += drag;

    // The wheel changes the distance to the pivot and pulls the camera along the look line by the
    // same amount,
    // so a wheel turn is a nudge towards what the camera looks at rather than a jump. The distance
    // is what an
    // orbit swings around, so it is the part that cannot go through zero
    this->distance = Math::max(this->distance - input.zoom, MinPivotDistance);
    this->targetPosition -= forward * input.zoom;

    // The keys move the point the camera wants to be at, along the basis it looks with
    this->targetPosition += (input.moveDir.z * forward + input.moveDir.x * strafe) * dt;

    const float gain = Core::CVarReadFloat(ed_camera_gain);
    const float factor = Math::clamp(gain * dt, 0.0f, 1.0f);
    this->position += (this->targetPosition - this->position) * factor;

    Math::mat4 translate = Math::translation(this->position);
    this->view = Math::inverse(translate * rotate);

    this->forward = forward;
}

//------------------------------------------------------------------------------
/**
    The forward axis Update builds from a pitch and a yaw
*/
static Math::vec3
ForwardOf(float rotx, float roty)
{
    const Math::mat4 rotate = Math::rotationy(roty) * Math::rotationx(rotx);
    return Math::vec3(rotate.m[2][0], rotate.m[2][1], rotate.m[2][2]);
}

//------------------------------------------------------------------------------
/**
    The pitch and the yaw are searched rather than worked out, so this agrees with Update whatever
    the rotation functions' conventions are. The pitch stops short of straight up and down, the way
    the drags do, so a top view looks down at 89 degrees
*/
void
Editor::EditorCamera::LookFrom(const Math::vec3& direction)
{
    const Math::vec3 pivot = this->Pivot();
    const Math::vec3 want = Math::normalize(direction);

    float bestX = this->rotx, bestY = this->roty, bestDot = -2.0f;
    const float pitches[3] = { 0.0f, Math::deg2rad(89.0f), Math::deg2rad(-89.0f) };
    for (float pitch : pitches)
    {
        for (int quarter = 0; quarter < 4; quarter++)
        {
            const float yaw = Math::deg2rad(90.0f * quarter);
            const float d = Math::dot(ForwardOf(pitch, yaw), want);
            if (d > bestDot)
            {
                bestDot = d;
                bestX = pitch;
                bestY = yaw;
            }
        }
    }

    this->rotx = bestX;
    this->roty = bestY;
    this->forward = ForwardOf(bestX, bestY);
    this->position = this->targetPosition = pivot + this->forward * this->distance;

    const Math::mat4 rotate = Math::rotationy(this->roty) * Math::rotationx(this->rotx);
    this->view = Math::inverse(Math::translation(this->position) * rotate);
}

} // namespace Editor
