#pragma once
//------------------------------------------------------------------------------
/**
    A visibility system describes some virtual representation of the scene, 
    wherein objects are searchable for visibility. This is coarse grained
    visibility, and does not account for any pixel-perfect techniques like
    occlusion culling.

    Some systems are procedural (Octree and Quadtree) while other systems require
    authoring. 
    
    Different systems have different strenghts, as described here:

    Box system:
        Fully authored visibility system where objects are grouped into boxes,
        so if the box is visible, all objects within are too. Useful for big objects,
        or areas, but for many small objects this might result in drawing too much not in the screen.

    Octree system:
        3D subdivision of the scene into incrementally smaller regions. Useful for large game worlds,
        where movement is small or none, such as terrain or static objects. Octrees also support difference
        in height, meaning they are more suitable than Quadtrees for shooters or platformers.

    Portal system:
        Produces frustums for 'portals', which in a linked-list manner resolves visibility only
        for things seen through said portals. Useful for indoor environments where there are 
        many occluders. 

    Quadtree system:
        Like Octree but 2D, meaning there is only a 2D grid. Useful for big worlds where
        there is no importance of visibility along one axis (isomorphic RPG, 2D scroller)

    Bruteforce system:
        Doesn't do anything but view frustum culling on everything in the scene.

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "math/mat4.h"
#include "jobs/jobs.h"
#include "math/bbox.h"
#include "resources/resourceid.h"
#include "graphics/graphicsentity.h"
namespace Visibility
{

struct BoxSystemLoadInfo
{
    Resources::ResourceName path;   // path to authored box system
};

struct PortalSystemLoadInfo
{
    Resources::ResourceName path;   // path to authored portal system
};

struct OctreeSystemLoadInfo
{
    bool worldExpanding;            // if true, will cover the entire game world, and the next fields are unused
    uint cellsX, cellsY, cellsZ;    // how many cells per unit dimension
    uint width, height, depth;      // total size of the tree
    Math::vec4 pos;                 // center position of tree
};

struct QuadtreeSystemLoadInfo
{
    bool worldExpanding;            // if true, will cover the entire game world, and the next fields are unused
    uint cellsX, cellsY;            // how many cells per unit dimension
    uint width, height;             // total size of the tree
    Math::vec4 pos;                 // center position of tree
};

struct BruteforceSystemLoadInfo
{
    // empty on purpose
};

class VisibilitySystem
{
public:

    /// setup observers
    virtual void PrepareObservers(const Math::mat4* transforms, Math::ClipStatus::Type* const* vis, const SizeT count);
    /// prepare system with entities to insert into the structure
    virtual void PrepareEntities(const Math::mat4* transforms, const Graphics::GraphicsEntityId* entities, const bool* activeFlags, const SizeT count);
    /// run system
    virtual void Run();

protected:

    Math::vec3 center;
    Math::bbox boundingbox;

    struct Observer
    {
        const Math::mat4* transforms;
        Math::ClipStatus::Type* const* vis;
        SizeT count;
    } obs;

    struct Entity
    {
        const Math::mat4* transforms;
        const Graphics::GraphicsEntityId* entities;
        const bool* activeFlags;
        SizeT count;
    } ent;
};

} // namespace Visibility
