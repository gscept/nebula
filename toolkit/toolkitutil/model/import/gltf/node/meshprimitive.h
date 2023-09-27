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
#include "model/import/gltf/gltfdata.h"
#include "model/import/base/scenenode.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{

struct MeshPrimitiveJobContext
{
    const Gltf::Document* scene;
    const Gltf::Mesh* mesh;

    const Gltf::Primitive** primitives;
    ExportFlags flags;

    uint meshIndex;
    MeshBuilder** outMeshes;
    SceneNode** outSceneNodes;
};

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
void MeshPrimitiveFunc(SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx);

} // namespace ToolkitUtil
//------------------------------------------------------------------------------