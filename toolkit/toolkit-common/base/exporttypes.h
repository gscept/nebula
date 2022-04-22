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
    HasSkin = 1 << 0,
    HasMultipleUVs = 1 << 1,
    HasVertexColors = 1 << 2,
    HasBinormals = 1 << 3,
    HasTangents = 1 << 4,
    IsDynamic = 1 << 5
};

enum ExportMode
{
    Static,
    Skeletal,

    NumImportModes
};

enum ExportFlags
{
    None = 0,
    RemoveRedundant = 1 << 0,
    CalcNormals = 1 << 1,
    FlipUVs = 1 << 2,
    ImportColors = 1 << 3,
    ImportSecondaryUVs = 1 << 4,
    CalcBinormalsAndTangents = 1 << 5,
    All = (1 << 6) - 1,

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
