#pragma once
//------------------------------------------------------------------------------
/**
    Queries on a loaded navmesh

    Points are found on the surface of a navmesh, paths follow it from one point to
    another. A navmesh is picked with NavigationFeature::FindNavMesh, by the agent kind
    it was baked for.

    A filter decides which polygons a query may use, and what walking on each area costs.
    The default filter has the costs of the area table and lets every polygon with a flag
    through, a polygon of an area without flags is never used.

    Every navmesh has one Detour query, the functions run on the game thread only.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "math/point.h"
#include "math/vec3.h"
#include "util/array.h"
#include "navigationfeature/streamnavmeshcache.h"

namespace NavigationFeature
{

struct QueryFilter
{
    /// cost of walking a unit on each area, by area id
    float areaCost[64];
    /// a polygon is used if it has one of these flags
    uint16_t includeFlags = 0xffff;
    /// and none of these
    uint16_t excludeFlags = 0;
};

enum class PathStatus
{
    /// the path reaches the end
    Found,
    /// the end can't be reached, the path ends at the point closest to it
    Partial,
    /// there is no navmesh surface near the start
    NoStart,
    /// there is no navmesh surface near the end
    NoEnd,
    /// the navmesh isn't loaded, or the search failed
    Failed
};

struct NavPath
{
    PathStatus status = PathStatus::Failed;
    /// the corners of the path on the surface, from the start to the end
    Util::Array<Math::point> points;
    /// the area of each segment, from points[i] to points[i + 1]
    Util::Array<unsigned char> areas;
    /// the length of the path
    float length = 0.0f;
    /// the length of every segment times the cost of its area
    float cost = 0.0f;
};

/// a filter with the area costs of the area table, every flag included
QueryFilter DefaultQueryFilter();
/// set the cost of an area by name, false if the area table has no such area
bool SetAreaCost(QueryFilter& filter, const Util::String& area, float cost);

/// half the size of the box points are looked for in, from the agent kind of the navmesh
Math::vec3 SearchExtents(Navigation::NavMeshId navMesh);
/// the point on the surface closest to a position, within the extents around it. Returns false if there is none
bool FindNearestPoint(Navigation::NavMeshId navMesh, const Math::point& position, const Math::vec3& extents, Math::point& outPoint, const QueryFilter* filter = nullptr);
/// a path between two positions, which are moved to the closest surface within the search extents first
NavPath FindPath(Navigation::NavMeshId navMesh, const Math::point& start, const Math::point& end, const QueryFilter* filter = nullptr);
/// walk along the surface from start towards end. Returns true and the point where it is blocked if an edge of the navmesh is in the way
bool Raycast(Navigation::NavMeshId navMesh, const Math::point& start, const Math::point& end, Math::point& outHit, const QueryFilter* filter = nullptr);
/// a random point on the surface, every area of surface is as likely. Returns false if the navmesh has none the filter lets through
bool RandomPoint(Navigation::NavMeshId navMesh, Math::point& outPoint, const QueryFilter* filter = nullptr);
/// a random point on the surface which is connected to the center, on a polygon the circle touches, so it may be up to a polygon outside the radius. Returns false if there is none
bool RandomPointAround(Navigation::NavMeshId navMesh, const Math::point& center, float radius, Math::point& outPoint, const QueryFilter* filter = nullptr);

} // namespace NavigationFeature
