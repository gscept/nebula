#pragma once
//------------------------------------------------------------------------------
/**
    Navigation::Build geometry sources

    Fills navmesh build sources from resource URNs through the registered
    Physics::GeometryLoader, so the navmesh reads exactly the geometry the
    colliders are cooked from.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "navbuild/navmeshbuilder.h"
#include "util/stringatom.h"
#include "math/quat.h"

namespace Navigation
{
namespace Build
{

/// world transform of a collider shape, placed like physics places it: the entity's world position and
/// orientation composed with the collider offset. Scale is ignored, physics doesn't scale collider shapes
Math::mat4 ColliderTransform(const Math::vec3& position, const Math::quat& orientation, const Math::vec3& localPosition, const Math::quat& localOrientation);
/// convex hull of points seen from above, in x and z, counter clockwise. The y of the points is kept
Util::Array<Math::vec3> ConvexHullXZ(const Util::Array<Math::vec3>& points);

/// the vertical prism an area volume marks
struct AreaPrism
{
    Util::Array<Math::vec3> outline;    // convex, counter clockwise seen from above, y is 0
    float minY = 0.0f;
    float maxY = 0.0f;
};
/// the prism an area volume marks. With fewer than 3 points it is the prism around the box. Otherwise the points
/// are the outline, in order, and the height span covers every point give or take half the height of the box, so
/// the surface a point was placed on is inside. The outline may be concave. Points are transformed by pointTransform,
/// box corners by boxTransform
AreaPrism MakeAreaPrism(const Math::mat4& boxTransform, const Math::bbox& localBox, const Math::mat4& pointTransform, const Math::vec3* points, SizeT pointCount);
/// the outline of a box seen from above, the hull of its corners, in world space
Util::Array<Math::vec3> BoxOutlineXZ(const Math::mat4& boxTransform, const Math::bbox& localBox);
/// where a point goes into an outline: the index of the edge closest to it seen from above, the point is
/// inserted after that index, between the two outline points of the edge
IndexT OutlineInsertIndex(const Math::vec3* outline, SizeT count, const Math::vec3& point);
/// true if no two edges of the outline cross each other seen from above
bool IsSimpleOutlineXZ(const Math::vec3* outline, SizeT count);

/// intersect a ray with the detail surface of a navmesh, t is in units of dir. Returns false if it misses
bool RaycastNavMesh(const dtNavMesh& navMesh, const Math::point& start, const Math::vector& dir, float& outT);
/// fill a mesh source from a mesh URN, folder:file:msh, like a MeshCollider reads it
bool LoadMeshSource(Util::StringAtom resource, IndexT meshIndex, IndexT primGroup, const Math::mat4& transform, MeshSource& out);
/// fill a height field source from a heightmap URN, folder:file:tex, like a HeightFieldCollider reads it
bool LoadHeightFieldSource(Util::StringAtom resource, float heightRange, float sizeX, float sizeZ, const Math::mat4& transform, HeightFieldSource& out);

} // namespace Build
} // namespace Navigation
