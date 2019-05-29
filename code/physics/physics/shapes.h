#pragma once


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
    MeshConvex = 0,
    MeshConcave = 1,
    MeshConvexHull = 2,
    MeshStatic = 4
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