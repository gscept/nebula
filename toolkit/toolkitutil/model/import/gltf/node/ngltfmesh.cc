//------------------------------------------------------------------------------
//  fbxmeshnode.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "ngltfmesh.h"
#include "model/meshutil/meshbuildervertex.h"
#include "ngltfscene.h"
#include "util/bitfield.h"
#include "jobs2/jobs2.h"
#include "meshprimitive.h"

namespace ToolkitUtil
{

using namespace Math;
using namespace Util;
using namespace CoreAnimation;
using namespace ToolkitUtil;

__ImplementClass(ToolkitUtil::NglTFMesh, 'ASMN', ToolkitUtil::NglTFNode);

//------------------------------------------------------------------------------
/**
*/
NglTFMesh::NglTFMesh()
{
    this->meshBuilder = new MeshBuilder();
}

//------------------------------------------------------------------------------
/**
*/
NglTFMesh::~NglTFMesh()
{
    delete this->meshBuilder;
}

//------------------------------------------------------------------------------
/**
*/
void 
NglTFMesh::Setup(Util::Array<MeshBuilder>& meshes
        , const Gltf::Mesh* gltfMesh
        , const Gltf::Document* scene
        , const ToolkitUtil::ExportFlags flags
        , const uint basePrimitive
        , const uint meshIndex
        , SceneNode** nodes
)
{
    if (gltfMesh->primitives.Size() == 0)
        return;

    // Extract and pre-process primitives ------------

    MeshPrimitiveJobContext jobContext;
    jobContext.scene = scene;
    jobContext.mesh = gltfMesh;
    jobContext.flags = flags;
    jobContext.outMeshes = Jobs2::JobAlloc<MeshBuilder*>(gltfMesh->primitives.Size());
    jobContext.outSceneNodes = nodes;
    jobContext.primitives = Jobs2::JobAlloc<const Gltf::Primitive*>(gltfMesh->primitives.Size());
    jobContext.basePrimitive = basePrimitive;
    jobContext.meshIndex = meshIndex;

    meshes.Resize(gltfMesh->primitives.Size());

    SizeT meshIterator = 0;
    for (auto const& primitive : gltfMesh->primitives)
    {
        jobContext.primitives[meshIterator] = &primitive;
        jobContext.outMeshes[meshIterator] = &meshes[meshIterator];

        ++meshIterator;
    }

    Threading::Event event;
    Jobs2::JobDispatch(MeshPrimitiveFunc, gltfMesh->primitives.Size(), 1, jobContext, nullptr, nullptr, &event);

    event.Wait();
}

} // namespace ToolkitUtil