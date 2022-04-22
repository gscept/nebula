//------------------------------------------------------------------------------
//  fbxmeshnode.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "ngltfmesh.h"
#include "meshutil/meshbuildervertex.h"
#include "ngltfscene.h"
#include "util/bitfield.h"
#include "jobs/jobs.h"
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
NglTFMesh::Setup(Gltf::Node const* node, const Ptr<NglTFScene>& scene)
{
    NglTFNode::Setup(node, scene);

    this->meshId = node->mesh;
    n_assert(scene->GetScene()->meshes.Size() > this->meshId);
    this->gltfMesh = &scene->GetScene()->meshes[this->meshId];
    
    this->ExtractMesh();
}

//------------------------------------------------------------------------------
/**
*/
void
NglTFMesh::Discard()
{
    this->meshBuilder->Clear();
}


//------------------------------------------------------------------------------
/**
    Extracts mesh information from Gltf::Mesh
*/
void
NglTFMesh::ExtractMesh()
{
    if (this->gltfMesh->primitives.IsEmpty())
        return;

    Jobs::CreateJobPortInfo portInfo;
    portInfo.name = "meshJobPort"_atm;
    portInfo.numThreads = Math::min(8, this->gltfMesh->primitives.Size());
    portInfo.priority = Threading::Thread::Priority::High; // UINT_MAX
    portInfo.affinity = System::Cpu::Core0 | System::Cpu::Core1 | System::Cpu::Core2 | System::Cpu::Core3 | System::Cpu::Core4 | System::Cpu::Core5 | System::Cpu::Core6 | System::Cpu::Core7;

    auto jobPort = Jobs::CreateJobPort(portInfo);

    Jobs::CreateJobSyncInfo syncInfo;
    syncInfo.callback = nullptr;

    auto jobInternalSync = Jobs::CreateJobSync(syncInfo);
    auto jobHostSync = Jobs::CreateJobSync({ nullptr });

    // Extract and pre-process primitives ------------

    Util::Array<PrimitiveJobInput> pjInputs;
    Util::Array<PrimitiveJobOutput> pjOutputs;

    for (auto const& primitive : this->gltfMesh->primitives)
    {
        PrimitiveJobInput input = {
            this->gltfMesh,
            &primitive,
            // TODO: attributes per primitive group
            this->scene->GetExportFlags(),
            this->meshId
        };

        pjInputs.Append(input);

        PrimitiveJobOutput output;
        output.mesh = n_new(MeshBuilder);
            
        pjOutputs.Append(output);
    }

    Jobs::JobContext primitiveJobContext;
    primitiveJobContext.uniform.data[0] = gltfScene;
    primitiveJobContext.uniform.dataSize[0] = 1;
    primitiveJobContext.uniform.numBuffers = 1;
    primitiveJobContext.uniform.scratchSize = 0;

    primitiveJobContext.input.numBuffers = 1;
    primitiveJobContext.input.data[0] = pjInputs.Begin();
    primitiveJobContext.input.dataSize[0] = sizeof(PrimitiveJobInput) * pjInputs.Size();
    primitiveJobContext.input.sliceSize[0] = sizeof(PrimitiveJobInput);
    
    primitiveJobContext.output.numBuffers = 1;
    primitiveJobContext.output.data[0] = pjOutputs.Begin();
    primitiveJobContext.output.dataSize[0] = sizeof(PrimitiveJobOutput) * pjOutputs.Size();
    primitiveJobContext.output.sliceSize[0] = sizeof(PrimitiveJobOutput);

    Jobs::CreateJobInfo primitiveJob{ &SetupPrimitiveGroupJobFunc };
    
    auto primitiveJobId = Jobs::CreateJob(primitiveJob);

    // run job
    Jobs::JobSchedule(primitiveJobId, jobPort, primitiveJobContext);
    Jobs::JobSyncThreadSignal(jobInternalSync, jobPort);
    Jobs::JobSyncHostWait(jobInternalSync);
    Jobs::DestroyJob(primitiveJobId);
    
    Jobs::DestroyJobPort(jobPort);

    // Merge all primitive groups into one mesh
    this->boundingBox.begin_extend();

    IndexT groupId = 0;
    for (auto& output : pjOutputs)
    {
        IndexT const firstTriangle = this->meshBuilder->GetNumTriangles();
        this->meshBuilder->Merge(*output.mesh);

        SizeT const size = this->meshBuilder->GetNumTriangles();
        for (IndexT triIndex = firstTriangle; triIndex < size; triIndex++)
        {
            this->meshBuilder->TriangleAt(triIndex).SetGroupId(groupId);
        }

        MeshPrimitive groupDesc;
        MeshBuilderGroup group;
        group.SetFirstTriangleIndex(firstTriangle);
        group.SetGroupId(groupId);
        group.SetNumTriangles(size - firstTriangle);
        groupDesc.group = group;
        //output.bbox.pmin *= this->scale;
        //output.bbox.pmax *= this->scale;
        groupDesc.boundingBox = output.bbox;
        //groupDesc.meshFlags = 
        groupDesc.name.Format("%s:%i", output.name.AsCharPtr(), groupId);

        groupDesc.material = output.material->name;
        this->primGroups.Append(groupDesc);

        this->boundingBox.extend(output.bbox);

        groupId++;
    }

    this->boundingBox.end_extend();

    // clean up old output
    for (auto const& output : pjOutputs)
    {
        n_delete(output.mesh);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
NglTFMesh::CalculateNormals()
{
    int numTriangles = this->meshBuilder->GetNumTriangles();
    for (int triIndex = 0; triIndex < numTriangles; triIndex++)
    {
        MeshBuilderTriangle& tri = this->meshBuilder->TriangleAt(triIndex);
        int vertIndex1, vertIndex2, vertIndex3;
        tri.GetVertexIndices(vertIndex1, vertIndex2, vertIndex3);

        MeshBuilderVertex& v1 = this->meshBuilder->VertexAt(vertIndex1);
        MeshBuilderVertex& v2 = this->meshBuilder->VertexAt(vertIndex2);
        MeshBuilderVertex& v3 = this->meshBuilder->VertexAt(vertIndex3);

        vec4 p1 = v1.GetComponent(MeshBuilderVertex::CoordIndex);
        vec4 p2 = v2.GetComponent(MeshBuilderVertex::CoordIndex);
        vec4 p3 = v3.GetComponent(MeshBuilderVertex::CoordIndex);

        vec4 normal1 = cross3(p2 - p1, p3 - p1);

        if (v1.HasComponent(MeshBuilderVertex::NormalB4NBit))
        {
            vec4 normal = v1.GetComponent(MeshBuilderVertex::NormalB4NIndex) + normal1;
            normal = normalize3(normal);
            v1.SetComponent(MeshBuilderVertex::NormalB4NIndex, normal);
        }
        else
        {
            v1.SetComponent(MeshBuilderVertex::NormalB4NIndex, normal1);
        }

        if (v2.HasComponent(MeshBuilderVertex::NormalB4NBit))
        {
            vec4 normal = v1.GetComponent(MeshBuilderVertex::NormalB4NIndex) + normal1;
            normal = normalize3(normal);
            v2.SetComponent(MeshBuilderVertex::NormalB4NIndex, normal);
        }
        else
        {
            v2.SetComponent(MeshBuilderVertex::NormalB4NIndex, normal1);
        }

        if (v3.HasComponent(MeshBuilderVertex::NormalB4NBit))
        {
            vec4 normal = v1.GetComponent(MeshBuilderVertex::NormalB4NIndex) + normal1;
            normal = normalize3(normal);
            v3.SetComponent(MeshBuilderVertex::NormalB4NIndex, normal);
        }
        else
        {
            v3.SetComponent(MeshBuilderVertex::NormalB4NIndex, normal1);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
NglTFMesh::CalculateTangentsAndBinormals()
{
    int numVertices = meshBuilder->GetNumVertices();
    int numTriangles = meshBuilder->GetNumTriangles();
    vec4* tangents1 = new vec4[numVertices * 2];
    vec4* tangents2 = tangents1 + numVertices;

    memset(tangents1, 0, numVertices * sizeof(vec4) * 2);
    for (int triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
    {
        MeshBuilderTriangle& triangle = meshBuilder->TriangleAt(triangleIndex);
        int v1Index = triangle.GetVertexIndex(0);
        int v2Index = triangle.GetVertexIndex(1);
        int v3Index = triangle.GetVertexIndex(2);

        MeshBuilderVertex& vertex1 = meshBuilder->VertexAt(v1Index);
        MeshBuilderVertex& vertex2 = meshBuilder->VertexAt(v2Index);
        MeshBuilderVertex& vertex3 = meshBuilder->VertexAt(v3Index);

        // v1 normal
        const vec4& v1 = vertex1.GetComponent(MeshBuilderVertex::CoordIndex);
        // v2 normal
        const vec4& v2 = vertex2.GetComponent(MeshBuilderVertex::CoordIndex);
        // v3 normal
        const vec4& v3 = vertex3.GetComponent(MeshBuilderVertex::CoordIndex);

        // v1 texture coordinate
        const vec4& w1 = vertex1.GetComponent(MeshBuilderVertex::Uv0Index);
        // v2 texture coordinate
        const vec4& w2 = vertex2.GetComponent(MeshBuilderVertex::Uv0Index);
        // v3 texture coordinate
        const vec4& w3 = vertex3.GetComponent(MeshBuilderVertex::Uv0Index);

        float x1 = v2.x - v1.x;
        float x2 = v3.x - v1.x;
        float y1 = v2.y - v1.y;
        float y2 = v3.y - v1.y;
        float z1 = v2.z - v1.z;
        float z2 = v3.z - v1.z;

        float s1 = w2.x - w1.x;
        float s2 = w3.x - w1.x;
        float t1 = w2.y - w1.y;
        float t2 = w3.y - w1.y;

        float rDenom = (s1 * t2 - s2 * t1);
        float r = 1 / rDenom;

        vec4 sdir = vec4((t2 * x1 - t1 * x2) * r, (t2*y1 - t1 * y2) * r, (t2*z1 - t1 * z2) * r, 0.0f);
        vec4 tdir = vec4((s1 * x2 - s2 * x1) * r, (s1*y2 - s2 * y1) * r, (s1*z2 - s2 * z1) * r, 0.0f);

        tangents1[v1Index] += sdir;
        tangents1[v2Index] += sdir;
        tangents1[v3Index] += sdir;

        tangents2[v1Index] += tdir;
        tangents2[v2Index] += tdir;
        tangents2[v3Index] += tdir;
    }

    for (int vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        MeshBuilderVertex& vertex = meshBuilder->VertexAt(vertexIndex);
        vec4 n = normalize3(vertex.GetComponent(MeshBuilderVertex::NormalB4NIndex));
        vec4 t = tangents1[vertexIndex];
        vec4 b = tangents2[vertexIndex];

        vec4 tangent = normalize3(t - n * dot3(n, t));
        float handedNess = (dot3(cross3(n, t), tangents2[vertexIndex]) < 0.0f ? 1.0f : -1.0f);
        vec4 bitangent = normalize3(cross3(n, tangent) * handedNess);

        if (vertex.HasComponent(MeshBuilderVertex::TangentB4NBit))
        {
            vec4 oldTangent = vertex.GetComponent(MeshBuilderVertex::TangentB4NIndex);
            vec4 newTangent = oldTangent + tangent;
            vertex.SetComponent(MeshBuilderVertex::TangentB4NIndex, newTangent);
        }
        else
        {
            vertex.SetComponent(MeshBuilderVertex::TangentB4NIndex, tangent);
        }

        if (vertex.HasComponent(MeshBuilderVertex::BinormalB4NBit))
        {
            vec4 oldBinormal = vertex.GetComponent(MeshBuilderVertex::BinormalB4NIndex);
            vec4 newBinormal = oldBinormal + bitangent;
            vertex.SetComponent(MeshBuilderVertex::BinormalB4NIndex, newBinormal);
        }
        else
        {
            vertex.SetComponent(MeshBuilderVertex::BinormalB4NIndex, bitangent);
        }
    }

    // finally normalize all of it!
    for (int vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        MeshBuilderVertex& vertex = meshBuilder->VertexAt(vertexIndex);
        vertex.SetComponent(MeshBuilderVertex::NormalB4NIndex, normalize3(vertex.GetComponent(MeshBuilderVertex::NormalB4NIndex)));
        vertex.SetComponent(MeshBuilderVertex::TangentB4NIndex, normalize3(vertex.GetComponent(MeshBuilderVertex::TangentB4NIndex)));
        vertex.SetComponent(MeshBuilderVertex::BinormalB4NIndex, normalize3(vertex.GetComponent(MeshBuilderVertex::BinormalB4NIndex)));
    }

    delete[] tangents1;
}

} // namespace ToolkitUtil