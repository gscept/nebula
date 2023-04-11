#pragma once
//------------------------------------------------------------------------------
//  exporttypes.h
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

namespace ToolkitUtil
{

enum MeshFlags
{
    NoMeshFlags = 0,
    HasNormals = 0x1,
    HasSkin = 0x2,
    HasMultipleUVs = 0x4,
    HasVertexColors = 0x8,
    HasTangents = 0x10
};

enum ExportFlags
{
    None = 0,
    RemoveRedundant = 1 << 0,
    CalcNormals = 1 << 1,
    FlipUVs = 1 << 2,
    ImportColors = 1 << 3,
    ImportSecondaryUVs = 1 << 4,
    CalcTangents = 1 << 5,
    CalcRigidSkin = 1 << 6,
    All = (1 << 7) - 1,

    NumMeshFlags
};

enum PhysicsExportMode
{
    UsePhysics = 0,
    UseBoundingBox = 1,
    UseGraphicsMesh = 2,
    UseBoundingSphere = 3,
    UseBoundingCapsule = 4
};

} // namespace ToolkitUtil
