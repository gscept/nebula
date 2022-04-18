#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::MeshPrimitive

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "math/bbox.h"
#include "meshutil/meshbuilder.h"
#include "toolkit-common/base/exporttypes.h"
#include "meshutil/meshbuildergroup.h"
#include "jobs/jobs.h"
#include "gltf/gltfdata.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{

struct MeshPrimitive
{
    ToolkitUtil::MeshFlags meshFlags;
    MeshBuilderGroup group;
    Math::bbox boundingBox;
    Util::String name;
    Util::String material;
};

struct PrimitiveJobInput
{
    Gltf::Mesh const* mesh;
    Gltf::Primitive const* primitive;
    ExportFlags exportFlags;
    int32_t meshId;
};

struct PrimitiveJobOutput
{
    Util::String name;
    Math::bbox bbox;
    MeshBuilder* mesh;
    Gltf::Material const* material;
};

void SetupPrimitiveGroupJobFunc(Jobs::JobFuncContext const& context);

} // namespace ToolkitUtil
//------------------------------------------------------------------------------