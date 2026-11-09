//------------------------------------------------------------------------------
//  @file cookingtest.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "testbase/testcase.h"
#include "cookingtest.h"

#include "physics/geometry.h"
#include "physics/cooking.h"
#include "physics/physxstate.h"

#include "PxPhysicsAPI.h"

using namespace physx;

namespace Test
{
__ImplementClass(Test::CookingTest, 'CPTC', Test::TestCase);

//------------------------------------------------------------------------------
/**
    Append a vertex to a geometry description, the way a format reader would
*/
static void
AppendVertex(Physics::MeshGeometryInput& input, float x, float y, float z)
{
    input.positions.Append(x);
    input.positions.Append(y);
    input.positions.Append(z);
    input.vertexCount++;
}

//------------------------------------------------------------------------------
/**
*/
static void
AppendIndex(Physics::MeshGeometryInput& input, uint index)
{
    SizeT indexSize = Physics::IndexSize(input.indexType);
    for (SizeT i = 0; i < indexSize; i++)
    {
        input.indices.Append((ubyte)((index >> (i * 8)) & 0xFF));
    }
    input.indexCount++;
}

//------------------------------------------------------------------------------
/**
*/
static bool
NearlyEqual(float a, float b)
{
    float diff = a - b;
    if (diff < 0.0f) { diff = -diff; }
    return diff < 0.0001f;
}

//------------------------------------------------------------------------------
/**
*/
static Physics::MeshGeometryInput
BuildTetrahedron(Physics::IndexType indexType)
{
    Physics::MeshGeometryInput input;
    input.indexType = indexType;
    input.topology = Physics::MeshTopology_Triangles;

    AppendVertex(input, 0.0f, 0.0f, 0.0f);
    AppendVertex(input, 1.0f, 0.0f, 0.0f);
    AppendVertex(input, 0.0f, 1.0f, 0.0f);
    AppendVertex(input, 0.0f, 0.0f, 1.0f);

    AppendIndex(input, 0); AppendIndex(input, 1); AppendIndex(input, 2);
    AppendIndex(input, 1); AppendIndex(input, 3); AppendIndex(input, 2);
    AppendIndex(input, 1); AppendIndex(input, 0); AppendIndex(input, 3);
    AppendIndex(input, 2); AppendIndex(input, 3); AppendIndex(input, 0);

    return input;
}

//------------------------------------------------------------------------------
/**
    Run the cooking layer on hand built geometry
*/
void
CookingTest::Run()
{
    Physics::state.Setup();

    // the index sizes must stay meaningful for the raw index bytes
    VERIFY(Physics::IndexSize(Physics::IndexType::Index16) == sizeof(ushort));
    VERIFY(Physics::IndexSize(Physics::IndexType::Index32) == sizeof(uint));
    VERIFY(Physics::IndexSize(Physics::IndexType::InvalidIndexType) == 0);

    // hand built geometry loader
    Physics::GeometryLoader loader;
    loader.LoadMesh = [](Util::StringAtom resource, IndexT primGroup, Physics::MeshGeometryInput& out) -> bool
    {
        if (resource != Util::StringAtom("test:tetrahedron"))
        {
            return false;
        }
        out = BuildTetrahedron(Physics::IndexType::Index16);
        return true;
    };
    loader.LoadHeightField = [](Util::StringAtom resource, Physics::HeightFieldInput& out) -> bool
    {
        if (resource != Util::StringAtom("test:heightfield"))
        {
            return false;
        }
        out.width = 2;
        out.height = 2;
        out.samples.Append(0);
        out.samples.Append(8192);
        out.samples.Append(16384);
        out.samples.Append(24576);
        return true;
    };

    Physics::SetGeometryLoader(loader);
    VERIFY(Physics::GetGeometryLoader().LoadMesh != nullptr);
    VERIFY(Physics::GetGeometryLoader().LoadHeightField != nullptr);

    // the delegate fills the neutral description
    Physics::MeshGeometryInput mesh;
    VERIFY(Physics::GetGeometryLoader().LoadMesh("test:tetrahedron", 0, mesh));
    VERIFY(mesh.vertexCount == 4);
    VERIFY(mesh.indexCount == 12);
    VERIFY(Physics::GetGeometryLoader().LoadMesh("test:unknown", 0, mesh) == false);

    // cook the geometry as a triangle mesh
    physx::PxGeometryHolder triangles = Physics::CookMesh(mesh);
    VERIFY(triangles.getType() == PxGeometryType::eTRIANGLEMESH);
    PxTriangleMesh* cookedTriangles = triangles.triangleMesh().triangleMesh;
    VERIFY(cookedTriangles != nullptr);
    if (cookedTriangles != nullptr)
    {
        VERIFY(cookedTriangles->getNbVertices() == 4);
        VERIFY(cookedTriangles->getNbTriangles() == 4);
    }

    // the same geometry with 32 bit indices
    Physics::MeshGeometryInput wideMesh = BuildTetrahedron(Physics::IndexType::Index32);
    VERIFY(wideMesh.indexCount == 12);
    VERIFY(wideMesh.indices.Size() == 48);
    physx::PxGeometryHolder wideTriangles = Physics::CookMesh(wideMesh);
    VERIFY(wideTriangles.getType() == PxGeometryType::eTRIANGLEMESH);
    PxTriangleMesh* cookedWideTriangles = wideTriangles.triangleMesh().triangleMesh;
    VERIFY(cookedWideTriangles != nullptr);
    if (cookedWideTriangles != nullptr)
    {
        VERIFY(cookedWideTriangles->getNbTriangles() == 4);
    }

    // cube points cooked as a convex hull
    Physics::MeshGeometryInput cube;
    cube.indexType = Physics::IndexType::Index16;
    cube.topology = Physics::MeshTopology_Convex;
    AppendVertex(cube, 0.0f, 0.0f, 0.0f);
    AppendVertex(cube, 1.0f, 0.0f, 0.0f);
    AppendVertex(cube, 1.0f, 1.0f, 0.0f);
    AppendVertex(cube, 0.0f, 1.0f, 0.0f);
    AppendVertex(cube, 0.0f, 0.0f, 1.0f);
    AppendVertex(cube, 1.0f, 0.0f, 1.0f);
    AppendVertex(cube, 1.0f, 1.0f, 1.0f);
    AppendVertex(cube, 0.0f, 1.0f, 1.0f);

    physx::PxGeometryHolder convex = Physics::CookMesh(cube);
    VERIFY(convex.getType() == PxGeometryType::eCONVEXMESH);
    PxConvexMesh* cookedConvex = convex.convexMesh().convexMesh;
    VERIFY(cookedConvex != nullptr);
    if (cookedConvex != nullptr)
    {
        VERIFY(cookedConvex->getNbPolygons() == 6);
    }

    // one convex hull per bone. Bone 0 has the first four points and bone 1 the last four.
    Physics::MeshGeometryInput skinned;
    skinned.indexType = Physics::IndexType::Index16;
    skinned.topology = Physics::MeshTopology_ApproxSkin;
    for (IndexT bone = 0; bone < 2; bone++)
    {
        float offset = (float)bone * 2.0f;
        AppendVertex(skinned, offset + 0.0f, 0.0f, 0.0f);
        AppendVertex(skinned, offset + 1.0f, 0.0f, 0.0f);
        AppendVertex(skinned, offset + 0.0f, 1.0f, 0.0f);
        AppendVertex(skinned, offset + 0.0f, 0.0f, 1.0f);

        for (SizeT v = 0; v < 4; v++)
        {
            skinned.weights.Append(1.0f);
            skinned.weights.Append(0.0f);
            skinned.weights.Append(0.0f);
            skinned.weights.Append(0.0f);
            skinned.bones.Append((uint)bone);
        }
    }

    Util::Array<Physics::CookedConvex> hulls = Physics::CookApproxSkin(skinned);
    VERIFY(hulls.Size() == 2);
    VERIFY(hulls.Size() > 0 && hulls[0].bone == 0 && hulls[0].geometry.convexMesh().convexMesh != nullptr);
    VERIFY(hulls.Size() > 1 && hulls[1].bone == 1 && hulls[1].geometry.convexMesh().convexMesh != nullptr);

    // height field through the delegate
    Physics::HeightFieldInput field;
    VERIFY(Physics::GetGeometryLoader().LoadHeightField("test:heightfield", field));
    VERIFY(field.width == 2 && field.height == 2);

    physx::PxGeometryHolder heightField = Physics::CookHeightField(field, 65536.0f, 10.0f, 20.0f);
    VERIFY(heightField.getType() == PxGeometryType::eHEIGHTFIELD);
    const PxHeightFieldGeometry& hfGeom = heightField.heightField();
    VERIFY(hfGeom.heightField != nullptr);
    if (hfGeom.heightField != nullptr)
    {
        VERIFY(hfGeom.heightField->getNbColumns() == 2);
        VERIFY(hfGeom.heightField->getNbRows() == 2);
        VERIFY(hfGeom.heightField->getSample(0, 0).height == 0);
        VERIFY(hfGeom.heightField->getSample(0, 1).height == 8192);
        VERIFY(hfGeom.heightField->getSample(1, 0).height == 16384);
        VERIFY(hfGeom.heightField->getSample(1, 1).height == 24576);

        // the scales are derived from the height range and the world extent
        VERIFY(NearlyEqual(hfGeom.heightScale, 1.0f));
        VERIFY(NearlyEqual(hfGeom.rowScale, 5.0f));
        VERIFY(NearlyEqual(hfGeom.columnScale, 10.0f));
    }

    // a project that registers no loader at all can not build file based colliders
    Physics::SetGeometryLoader(Physics::GeometryLoader());
    VERIFY(Physics::GetGeometryLoader().LoadMesh == nullptr);
    VERIFY(Physics::GetGeometryLoader().LoadHeightField == nullptr);

    Physics::state.Shutdown();
}

} // namespace Test
//------------------------------------------------------------------------------
