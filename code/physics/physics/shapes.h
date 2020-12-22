#pragma once
//------------------------------------------------------------------------------
/**
    Physics shape enums

    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

namespace Physics
{

enum CollsionGroups
{
    Default = 0,    
    Static = 1,
    Kinematic = 3,
    Debris = 4,
    SensorTrigger = 5,
    Characters = 6,    
};

enum MeshTopologyType
{
    Convex = 0,    
    ConvexHull = 1,
    Triangles = 2,
    HeightField = 3,
};

enum ColliderType
{
    ColliderSphere = 0,
    ColliderCube = 1,    
    ColliderCapsule = 2,
    ColliderPlane = 3,
    ColliderMesh = 4
};
}