//------------------------------------------------------------------------------
//  @file geometrysources.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "navbuild/geometrysources.h"
#include "physics/geometry.h"
#include <algorithm>
#include <cfloat>
#include "DetourNavMesh.h"

namespace Navigation
{
namespace Build
{

//------------------------------------------------------------------------------
/**
*/
Math::mat4
ColliderTransform(const Math::vec3& position, const Math::quat& orientation, const Math::vec3& localPosition, const Math::quat& localOrientation)
{
    const Math::mat4 entity = Math::trs(position, orientation, Math::vec3(1.0f));
    const Math::mat4 local = Math::trs(localPosition, localOrientation, Math::vec3(1.0f));
    // the offset is applied in the space of the entity
    return entity * local;
}

//------------------------------------------------------------------------------
/**
    Monotone chain
*/
Util::Array<Math::vec3>
ConvexHullXZ(const Util::Array<Math::vec3>& input)
{
    Util::Array<Math::vec3> points = input;
    std::sort(points.begin(), points.end(), [](const Math::vec3& a, const Math::vec3& b)
    {
        return a.x < b.x || (a.x == b.x && a.z < b.z);
    });
    auto cross = [](const Math::vec3& o, const Math::vec3& a, const Math::vec3& b)
    {
        return (a.x - o.x) * (b.z - o.z) - (a.z - o.z) * (b.x - o.x);
    };
    Util::Array<Math::vec3> hull;
    if (points.Size() < 3)
    {
        return points;
    }
    for (int pass = 0; pass < 2; pass++)
    {
        const SizeT start = hull.Size();
        for (IndexT k = 0; k < points.Size(); k++)
        {
            const Math::vec3& p = pass == 0 ? points[k] : points[points.Size() - 1 - k];
            // collinear and duplicate points are dropped
            while (hull.Size() >= start + 2 && cross(hull[hull.Size() - 2], hull[hull.Size() - 1], p) <= 0.0f)
            {
                hull.EraseIndex(hull.Size() - 1);
            }
            hull.Append(p);
        }
        // the last point is the first point of the other half
        hull.EraseIndex(hull.Size() - 1);
    }
    return hull;
}

//------------------------------------------------------------------------------
/**
*/
AreaPrism
MakeAreaPrism(const Math::mat4& boxTransform, const Math::bbox& localBox, const Math::mat4& pointTransform, const Math::vec3* points, SizeT pointCount)
{
    AreaPrism prism;
    float boxMinY = FLT_MAX, boxMaxY = -FLT_MAX;
    for (int i = 0; i < 8; i++)
    {
        const Math::vec4 p = boxTransform * Math::vec4(localBox.corner_point(i));
        boxMinY = Math::min(boxMinY, p.y);
        boxMaxY = Math::max(boxMaxY, p.y);
    }
    if (pointCount < 3)
    {
        prism.outline = BoxOutlineXZ(boxTransform, localBox);
        prism.minY = boxMinY;
        prism.maxY = boxMaxY;
        return prism;
    }

    const float halfHeight = (boxMaxY - boxMinY) * 0.5f;
    prism.minY = FLT_MAX;
    prism.maxY = -FLT_MAX;
    for (SizeT i = 0; i < pointCount; i++)
    {
        const Math::vec4 p = pointTransform * Math::vec4(points[i].x, points[i].y, points[i].z, 1.0f);
        prism.outline.Append(Math::vec3(p.x, 0.0f, p.z));
        prism.minY = Math::min(prism.minY, p.y - halfHeight);
        prism.maxY = Math::max(prism.maxY, p.y + halfHeight);
    }
    return prism;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Math::vec3>
BoxOutlineXZ(const Math::mat4& boxTransform, const Math::bbox& localBox)
{
    Util::Array<Math::vec3> corners;
    for (int i = 0; i < 8; i++)
    {
        const Math::vec4 p = boxTransform * Math::vec4(localBox.corner_point(i));
        corners.Append(Math::vec3(p.x, 0.0f, p.z));
    }
    return ConvexHullXZ(corners);
}

//------------------------------------------------------------------------------
/**
*/
static float
SegmentDistanceSqXZ(const Math::vec3& p, const Math::vec3& a, const Math::vec3& b)
{
    const float abx = b.x - a.x, abz = b.z - a.z;
    const float lengthSq = abx * abx + abz * abz;
    float t = lengthSq > 0.0f ? ((p.x - a.x) * abx + (p.z - a.z) * abz) / lengthSq : 0.0f;
    t = Math::clamp(t, 0.0f, 1.0f);
    const float dx = a.x + abx * t - p.x, dz = a.z + abz * t - p.z;
    return dx * dx + dz * dz;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
OutlineInsertIndex(const Math::vec3* outline, SizeT count, const Math::vec3& point)
{
    IndexT best = count > 0 ? count - 1 : 0;
    float bestDistance = FLT_MAX;
    for (SizeT i = 0; i < count; i++)
    {
        const float distance = SegmentDistanceSqXZ(point, outline[i], outline[(i + 1) % count]);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            best = i;
        }
    }
    return best;
}

//------------------------------------------------------------------------------
/**
*/
static bool
SegmentsCrossXZ(const Math::vec3& a, const Math::vec3& b, const Math::vec3& c, const Math::vec3& d)
{
    auto orient = [](const Math::vec3& p, const Math::vec3& q, const Math::vec3& r)
    {
        return (q.x - p.x) * (r.z - p.z) - (q.z - p.z) * (r.x - p.x);
    };
    const float d1 = orient(c, d, a), d2 = orient(c, d, b);
    const float d3 = orient(a, b, c), d4 = orient(a, b, d);
    return ((d1 > 0.0f) != (d2 > 0.0f)) && ((d3 > 0.0f) != (d4 > 0.0f)) && d1 != 0.0f && d2 != 0.0f && d3 != 0.0f && d4 != 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
bool
IsSimpleOutlineXZ(const Math::vec3* outline, SizeT count)
{
    for (SizeT i = 0; i < count; i++)
    {
        for (SizeT j = i + 1; j < count; j++)
        {
            // edges sharing a point touch, they don't cross
            if (j == i + 1 || (i == 0 && j == count - 1))
            {
                continue;
            }
            if (SegmentsCrossXZ(outline[i], outline[(i + 1) % count], outline[j], outline[(j + 1) % count]))
            {
                return false;
            }
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
static bool
RayTriangle(const Math::vec3& o, const Math::vec3& d, const float* a, const float* b, const float* c, float& t)
{
    const Math::vec3 va(a[0], a[1], a[2]);
    const Math::vec3 e1 = Math::vec3(b[0], b[1], b[2]) - va;
    const Math::vec3 e2 = Math::vec3(c[0], c[1], c[2]) - va;
    const Math::vec3 p = Math::cross(d, e2);
    const float det = Math::dot(e1, p);
    // both sides count, the surface is picked from wherever the camera is
    if (Math::abs(det) < 1e-12f)
    {
        return false;
    }
    const float invDet = 1.0f / det;
    const Math::vec3 s = o - va;
    const float u = Math::dot(s, p) * invDet;
    if (u < 0.0f || u > 1.0f)
    {
        return false;
    }
    const Math::vec3 q = Math::cross(s, e1);
    const float v = Math::dot(d, q) * invDet;
    if (v < 0.0f || u + v > 1.0f)
    {
        return false;
    }
    t = Math::dot(e2, q) * invDet;
    return t >= 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
static bool
RayBox(const Math::vec3& o, const Math::vec3& d, const float* bmin, const float* bmax)
{
    float tmin = 0.0f, tmax = FLT_MAX;
    const float origin[3] = { o.x, o.y, o.z };
    const float dir[3] = { d.x, d.y, d.z };
    for (int i = 0; i < 3; i++)
    {
        if (Math::abs(dir[i]) < 1e-12f)
        {
            if (origin[i] < bmin[i] || origin[i] > bmax[i])
            {
                return false;
            }
            continue;
        }
        float t0 = (bmin[i] - origin[i]) / dir[i];
        float t1 = (bmax[i] - origin[i]) / dir[i];
        if (t0 > t1)
        {
            std::swap(t0, t1);
        }
        tmin = Math::max(tmin, t0);
        tmax = Math::min(tmax, t1);
        if (tmin > tmax)
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Walks the detail triangles of every polygon, the same triangles Detour gives heights with
*/
bool
RaycastNavMesh(const dtNavMesh& navMesh, const Math::point& start, const Math::vector& dir, float& outT)
{
    const Math::vec3 o(start.x, start.y, start.z);
    const Math::vec3 d(dir.x, dir.y, dir.z);
    bool hit = false;
    outT = FLT_MAX;
    for (int i = 0; i < navMesh.getMaxTiles(); i++)
    {
        const dtMeshTile* tile = navMesh.getTile(i);
        if (tile == nullptr || tile->header == nullptr || !RayBox(o, d, tile->header->bmin, tile->header->bmax))
        {
            continue;
        }
        for (int p = 0; p < tile->header->polyCount; p++)
        {
            const dtPoly& poly = tile->polys[p];
            if (poly.getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
            {
                continue;
            }
            const dtPolyDetail& detail = tile->detailMeshes[p];
            for (int j = 0; j < detail.triCount; j++)
            {
                const unsigned char* tri = &tile->detailTris[(detail.triBase + j) * 4];
                const float* v[3];
                for (int k = 0; k < 3; k++)
                {
                    v[k] = tri[k] < poly.vertCount
                        ? &tile->verts[poly.verts[tri[k]] * 3]
                        : &tile->detailVerts[(detail.vertBase + (tri[k] - poly.vertCount)) * 3];
                }
                float t;
                if (RayTriangle(o, d, v[0], v[1], v[2], t) && t < outT)
                {
                    outT = t;
                    hit = true;
                }
            }
        }
    }
    return hit;
}

//------------------------------------------------------------------------------
/**
*/
bool
LoadMeshSource(Util::StringAtom resource, IndexT meshIndex, IndexT primGroup, const Math::mat4& transform, MeshSource& out)
{
    const Physics::GeometryLoader& loader = Physics::GetGeometryLoader();
    if (loader.LoadMesh == nullptr)
    {
        n_warning("LoadMeshSource: no geometry loader registered, cannot read '%s'\n", resource.Value());
        return false;
    }

    Physics::MeshGeometryInput input;
    if (!loader.LoadMesh(resource, meshIndex, primGroup, input))
    {
        return false;
    }
    const SizeT indexSize = Physics::IndexSize(input.indexType);
    if (indexSize == 0 || input.indices.Size() < input.indexCount * indexSize || input.positions.Size() < input.vertexCount * 3)
    {
        n_warning("LoadMeshSource: '%s' returned inconsistent geometry\n", resource.Value());
        return false;
    }

    out.positions.Clear();
    out.positions.AppendArray(input.positions.Begin(), input.vertexCount * 3);
    out.indices.Clear();
    out.indices.Reserve(input.indexCount);
    for (SizeT i = 0; i < input.indexCount; i++)
    {
        const byte* index = input.indices.Begin() + i * indexSize;
        if (input.indexType == Physics::IndexType::Index16)
        {
            uint16_t value;
            memcpy(&value, index, sizeof(value));
            out.indices.Append(value);
        }
        else
        {
            uint32_t value;
            memcpy(&value, index, sizeof(value));
            out.indices.Append(value);
        }
    }
    out.transform = transform;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
LoadHeightFieldSource(Util::StringAtom resource, float heightRange, float sizeX, float sizeZ, const Math::mat4& transform, HeightFieldSource& out)
{
    const Physics::GeometryLoader& loader = Physics::GetGeometryLoader();
    if (loader.LoadHeightField == nullptr)
    {
        n_warning("LoadHeightFieldSource: no geometry loader registered, cannot read '%s'\n", resource.Value());
        return false;
    }

    Physics::HeightFieldInput input;
    if (!loader.LoadHeightField(resource, input))
    {
        return false;
    }

    out.width = input.width;
    out.height = input.height;
    out.samples = std::move(input.samples);
    out.heightRange = heightRange;
    out.sizeX = sizeX;
    out.sizeZ = sizeZ;
    out.transform = transform;
    return true;
}

} // namespace Build
} // namespace Navigation
