//------------------------------------------------------------------------------
//  @file physicsgizmo.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "physicsfeature/editor/physicsgizmo.h"

#if WITH_NEBULA_EDITOR
#include "game/component.h"
#include "neweditor/api/gizmo.h"
#include "physicsfeature/components/physicsfeature.h"
#include "math/mat4.h"
#include "math/quat.h"
#include "util/string.h"

namespace PhysicsFeature
{

// A collider is drawn in a color of its own, not the selection outline's. The green is the physics
// debug draw's
static const Math::vec4 ColliderColor(0.20f, 0.60f, 0.35f, 1.0f);

// The same color with alpha, for the faces
static const Math::vec4 ColliderFill(0.20f, 0.60f, 0.35f, 0.20f);

// The handle id for each face. The id has to be the same string every frame
static const char* const FaceNames[3][2] = {
    { "x+", "x-" },
    { "y+", "y-" },
    { "z+", "z-" }
};

//------------------------------------------------------------------------------
/**
    The box as its twelve edges. The edges are put on the canvas from the shape's own matrix, because a
    shape can be turned inside its entity.
*/
static void
DrawShapeEdges(const Math::mat4& shape, const Math::vec3& extents, const Math::vec4& color)
{
    Math::vec3 corners[8];
    for (int i = 0; i < 8; i++)
    {
        const Math::vec3 local(
            (i & 1) ? extents.x : -extents.x,
            (i & 2) ? extents.y : -extents.y,
            (i & 4) ? extents.z : -extents.z);

        const Math::vec4 world = shape * Math::vec4(local, 1.0f);
        corners[i] = Math::vec3(world.x, world.y, world.z);
    }

    for (int i = 0; i < 8; i++)
    {
        for (int b = 0; b < 3; b++)
        {
            const int j = i ^ (1 << b);
            if (j <= i)
                continue;   // an edge is reached from both of its ends

            Editor::Gizmo::DrawLine(corners[i], corners[j], color, 1.0f, Editor::Gizmo::DrawFlag_AlwaysOnTop);
        }
    }
}

//------------------------------------------------------------------------------
/**
    One handle on each of the six faces. A face handle moves that face and the opposite face stays, so
    half the move falls on the center. Alt applies the whole move to the size instead, and the center
    stays.
*/
static void
DrawBoxColliderGizmo(const Editor::Gizmo::GizmoFrame& frame)
{
    const BoxCollider* base = (const BoxCollider*)frame.value;
    if (base == nullptr)
        return;

    BoxCollider value = *base;

    const Math::mat4 local = Math::trs(base->localPosition, base->localOrientation, Math::vec3(1.0f));
    const Math::mat4 shape = frame.transform * local;

    const Math::vec3 center(Math::xyz(shape.position()));

    const Math::vec3 axes[3] = {
        Math::xyz(shape.x_axis()),
        Math::xyz(shape.y_axis()),
        Math::xyz(shape.z_axis())
    };

    const Math::vec3 unitAxes[3] = {
        Math::rotate(base->localOrientation, Math::vec3(1.0f, 0.0f, 0.0f)),
        Math::rotate(base->localOrientation, Math::vec3(0.0f, 1.0f, 0.0f)),
        Math::rotate(base->localOrientation, Math::vec3(0.0f, 0.0f, 1.0f))
    };

    Editor::Gizmo::HandleStyle style = Editor::Gizmo::DefaultHandleStyle();
    style.color = ColliderColor;

    const bool wholeSize = frame.view->alt;

    bool changed = false;

    for (int i = 0; i < 3; i++)
    {
        for (int side = 0; side < 2; side++)
        {
            // The outward direction of this face, so a drag away from the box reads as a positive move
            const float sign = side == 0 ? 1.0f : -1.0f;
            const Math::vec3 outward = axes[i] * sign;

            Util::String tip;
            tip.Format(wholeSize ? "size %s" : "face %s", FaceNames[i][side]);
            style.tip = tip.AsCharPtr();

            const Math::vec3 at(center + outward * base->extents[i]);

            float out;
            if (Editor::Gizmo::HandleFloat(FaceNames[i][side], base->extents[i], &out, at, outward, style))
            {
                const float moved = out - base->extents[i];

                if (wholeSize)
                {
                    // The whole size changes, so the center stays where it is
                    value.extents[i] = out;
                }
                else
                {
                    value.extents[i] = base->extents[i] + moved * 0.5f;
                    value.localPosition = base->localPosition + unitAxes[i] * (moved * 0.5f * sign);
                }

                Util::String info;
                info.Format("localPosition\n[%5.3f, %5.3f, %5.3f]", value.localPosition.x, value.localPosition.y, value.localPosition.z);
                Editor::Gizmo::DrawText(info.AsCharPtr(), frame.transform * (value.localPosition), {1,1,1,0.85f});
                info.Format("extents\n[%5.3f, %5.3f, %5.3f]", value.extents.x, value.extents.y, value.extents.z);
                Editor::Gizmo::DrawText(info.AsCharPtr(), frame.transform * (value.localPosition + unitAxes[i] * value.extents[i]), {1,1,1,0.85f});

                changed = true;
            }
        }
    }

    const Math::mat4 drawn = frame.transform * Math::trs(value.localPosition, value.localOrientation, Math::vec3(1.0f));

    Editor::Gizmo::DrawBox(drawn * Math::scaling(value.extents * 2.0f), ColliderFill, 1.0f,
        Editor::Gizmo::DrawFlag_Translucent);

    DrawShapeEdges(drawn, value.extents, ColliderColor);

    if (changed)
        Editor::Gizmo::CommitComponent(frame.scene, frame.entity, frame.component, &value, sizeof(value));
}

//------------------------------------------------------------------------------
/**
    One handle, on the ring along x, driving the radius. A handle needs a direction to read the cursor
    against, and a radius has no single one.
*/
static void
DrawSphereColliderGizmo(const Editor::Gizmo::GizmoFrame& frame)
{
    const SphereCollider* base = (const SphereCollider*)frame.value;
    if (base == nullptr)
        return;

    SphereCollider value = *base;

    const Math::mat4 local = Math::trs(base->localPosition, base->localOrientation, Math::vec3(1.0f));
    const Math::mat4 shape = frame.transform * local;
    const Math::vec3 center(Math::xyz(shape.position()));

    // The radius is a world distance, so the handle sits one radius out along a unit axis rather than
    // along the shape's own axis, which carries the entity's scale
    const Math::vec3 axes[3] = {
        Math::rotate(base->localOrientation, Math::vec3(1.0f, 0.0f, 0.0f)),
        Math::rotate(base->localOrientation, Math::vec3(0.0f, 1.0f, 0.0f)),
        Math::rotate(base->localOrientation, Math::vec3(0.0f, 0.0f, 1.0f))
    };

    Editor::Gizmo::HandleStyle style = Editor::Gizmo::DefaultHandleStyle();
    style.color = ColliderColor;
    style.tip = "radius";

    float out;
    const Math::vec3 at(center + axes[0] * base->radius);

    bool changed = false;
    if (Editor::Gizmo::HandleFloat("radius", base->radius, &out, at, axes[0], style))
    {
        value.radius = out;
        changed = true;
    }

    for (int i = 0; i < 3; i++)
        Editor::Gizmo::DrawCircle(center, value.radius, axes[i], ColliderColor, 1.0f,
            Editor::Gizmo::DrawFlag_AlwaysOnTop);

    if (changed)
        Editor::Gizmo::CommitComponent(frame.scene, frame.entity, frame.component, &value, sizeof(value));
}

//------------------------------------------------------------------------------
/**
*/
static void
DrawCharacterGizmo(const Editor::Gizmo::GizmoFrame& frame)
{
    const Character* base = (const Character*)frame.value;
    if (base == nullptr)
        return;

    Character value = *base;

    const Math::vec3 foot(Math::xyz(frame.transform.position()));

    const bool capsule = base->shape == PhysicsFeature::CharacterShape::Capsule;

    Math::vec3 axes[3];
    for (int i = 0; i < 3; i++)
    {
        const Math::vec3 axis(Math::xyz(i == 0 ? frame.transform.x_axis() :
            (i == 1 ? frame.transform.y_axis() : frame.transform.z_axis())));
        axes[i] = Math::normalize(axis);
    }

    Editor::Gizmo::HandleStyle style = Editor::Gizmo::DefaultHandleStyle();
    style.color = ColliderColor;

    bool changed = false;

    float out;

    float halfHeight = capsule ? value.radius + value.height * 0.5f : value.extents.y;
    float halfWidth = capsule ? value.radius : value.extents.x;

    style.tip = "radius";
    if (Editor::Gizmo::HandleFloat("radius", halfWidth, &out, foot + axes[1] * halfHeight + axes[0] * halfWidth, axes[0], style))
    {
        if (capsule)
            value.radius = out;
        else
        {
            value.extents.x = out;
            value.extents.z = out;
        }
        changed = true;
    }

    style.tip = "height";

    const float top = capsule ? value.radius + value.height : value.extents.y * 2.0f;
    const float measure = capsule ? value.height : top;

    if (Editor::Gizmo::HandleFloat("height", measure, &out, foot + axes[1] * top, axes[1], style))
    {
        if (capsule)
            value.height = out;
        else
            value.extents.y = out * 0.5f;
        changed = true;
    }

    halfHeight = capsule ? value.radius + value.height * 0.5f : value.extents.y;
    halfWidth = capsule ? value.radius : value.extents.x;

    const Math::vec3 center(foot + axes[1] * halfHeight);

    const Math::mat4 drawn(
        Math::vec4(axes[0], 0.0f),
        Math::vec4(axes[1], 0.0f),
        Math::vec4(axes[2], 0.0f),
        Math::vec4(center, 1.0f));

    if (capsule)
    {
        // The centers of the two end caps, which is where the caps meet the cylinder between them
        const Math::vec3 cap0(foot + axes[1] * value.radius);
        const Math::vec3 cap1(foot + axes[1] * (value.radius + value.height));

        for (int cap = 0; cap < 2; cap++)
        {
            const Math::vec3 ring(cap == 0 ? cap0 : cap1);

            Editor::Gizmo::DrawCircle(ring, value.radius, axes[1], ColliderColor, 1.0f,
                Editor::Gizmo::DrawFlag_AlwaysOnTop);
            Editor::Gizmo::DrawCircle(ring, value.radius, axes[2], ColliderColor, 1.0f,
                Editor::Gizmo::DrawFlag_AlwaysOnTop);
        }

        for (int side = 0; side < 2; side++)
        {
            const float sign = side == 0 ? 1.0f : -1.0f;

            Editor::Gizmo::DrawLine(cap0 + axes[0] * value.radius * sign,
                cap1 + axes[0] * value.radius * sign,
                ColliderColor, 1.0f, Editor::Gizmo::DrawFlag_AlwaysOnTop);
        }
    }
    else
    {
        DrawShapeEdges(drawn, value.extents, ColliderColor);
    }

    Editor::Gizmo::DrawPoint(foot, ColliderColor, 2.0f);

    if (changed)
        Editor::Gizmo::CommitComponent(frame.scene, frame.entity, frame.component, &value, sizeof(value));
}

//------------------------------------------------------------------------------
/**
    The feature registers what it has, and the editor reads the registry at open. A feature that
    registers nothing is never asked for.
*/
void
RegisterColliderGizmos()
{
    Editor::Gizmo::ComponentGizmo box;
    box.component = Game::GetComponentId<PhysicsFeature::BoxCollider>();
    box.name = "box collider";
    box.draw = Util::Delegate<void(const Editor::Gizmo::GizmoFrame&)>::
        FromFunction<&PhysicsFeature::DrawBoxColliderGizmo>();
    Editor::Gizmo::RegisterComponentGizmo(box);

    Editor::Gizmo::ComponentGizmo sphere;
    sphere.component = Game::GetComponentId<PhysicsFeature::SphereCollider>();
    sphere.name = "sphere collider";
    sphere.draw = Util::Delegate<void(const Editor::Gizmo::GizmoFrame&)>::
        FromFunction<&PhysicsFeature::DrawSphereColliderGizmo>();
    Editor::Gizmo::RegisterComponentGizmo(sphere);

    Editor::Gizmo::ComponentGizmo character;
    character.component = Game::GetComponentId<PhysicsFeature::Character>();
    character.name = "character";
    character.draw = Util::Delegate<void(const Editor::Gizmo::GizmoFrame&)>::
        FromFunction<&PhysicsFeature::DrawCharacterGizmo>();
    Editor::Gizmo::RegisterComponentGizmo(character);
}

//------------------------------------------------------------------------------
/**
    Taken back here rather than by the editor, because the editor does not know who registered what.
*/
void
ClearColliderGizmos()
{
    Editor::Gizmo::UnregisterComponentGizmo(Game::GetComponentId<PhysicsFeature::BoxCollider>());
    Editor::Gizmo::UnregisterComponentGizmo(Game::GetComponentId<PhysicsFeature::SphereCollider>());
    Editor::Gizmo::UnregisterComponentGizmo(Game::GetComponentId<PhysicsFeature::Character>());
}

} // namespace PhysicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
