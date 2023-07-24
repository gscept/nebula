//------------------------------------------------------------------------------
//  clustering.fxh
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#ifndef CLUSTERING_FXH
#define CLUSTERING_FXH

#define CHECK_FLAG(bits, bit) ((bits & bit) == bit)

//------------------------------------------------------------------------------
/**
    Calculate 3D index from screen position and depth
*/
uint3 
CalculateClusterIndex(vec2 screenPos, float depth, float scale, float bias)
{
    uint i = uint(screenPos.x);
    uint j = uint(screenPos.y);
    uint k = uint(log2(-depth) * scale + bias);

    return uint3(i, j, k);
}

//------------------------------------------------------------------------------
/**
    Calculate view Z from world space
*/
float
CalculateViewDepth(mat4 view, vec3 worldPos)
{
    vec4 m = vec4(view[0][2], view[1][2], view[2][2], view[3][2]);
    return dot(m, vec4(worldPos, 1));
}


//------------------------------------------------------------------------------
/**
*/
bool
TestAABBAABB(ClusterAABB aabb, vec3 bboxMin, vec3 bboxMax)
{
    // this expression can be unfolded like this:
    //	C = min x, y, z in A must be smaller than the max x, y, z in B
    //	D = max x, y, z in A must be bigger than the min x, y, z in B
    //	E = C equals D
    //	return if all members of E are true
    return all(equal(lessThan(aabb.minPoint.xyz, bboxMax), greaterThan(aabb.maxPoint.xyz, bboxMin)));
}

//------------------------------------------------------------------------------
/**
*/
bool
TestAABBSphere(ClusterAABB aabb, vec3 pos, float radius)
{
    float sqDist = 0.0f;
    for (int i = 0; i < 3; i++)
    {
        float v = (pos)[i];

        if (v < aabb.minPoint[i]) sqDist += pow(aabb.minPoint[i] - v, 2);
        if (v > aabb.maxPoint[i]) sqDist += pow(v - aabb.maxPoint[i], 2);
    }
    return sqDist <= radius * radius;
}

//------------------------------------------------------------------------------
/**
    Treat AABB as a sphere for simplicity of intersection detection.

    https://bartwronski.com/2017/04/13/cull-that-cone/
*/
bool
TestAABBCone(ClusterAABB aabb, vec3 pos, vec3 forward, float radius, vec2 sinCosAngles)
{
    float3 aabbExtents = (aabb.maxPoint.xyz - aabb.minPoint.xyz) * 0.5f;
    float3 aabbCenter = aabb.minPoint.xyz + aabbExtents;
    float aabbRadius = aabb.maxPoint.w;

    float3 v = aabbCenter - pos;
    const float vlensq = dot(v, v);
    const float v1len = dot(v, -forward);
    const float distanceClosestPoint = sinCosAngles.y * sqrt(vlensq - v1len * v1len) - v1len * sinCosAngles.x;

    const bool angleCull = distanceClosestPoint > aabbRadius;
    const bool frontCull = v1len > aabbRadius + radius;
    const bool backCull = v1len < -aabbRadius;
    return !(angleCull || backCull || frontCull);
}

//------------------------------------------------------------------------------
/**
*/
bool 
TestAABBOBB(ClusterAABB aabb, mat4 obb)
{
    return true;
}


