//------------------------------------------------------------------------------
//  navquery.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "navquery.h"
#include "navigationfeatureunit.h"
#include "navbuild/navareas.h"
#include "math/scalar.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourCommon.h"
#include "profiling/profiling.h"

namespace Navigation
{
extern StreamNavMeshCache* navMeshCache;
}

namespace NavigationFeature
{

static constexpr int MaxPathPolys = 512;
static constexpr int MaxPathPoints = 512;

//------------------------------------------------------------------------------
/**
*/
QueryFilter
DefaultQueryFilter()
{
    QueryFilter filter;
    for (int i = 0; i < Navigation::Build::MaxAreas; i++)
    {
        filter.areaCost[i] = 1.0f;
    }
    for (const Navigation::Build::AreaDefinition& area : GetAreaTable().Areas())
    {
        filter.areaCost[area.id] = area.cost;
    }
    return filter;
}

//------------------------------------------------------------------------------
/**
*/
bool
SetAreaCost(QueryFilter& filter, const Util::String& area, float cost)
{
    const Navigation::Build::AreaDefinition* definition = GetAreaTable().Find(area);
    if (definition == nullptr)
    {
        return false;
    }
    filter.areaCost[definition->id] = cost;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
static dtQueryFilter
ToDetour(const QueryFilter* filter)
{
    const QueryFilter source = filter != nullptr ? *filter : DefaultQueryFilter();
    dtQueryFilter out;
    for (int i = 0; i < Navigation::Build::MaxAreas; i++)
    {
        out.setAreaCost(i, source.areaCost[i]);
    }
    out.setIncludeFlags(source.includeFlags);
    out.setExcludeFlags(source.excludeFlags);
    return out;
}

//------------------------------------------------------------------------------
/**
*/
static dtNavMeshQuery*
GetQuery(Navigation::NavMeshId navMesh)
{
    if (navMesh == Navigation::InvalidNavMeshId || Navigation::navMeshCache == nullptr)
    {
        return nullptr;
    }
    return Navigation::navMeshCache->GetQuery(navMesh);
}

//------------------------------------------------------------------------------
/**
*/
static Math::point
ToPoint(const float* v)
{
    return Math::point(v[0], v[1], v[2]);
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
SearchExtents(Navigation::NavMeshId navMesh)
{
    if (navMesh == Navigation::InvalidNavMeshId || Navigation::navMeshCache == nullptr)
    {
        return Math::vec3(1.0f, 2.0f, 1.0f);
    }
    const Navigation::AgentT& agent = Navigation::navMeshCache->GetAgent(navMesh);
    // a point up to an agent height above or below the surface, a couple of radii beside it
    const float side = Math::max(agent.agent_radius * 2.0f, 0.5f);
    const float up = Math::max(agent.agent_height, 1.0f);
    return Math::vec3(side, up, side);
}

//------------------------------------------------------------------------------
/**
*/
static bool
NearestPoly(dtNavMeshQuery* query, const dtQueryFilter& filter, const Math::point& position, const Math::vec3& extents, dtPolyRef& ref, float* nearest)
{
    const float center[3] = { position.x, position.y, position.z };
    const float halfExtents[3] = { extents.x, extents.y, extents.z };
    ref = 0;
    return dtStatusSucceed(query->findNearestPoly(center, halfExtents, &filter, &ref, nearest)) && ref != 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
FindNearestPoint(Navigation::NavMeshId navMesh, const Math::point& position, const Math::vec3& extents, Math::point& outPoint, const QueryFilter* filter)
{
    N_SCOPE_ACCUM(NavFindNearestPoint, Navigation);
    dtNavMeshQuery* query = GetQuery(navMesh);
    if (query == nullptr)
    {
        return false;
    }
    const dtQueryFilter detourFilter = ToDetour(filter);
    dtPolyRef ref;
    float nearest[3];
    if (!NearestPoly(query, detourFilter, position, extents, ref, nearest))
    {
        return false;
    }
    outPoint = ToPoint(nearest);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
NavPath
FindPath(Navigation::NavMeshId navMesh, const Math::point& start, const Math::point& end, const QueryFilter* filter)
{
    N_SCOPE_ACCUM(NavFindPath, Navigation);
    NavPath path;
    dtNavMeshQuery* query = GetQuery(navMesh);
    if (query == nullptr)
    {
        return path;
    }

    const QueryFilter source = filter != nullptr ? *filter : DefaultQueryFilter();
    const dtQueryFilter detourFilter = ToDetour(&source);
    const Math::vec3 extents = SearchExtents(navMesh);

    dtPolyRef startRef, endRef;
    float startPos[3], endPos[3];
    if (!NearestPoly(query, detourFilter, start, extents, startRef, startPos))
    {
        path.status = PathStatus::NoStart;
        return path;
    }
    if (!NearestPoly(query, detourFilter, end, extents, endRef, endPos))
    {
        path.status = PathStatus::NoEnd;
        return path;
    }

    dtPolyRef polys[MaxPathPolys];
    int polyCount = 0;
    const dtStatus status = query->findPath(startRef, endRef, startPos, endPos, &detourFilter, polys, &polyCount, MaxPathPolys);
    if (dtStatusFailed(status) || polyCount == 0)
    {
        return path;
    }

    // a partial path ends in the polygon closest to the end, the point closest to it on that polygon
    float targetPos[3];
    dtVcopy(targetPos, endPos);
    const bool partial = polys[polyCount - 1] != endRef || dtStatusDetail(status, DT_PARTIAL_RESULT);
    if (polys[polyCount - 1] != endRef)
    {
        query->closestPointOnPoly(polys[polyCount - 1], endPos, targetPos, nullptr);
    }

    // corners where the area changes too, so every segment has one area
    float points[MaxPathPoints * 3];
    unsigned char flags[MaxPathPoints];
    dtPolyRef refs[MaxPathPoints];
    int pointCount = 0;
    if (dtStatusFailed(query->findStraightPath(startPos, targetPos, polys, polyCount, points, flags, refs, &pointCount, MaxPathPoints, DT_STRAIGHTPATH_AREA_CROSSINGS)) || pointCount == 0)
    {
        return path;
    }

    const dtNavMesh* mesh = query->getAttachedNavMesh();
    for (int i = 0; i < pointCount; i++)
    {
        path.points.Append(ToPoint(points + i * 3));
        if (i + 1 < pointCount)
        {
            // the polygon a corner leads into is the one the segment after it crosses
            unsigned char area = 0;
            mesh->getPolyArea(refs[i], &area);
            path.areas.Append(area);
            const float length = dtVdist(points + i * 3, points + (i + 1) * 3);
            path.length += length;
            path.cost += length * source.areaCost[area & (Navigation::Build::MaxAreas - 1)];
        }
    }
    path.status = partial ? PathStatus::Partial : PathStatus::Found;
    return path;
}

//------------------------------------------------------------------------------
/**
*/
bool
Raycast(Navigation::NavMeshId navMesh, const Math::point& start, const Math::point& end, Math::point& outHit, const QueryFilter* filter)
{
    N_SCOPE_ACCUM(NavRaycast, Navigation);
    outHit = end;
    dtNavMeshQuery* query = GetQuery(navMesh);
    if (query == nullptr)
    {
        return false;
    }
    const dtQueryFilter detourFilter = ToDetour(filter);
    dtPolyRef startRef;
    float startPos[3];
    if (!NearestPoly(query, detourFilter, start, SearchExtents(navMesh), startRef, startPos))
    {
        return false;
    }

    const float endPos[3] = { end.x, end.y, end.z };
    float t = 0.0f;
    float normal[3];
    dtPolyRef polys[MaxPathPolys];
    int polyCount = 0;
    if (dtStatusFailed(query->raycast(startRef, startPos, endPos, &detourFilter, &t, normal, polys, &polyCount, MaxPathPolys)))
    {
        return false;
    }
    if (t >= 1.0f || polyCount == 0)
    {
        return false;
    }

    // the hit is on the line in the plane, its height is the surface of the last polygon
    float hit[3];
    dtVlerp(hit, startPos, endPos, t);
    float height = hit[1];
    if (dtStatusSucceed(query->getPolyHeight(polys[polyCount - 1], hit, &height)))
    {
        hit[1] = height;
    }
    outHit = ToPoint(hit);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
static float
RandomUnit()
{
    return Math::rand();
}

//------------------------------------------------------------------------------
/**
*/
bool
RandomPoint(Navigation::NavMeshId navMesh, Math::point& outPoint, const QueryFilter* filter)
{
    N_SCOPE_ACCUM(NavRandomPoint, Navigation);
    dtNavMeshQuery* query = GetQuery(navMesh);
    if (query == nullptr)
    {
        return false;
    }
    const dtQueryFilter detourFilter = ToDetour(filter);
    dtPolyRef ref = 0;
    float point[3];
    if (dtStatusFailed(query->findRandomPoint(&detourFilter, RandomUnit, &ref, point)) || ref == 0)
    {
        return false;
    }
    outPoint = ToPoint(point);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
RandomPointAround(Navigation::NavMeshId navMesh, const Math::point& center, float radius, Math::point& outPoint, const QueryFilter* filter)
{
    N_SCOPE_ACCUM(NavRandomPointAround, Navigation);
    dtNavMeshQuery* query = GetQuery(navMesh);
    if (query == nullptr)
    {
        return false;
    }
    const dtQueryFilter detourFilter = ToDetour(filter);
    dtPolyRef startRef;
    float startPos[3];
    if (!NearestPoly(query, detourFilter, center, SearchExtents(navMesh), startRef, startPos))
    {
        return false;
    }
    dtPolyRef ref = 0;
    float point[3];
    if (dtStatusFailed(query->findRandomPointAroundCircle(startRef, startPos, radius, &detourFilter, RandomUnit, &ref, point)) || ref == 0)
    {
        return false;
    }
    outPoint = ToPoint(point);
    return true;
}

} // namespace NavigationFeature
