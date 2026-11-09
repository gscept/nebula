//------------------------------------------------------------------------------
//  @file cooking.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "physics/cooking.h"
#include "physics/physxstate.h"
#include "math/scalar.h"

using namespace physx;

namespace Physics
{

//------------------------------------------------------------------------------
/**
*/
PxGeometryHolder
CookMesh(const MeshGeometryInput& input)
{
    n_assert(input.vertexCount > 0);
    physx::PxGeometryHolder holder;

    switch (input.topology)
    {
        // FIXME, cooking doesnt seem to like our meshes,
        // always create convex hull, even if already convex
        case MeshTopology_Convex:
        case MeshTopology_ConvexHull:
        {
            PxConvexMeshDesc convexDesc;
            convexDesc.points.count = (PxU32)input.vertexCount;
            convexDesc.points.stride = (PxU32)input.positionStride;
            convexDesc.points.data = input.positions.ConstBegin();
            convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

            PxTolerancesScale scale;
            PxCookingParams params(scale);
            PxDefaultMemoryOutputStream buf;
            PxConvexMeshCookingResult::Enum result;
            PxCookConvexMesh(params, convexDesc, buf, &result);

            PxDefaultMemoryInputData data(buf.getData(), buf.getSize());
            holder = PxConvexMeshGeometry(state.physics->createConvexMesh(data));
        }
        break;
        case MeshTopology_Triangles:
        {
            n_assert(input.indexCount % 3 == 0);

            PxTolerancesScale scale;
            PxCookingParams params(scale);
            // disable mesh cleaning - perform mesh validation on development configurations
            //params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
            // disable edge precompute, edges are set for each triangle, slows contact generation
            params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;

            PxTriangleMeshDesc meshDesc;
            meshDesc.points.count = (PxU32)input.vertexCount;
            meshDesc.points.stride = (PxU32)input.positionStride;
            meshDesc.points.data = input.positions.ConstBegin();

            meshDesc.triangles.count = (PxU32)(input.indexCount / 3);
            if (input.indexType == Physics::IndexType::Index16)
            {
                meshDesc.flags |= PxMeshFlag::e16_BIT_INDICES;
            }
            meshDesc.triangles.stride = (PxU32)(Physics::IndexSize(input.indexType) * 3);
            meshDesc.triangles.data = input.indices.ConstBegin();

#if NEBULA_DEBUG
            PxValidateTriangleMesh(params, meshDesc);
#endif
            holder = PxTriangleMeshGeometry(PxCreateTriangleMesh(params, meshDesc, state.physics->getPhysicsInsertionCallback()));
        }
        break;
        default:
            n_error("CookMesh: unsupported mesh topology %d", input.topology);
    }

    return holder;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<CookedConvex>
CookApproxSkin(const MeshGeometryInput& input)
{
    Util::Array<CookedConvex> cooked;

    n_assert(input.vertexCount > 0);
    n_assert(input.weights.Size() == input.vertexCount * 4);
    n_assert(input.bones.Size() == input.vertexCount);

    Util::Array<physx::PxVec3> targetVertexBuffer(input.vertexCount, 10);

    // find the highest bone the skin data references
    uint maxBone = 0;
    for (SizeT i = 0; i < input.vertexCount; i++)
    {
        if (input.weights[i * 4] > 0.0f)
        {
            maxBone = Math::max(input.bones[i], maxBone);
        }
    }

    // this is super inefficient, but also pointless, so who cares :D
    for (uint currBone = 0; currBone <= maxBone; currBone++)
    {
        targetVertexBuffer.Reset();
        for (SizeT i = 0; i < input.vertexCount; i++)
        {
            if (input.weights[i * 4] > 0.0f && input.bones[i] == currBone)
            {
                targetVertexBuffer.Append({ input.positions[i * 3], input.positions[i * 3 + 1], input.positions[i * 3 + 2] });
            }
        }
        if (targetVertexBuffer.Size() > 0)
        {
            PxConvexMeshDesc convexDesc;
            convexDesc.points.count = (PxU32)targetVertexBuffer.Size();
            convexDesc.points.stride = (PxU32)sizeof(PxVec3);
            convexDesc.points.data = &targetVertexBuffer[0];
            convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

            PxDefaultMemoryOutputStream buf;
            PxConvexMeshCookingResult::Enum result;
            PxTolerancesScale scale;
            PxCookingParams params(scale);
            PxCookConvexMesh(params, convexDesc, buf, &result);
            if (result == PxConvexMeshCookingResult::eSUCCESS)
            {
                CookedConvex convex;
                PxDefaultMemoryInputData data(buf.getData(), buf.getSize());
                convex.geometry = PxConvexMeshGeometry(state.physics->createConvexMesh(data));
                convex.bone = (IndexT)currBone;
                cooked.Append(convex);
            }
            else
            {
                n_printf("CookApproxSkin: convex mesh failed: %d\n", result);
            }
        }
    }

    return cooked;
}

//------------------------------------------------------------------------------
/**
*/
PxGeometryHolder
CookHeightField(const HeightFieldInput& input, float heightRange, float targetSizeX, float targetSizeZ)
{
    n_assert(input.width > 0 && input.height > 0);
    n_assert(input.samples.Size() == input.width * input.height);

    Util::Array<physx::PxHeightFieldSample> heightSamples(input.width * input.height, 10);
    heightSamples.Resize(input.width * input.height);
    for (SizeT i = 0; i < input.samples.Size(); i++)
    {
        heightSamples[i].height = (PxI16)input.samples[i];
        heightSamples[i].materialIndex0 = PxBitAndByte(0);
        heightSamples[i].materialIndex1 = PxBitAndByte(0);
    }

    PxHeightFieldDesc hfDesc;
    hfDesc.format = PxHeightFieldFormat::eS16_TM;
    hfDesc.nbColumns = (PxU32)input.width;
    hfDesc.nbRows = (PxU32)input.height;
    hfDesc.samples.data = heightSamples.ConstBegin();
    hfDesc.samples.stride = (PxU32)sizeof(PxHeightFieldSample);

    PxHeightField* physxHeightField = PxCreateHeightField(hfDesc);

    float sh = heightRange / 65536.0f;
    float sx = targetSizeX / (float)input.width;
    float sz = targetSizeZ / (float)input.height;

    return PxHeightFieldGeometry(physxHeightField, PxMeshGeometryFlags(), sh, sx, sz);
}

} // namespace Physics
//------------------------------------------------------------------------------