//------------------------------------------------------------------------------
/**
*/
bool
TestAABBOrthoProjection(ClusterAABB aabb, mat4 viewProjection)
{
    vec4 points[8] = {
        vec4(aabb.minPoint.xyz, 1.0f)                                          // near bottom left
        , vec4(aabb.maxPoint.x, aabb.minPoint.yz, 1.0f)                        // near bottom right
        , vec4(aabb.minPoint.x, aabb.maxPoint.y, aabb.minPoint.z, 1.0f)        // near top left
        , vec4(aabb.maxPoint.xy, aabb.minPoint.z, 1.0f)                        // near top right
        , vec4(aabb.minPoint.xy, aabb.maxPoint.z, 1.0f)                        // far bottom left
        , vec4(aabb.maxPoint.x, aabb.minPoint.y, aabb.maxPoint.z, 1.0f)        // far bottom right
        , vec4(aabb.minPoint.x, aabb.maxPoint.yz, 1.0f)                        // far top left
        , vec4(aabb.maxPoint.xyz, 1.0f)                                        // far top right
    };
    
    for (int i = 0; i < 8; i++)
    {
        // Transform cluster AABB points to clip space
        points[i] = points[i] * viewProjection;

        // Simply check if interlaps overlap with the unit cube
        if (all(equal(greaterThan(points[i].xyz, vec3(-1.0)), lessThan(points[i].xyz, vec3(1.0)))))
            return true;
    }    
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
TestAABBPerspectiveProjection(ClusterAABB aabb, mat4 viewProjection)
{
    vec4 points[8] = {
        vec4(aabb.minPoint.xyz, 1.0f)                                          // near bottom left
        , vec4(aabb.maxPoint.x, aabb.minPoint.yz, 1.0f)                        // near bottom right
        , vec4(aabb.minPoint.x, aabb.maxPoint.y, aabb.minPoint.z, 1.0f)        // near top left
        , vec4(aabb.maxPoint.xy, aabb.minPoint.z, 1.0f)                        // near top right
        , vec4(aabb.minPoint.xy, aabb.maxPoint.z, 1.0f)                        // far bottom left
        , vec4(aabb.maxPoint.x, aabb.minPoint.y, aabb.maxPoint.z, 1.0f)        // far bottom right
        , vec4(aabb.minPoint.x, aabb.maxPoint.yz, 1.0f)                        // far top left
        , vec4(aabb.maxPoint.xyz, 1.0f)                                        // far top right
    };

    uint mask = 0xffff;
    for (int i = 0; i < 2; i++)
    {
        // Transform cluster AABB points to clip space
        points[i * 4] = viewProjection * points[i * 4];
        points[i * 4 + 1] = viewProjection * points[i * 4 + 1];
        points[i * 4 + 2] = viewProjection * points[i * 4 + 2];
        points[i * 4 + 3] = viewProjection * points[i * 4 + 3];


        vec4 xs = vec4(points[i].x, points[i * 4 + 1].x, points[i * 4 + 2].x, points[i * 4 + 3].x);
        vec4 ys = vec4(points[i].y, points[i * 4 + 1].y, points[i * 4 + 2].y, points[i * 4 + 3].y);
        vec4 zs = vec4(points[i].z, points[i * 4 + 1].z, points[i * 4 + 2].z, points[i * 4 + 3].z);
        vec4 ws = vec4(points[i].w, points[i * 4 + 1].w, points[i * 4 + 2].w, points[i * 4 + 3].w);

        uvec4 flags =
            uvec4(lessThan(xs, -ws)) * uvec4(0x1)
            + uvec4(greaterThan(xs, ws)) * uvec4(0x2)
            + uvec4(lessThan(ys, -ws)) * uvec4(0x4)
            + uvec4(greaterThan(ys, ws)) * uvec4(0x8)
            + uvec4(lessThan(zs, -ws)) * uvec4(0x10)
            + uvec4(greaterThan(zs, ws)) * uvec4(0x20)
            ;

        mask &= flags.x;
        mask &= flags.y;
        mask &= flags.z;
        mask &= flags.w;


        /*
        vec3 lt = lessThan(points[i].xyz, vec3(-points[i].w));
        vec3 gt = greaterThan(points[i].xyz, vec3(points[i].w));
        mask &= any(lt);
        mask &= any(gt);
        // Without doing 1/w, simply check if intervals going from max to min overlaps with the range of w
        if (points[i].x < -points[i].w)
            mask &= 1 << 0;
        if (points[i].x > points[i].w)
            mask &= 1 << 1;
        if (points[i].y < -points[i].w)
            mask &= 1 << 2;
        if (points[i].y > points[i].w)
            mask &= 1 << 3;
        if (points[i].z < -points[i].w)
            mask &= 1 << 4;
        if (points[i].z > points[i].w)
            mask &= 1 << 5;
        */

    }
    return mask == 0;
}

#endif