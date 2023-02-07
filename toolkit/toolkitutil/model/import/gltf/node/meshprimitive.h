#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::MeshPrimitive

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "math/bbox.h"
#include "model/meshutil/meshbuilder.h"
#include "toolkit-common/base/exporttypes.h"
#include "model/meshutil/meshbuildergroup.h"
#include "jobs/jobs.h"
#include "gltf/gltfdata.h"
#include "model/import/base/scenenode.h"

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
};

struct PrimitiveJobOutput
{
    Util::String name;
    MeshBuilder* mesh;
    Gltf::Material const* material;
};

void SetupPrimitiveGroupJobFunc(Jobs::JobFuncContext const& context);

} // namespace ToolkitUtil
//------------------------------------------------------------------------------