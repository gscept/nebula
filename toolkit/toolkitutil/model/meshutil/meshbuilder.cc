//------------------------------------------------------------------------------
//  meshbuilder.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "meshbuilder.h"

namespace ToolkitUtil
{
using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
MeshBuilder::MeshBuilder()
    : topology(CoreGraphics::PrimitiveTopology::InvalidPrimitiveTopology)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilder::NewMesh(SizeT numVertices, SizeT numTriangles)
{
    // Allocate base attributes (position and UVs)
    this->vertices.Reserve(numVertices);
    this->triangles.Reserve(numTriangles);
}

//------------------------------------------------------------------------------
/**
*/
void 
MeshBuilder::SetComponents(const MeshBuilderVertex::ComponentMask mask)
{
    for (auto& v : this->vertices)
        v.SetComponents(mask);

    this->componentMask = mask;
}

//------------------------------------------------------------------------------
/**
*/
const MeshBuilderVertex::ComponentMask 
MeshBuilder::GetComponents()
{
    return this->componentMask;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilder::SetPrimitiveTopology(const CoreGraphics::PrimitiveTopology::Code& p)
{
    this->topology = p;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveTopology::Code&
MeshBuilder::GetPrimitiveTopology()
{
    return this->topology;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilder::SetPrimitiveGroups(const Util::Array<MeshBuilderGroup>& groups)
{
    this->groups = groups;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<MeshBuilderGroup>&
MeshBuilder::GetPrimitiveGroups()
{
    return this->groups;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilder::ClearPrimitiveGroups()
{
    this->groups.Clear();
}

//------------------------------------------------------------------------------
/**
    This method copies a triangle with its vertices from the source mesh
    to this mesh. Vertices will only be copied if they don't already exist
    in this mesh. To accomplish this, an indexMap array must be provided. The
    array must contain entries of type IndexT and its size must be identical to the
    number of vertices in the source mesh. The array elements must be
    initialized with InvalidIndex!! The copy method will record any copied vertices
    into the index map, so that it can find out at a later iteration if
    the vertex has already been copied. This method makes an extra cleanup
    pass unnecessary, since not redundant vertex data will be generated
    during the copy.
*/
void
MeshBuilder::CopyTriangle(const MeshBuilder& srcMesh, IndexT triIndex, Util::FixedArray<IndexT>& indexMap)
{
    const MeshBuilderTriangle& tri = srcMesh.TriangleAt(triIndex);
    IndexT index[3];
    tri.GetVertexIndices(index[0], index[1], index[2]);

    // add vertices, if they don't exist yet, and update indexMap
    IndexT i;
    for (i = 0; i < 3; i++)
    {
        if (indexMap[index[i]] != InvalidIndex)
        {
            index[i] = indexMap[index[i]];
        }
        else
        {
            this->AddVertex(srcMesh.VertexAt(index[i]));
            IndexT newIndex = this->GetNumVertices() - 1;
            indexMap[index[i]] = newIndex;
            index[i] = newIndex;
        }
    }

    // add triangle and update triangle indices
    this->AddTriangle(tri);
    MeshBuilderTriangle& newTri = this->TriangleAt(this->GetNumTriangles() - 1);
    newTri.SetVertexIndices(index[0], index[1], index[2]);
}

//------------------------------------------------------------------------------
/**
*/
bbox
MeshBuilder::ComputeBoundingBox() const
{
    bbox box;
    box.begin_extend();
    SizeT numVertices = this->GetNumVertices();
    IndexT vertexIndex;
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        const MeshBuilderVertex& vtx = this->VertexAt(vertexIndex);
        box.extend(xyz(vtx.base.position));
    }
    box.end_extend();
    return box;
}

//------------------------------------------------------------------------------
/**
    Sort the triangle array by group id, so that clusters of triangles
    with identical group ids are formed.
*/
void
MeshBuilder::SortTriangles()
{
    this->triangles.Sort();
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilder::Clear()
{
    this->vertices.Clear();
    this->triangles.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilder::Transform(const mat4& m)
{
    for (auto& v : this->vertices)
        v.Transform(m);
}

//------------------------------------------------------------------------------
/**
    qsort() hook for Deflate() method.
*/
MeshBuilder* MeshBuilder::qsortData = 0;
int
__cdecl
MeshBuilder::VertexSorter(const void* elm0, const void* elm1)
{
    MeshBuilder* meshBuilder = qsortData;
    int i0 = *(int*)elm0;
    int i1 = *(int*)elm1;
    const MeshBuilderVertex& v0 = meshBuilder->VertexAt(i0);
    const MeshBuilderVertex& v1 = meshBuilder->VertexAt(i1);
    return v0.Compare(v1);
}

//------------------------------------------------------------------------------
/**
    Cleanup the mesh. This removes redundant vertices and optionally record
    the collapse history into a client-provided collapsMap. The collaps map
    contains at each new vertex index the 'old' vertex indices which have
    been collapsed into the new vertex.
*/
void
MeshBuilder::Deflate(FixedArray<Array<IndexT>>* collapsMap)
{
    // generate a index remapping table and sorted vertex array
    SizeT numVertices = this->GetNumVertices();
    IndexT* indexMap = (IndexT*)Memory::Alloc(Memory::ScratchHeap, numVertices * sizeof(IndexT));
    IndexT* sortMap = (IndexT*)Memory::Alloc(Memory::ScratchHeap, numVertices * sizeof(IndexT));
    IndexT* shiftMap = (IndexT*)Memory::Alloc(Memory::ScratchHeap, numVertices * sizeof(IndexT));
    IndexT i;
    for (i = 0; i < numVertices; i++)
    {
        indexMap[i] = i;
        sortMap[i] = i;
    }

    // generate a sorted index map (sort by X coordinate)
    qsortData = this;
    qsort(sortMap, numVertices, sizeof(IndexT), MeshBuilder::VertexSorter);

    // search sorted array for redundant vertices
    IndexT baseIndex = 0;
    for (baseIndex = 0; baseIndex < (numVertices - 1);)
    {
        IndexT nextIndex = baseIndex + 1;
        while ((nextIndex < numVertices) &&
            (this->vertices[sortMap[baseIndex]] == this->vertices[sortMap[nextIndex]]))
        {
            // mark the vertex as invalid
            this->vertices[sortMap[nextIndex]].SetFlag(MeshBuilderVertex::Redundant);

            // put the new valid index into the index remapping table
            indexMap[sortMap[nextIndex]] = sortMap[baseIndex];
            nextIndex++;
        }
        baseIndex = nextIndex;
    }

    // fill the shiftMap, this contains for each vertex index the number
    // of invalid vertices in front of it
    SizeT numInvalid = 0;
    IndexT vertexIndex;
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        if (this->vertices[vertexIndex].CheckFlag(MeshBuilderVertex::Redundant))
        {
            numInvalid++;
        }
        shiftMap[vertexIndex] = numInvalid;
    }

    // fix the triangle's vertex indices, first, remap the old index to a
    // valid index from the indexMap, then decrement by the shiftMap entry
    // at that index (which contains the number of invalid vertices in front
    // of that index)
    // fix vertex indices in triangles
    SizeT numTriangles = this->triangles.Size();
    IndexT curTriangle;
    for (curTriangle = 0; curTriangle < numTriangles; curTriangle++)
    {
        MeshBuilderTriangle& t = this->triangles[curTriangle];
        for (i = 0; i < 3; i++)
        {
            IndexT newIndex = indexMap[t.vertexIndex[i]];
            t.vertexIndex[i] = newIndex - shiftMap[newIndex];
        }
    }

    // initialize the collapse map so that for each new (collapsed)
    // index it contains a list of old vertex indices which have been
    // collapsed into the new vertex 
    if (collapsMap)
    {
        collapsMap->SetSize(numVertices);
        for (i = 0; i < numVertices; i++)
        {
            IndexT newIndex = indexMap[i];
            IndexT collapsedIndex = newIndex - shiftMap[newIndex];
            (*collapsMap)[collapsedIndex].Append(i);
        }
    }

    // finally, remove the redundant vertices
    numVertices = this->vertices.Size();
    Array<MeshBuilderVertex> newArray;
    newArray.Reserve(numVertices);
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        if (!this->vertices[vertexIndex].CheckFlag(MeshBuilderVertex::Redundant))
        {
            newArray.Append(this->vertices[vertexIndex]);
        }
    }
    this->vertices = newArray;

    // cleanup
    Memory::Free(Memory::ScratchHeap, indexMap);
    Memory::Free(Memory::ScratchHeap, sortMap);
    Memory::Free(Memory::ScratchHeap, shiftMap);
}

//------------------------------------------------------------------------------
/**
    This creates 3 unique vertices for each triangle in the mesh, generating
    redundant vertices. This is the opposite operation to Deflate().
*/
void
MeshBuilder::Inflate()
{
    // for each triangle...
    Array<MeshBuilderVertex> newVertexArray;
    newVertexArray.Reserve(this->GetNumTriangles() * 3);
    SizeT numTriangles = this->GetNumTriangles();
    IndexT triangleIndex;
    for (triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
    {
        // build new vertex array and fix triangle vertex indices
        MeshBuilderTriangle& tri = this->TriangleAt(triangleIndex);
        IndexT i;
        for (i = 0; i < 3; i++)
        {
            MeshBuilderVertex vert = this->VertexAt(tri.GetVertexIndex(i));
            newVertexArray.Append(vert);
            tri.vertexIndex[i] = triangleIndex * 3 + i;
        }
    }

    // replace vertex array
    this->vertices = newVertexArray;
}

//------------------------------------------------------------------------------
/**
    Flip vertical texture coordinates.
*/
void
MeshBuilder::FlipUvs()
{
    SizeT numVertices = this->GetNumVertices();
    IndexT vertexIndex;
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        MeshBuilderVertex& v = this->VertexAt(vertexIndex);
        v.base.uv.y = 1.0f - v.base.uv.y;

        if (AllBits(this->componentMask, MeshBuilderVertex::Components::SecondUv))
            v.attributes.secondUv.uv2.y = 1.0f - v.attributes.secondUv.uv2.y;
    }
}

//-----------------------------------------------------------------------------------------------------------
//  Cleanup the mesh
//  This removes redundant vertices and optionally record the collapse history into a client-provided
//   collapseMap
//  The collaps map contains at each new vertex index the 'old' vertex indices which have been collapsed
//   into the new vertex.
//
//  30-Jan-03 Floh Optimizations

void MeshBuilder::Cleanup(Array<Array<int> >* collapseMap)
{
    int numVertices = this->vertices.Size();

    // generate a index remapping table and sorted vertex array
    int* indexMap = new int[numVertices];
    int* sortMap = new int[numVertices];
    int* shiftMap = new int[numVertices];

    int i;
    for (i = 0; i < numVertices; i++)
    {
        indexMap[i] = i;
        sortMap[i] = i;
    }

    // generate a sorted index map (sort by X coordinate)
    qsortData = this;
    qsort(sortMap, numVertices, sizeof(int), MeshBuilder::VertexSorter);

    // search sorted array for redundant vertices
    int baseIndex = 0;
    for (baseIndex = 0; baseIndex < (numVertices - 1);)
    {
        int nextIndex = baseIndex + 1;
        while (nextIndex < numVertices &&
            this->vertices[sortMap[baseIndex]] == this->vertices[sortMap[nextIndex]])
        {
            // mark the vertex as invalid
            this->vertices[sortMap[nextIndex]].SetFlag(MeshBuilderVertex::Redundant);

            // put the new valid index into the index remapping table
            indexMap[sortMap[nextIndex]] = sortMap[baseIndex];
            nextIndex++;
        }

        baseIndex = nextIndex;
    }

    // fill the shiftMap, this contains for each vertex index the number of invalid vertices in front of it
    int numInvalid = 0;

    int vertexIndex;
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        if (this->vertices[vertexIndex].CheckFlag(MeshBuilderVertex::Redundant))
            numInvalid++;

        shiftMap[vertexIndex] = numInvalid;
    }

    // fix the triangle's vertex indices, first, remap the old index to a valid index from the indexMap,
    //  then decrement by the shiftMap entry at that index, which contains the number of invalid vertices
    //  in front of that index
    //  fix vertex indices in triangles
    int numTriangles = this->triangles.Size();

    int curTriangle;
    for (curTriangle = 0; curTriangle < numTriangles; curTriangle++)
    {
        MeshBuilderTriangle& t = this->triangles[curTriangle];
        for (i = 0; i < 3; i++)
        {
            int newIndex = indexMap[t.vertexIndex[i]];
            t.vertexIndex[i] = newIndex - shiftMap[newIndex];
        }
    }

    // initialize the collaps map so that for each new (collapsed) index it contains a list of old vertex indices
    //  which have been collapsed into the new vertex
    if (collapseMap)
    {
        for (i = 0; i < numVertices; i++)
        {
            int newIndex = indexMap[i];
            int collapsedIndex = newIndex - shiftMap[newIndex];

            (*collapseMap)[collapsedIndex].Append(i);
        }
    }

    // finally, remove the redundant vertices
    numVertices = this->vertices.Size();

    Array<MeshBuilderVertex> newArray(numVertices, numVertices);
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        if (!this->vertices[vertexIndex].CheckFlag(MeshBuilderVertex::Redundant))
            newArray.Append(this->vertices[vertexIndex]);
    }

    this->vertices = newArray;

    // cleanup
    delete[] indexMap;
    delete[] sortMap;
    delete[] shiftMap;
}

//------------------------------------------------------------------------------
/**
    @note   This does not handle primitive groups. You need to manually adjust group ids!
*/
void
MeshBuilder::Merge(MeshBuilder const& sourceMesh)
{
    uint primitiveGroup = 0;

    SizeT vertOffset = this->GetNumVertices();

    SizeT vertCount = sourceMesh.GetNumVertices();
    IndexT vertIndex;
    SizeT triCount = sourceMesh.GetNumTriangles();
    IndexT triIndex;
    SizeT groupCount = sourceMesh.groups.Size();
    IndexT groupIndex;

    for (vertIndex = 0; vertIndex < sourceMesh.GetNumVertices(); vertIndex++)
    {
        this->AddVertex(sourceMesh.VertexAt(vertIndex));
    }

    // then add triangles and update vertex indices
    for (triIndex = 0; triIndex < triCount; triIndex++)
    {
        MeshBuilderTriangle tri = sourceMesh.TriangleAt(triIndex);
        tri.SetVertexIndex(0, vertOffset + tri.GetVertexIndex(0));
        tri.SetVertexIndex(1, vertOffset + tri.GetVertexIndex(1));
        tri.SetVertexIndex(2, vertOffset + tri.GetVertexIndex(2));

        // add triangle to mesh
        this->AddTriangle(tri);
    }

    for (groupIndex = 0; groupIndex < groupCount; groupIndex++)
    {
        this->groups.Append(sourceMesh.groups[groupIndex]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilder::CalculateNormals()
{
    int numTriangles = this->GetNumTriangles();
    for (int triIndex = 0; triIndex < numTriangles; triIndex++)
    {
        MeshBuilderTriangle& tri = this->TriangleAt(triIndex);
        int vertIndex1, vertIndex2, vertIndex3;
        tri.GetVertexIndices(vertIndex1, vertIndex2, vertIndex3);

        MeshBuilderVertex& v1 = this->VertexAt(vertIndex1);
        MeshBuilderVertex& v2 = this->VertexAt(vertIndex2);
        MeshBuilderVertex& v3 = this->VertexAt(vertIndex3);

        v1.SetComponents(this->componentMask);
        v2.SetComponents(this->componentMask);
        v3.SetComponents(this->componentMask);

        vec4 p1 = v1.base.position;
        vec4 p2 = v2.base.position;
        vec4 p3 = v3.base.position;

        vec3 normal1 = xyz(cross3(p2 - p1, p3 - p1));

        if (AllBits(this->componentMask, MeshBuilderVertex::Components::Normals))
        {
            v1.attributes.normal.normal = normalize(v1.attributes.normal.normal + normal1);
            v2.attributes.normal.normal = normalize(v2.attributes.normal.normal + normal1);
            v3.attributes.normal.normal = normalize(v3.attributes.normal.normal + normal1);
        }
        else
        {
            v1.attributes.normal.normal = normal1;
            v2.attributes.normal.normal = normal1;
            v3.attributes.normal.normal = normal1;
        }
    }
    this->componentMask |= MeshBuilderVertex::Components::Normals;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilder::CalculateTangents()
{
    int numVertices = this->GetNumVertices();
    int numTriangles = this->GetNumTriangles();
    vec3* tangents1 = new vec3[numVertices * 2];
    vec3* tangents2 = tangents1 + numVertices;

    // We must have normals to calculate tangents
    memset(tangents1, 0, numVertices * sizeof(vec3) * 2);
    for (int triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
    {
        MeshBuilderTriangle& triangle = this->TriangleAt(triangleIndex);
        int v1Index = triangle.GetVertexIndex(0);
        int v2Index = triangle.GetVertexIndex(1);
        int v3Index = triangle.GetVertexIndex(2);

        MeshBuilderVertex& v1 = this->VertexAt(v1Index);
        MeshBuilderVertex& v2 = this->VertexAt(v2Index);
        MeshBuilderVertex& v3 = this->VertexAt(v3Index);

        v1.SetComponents(this->componentMask);
        v2.SetComponents(this->componentMask);
        v3.SetComponents(this->componentMask);

        // v1 normal
        const vec4& p1 = v1.base.position;
        // v2 normal
        const vec4& p2 = v2.base.position;
        // v3 normal
        const vec4& p3 = v3.base.position;

        // v1 texture coordinate
        const vec2& w1 = v1.base.uv;
        // v2 texture coordinate
        const vec2& w2 = v2.base.uv;
        // v3 texture coordinate
        const vec2& w3 = v3.base.uv;

        float x1 = p2.x - p1.x;
        float x2 = p3.x - p1.x;
        float y1 = p2.y - p1.y;
        float y2 = p3.y - p1.y;
        float z1 = p2.z - p1.z;
        float z2 = p3.z - p1.z;

        float s1 = w2.x - w1.x;
        float s2 = w3.x - w1.x;
        float t1 = w2.y - w1.y;
        float t2 = w3.y - w1.y;

        float rDenom = (s1 * t2 - s2 * t1);
        float r = 1 / rDenom;

        vec3 sdir = vec3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
        vec3 tdir = vec3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

        tangents1[v1Index] += sdir;
        tangents1[v2Index] += sdir;
        tangents1[v3Index] += sdir;

        tangents2[v1Index] += tdir;
        tangents2[v2Index] += tdir;
        tangents2[v3Index] += tdir;
    }

    for (int vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        MeshBuilderVertex& vertex = this->VertexAt(vertexIndex);
        vec3 n = normalize(vertex.attributes.normal.normal);
        vec3 t = tangents1[vertexIndex];
        vec3 b = tangents2[vertexIndex];

        vec3 tangent = normalize(t - n * dot(n, t));
        byte handedNess = (dot(cross(n, t), tangents2[vertexIndex]) < 0.0f ? 1 : -1);
        vertex.attributes.normal.sign = handedNess;
        if (AllBits(this->componentMask, MeshBuilderVertex::Components::Tangents))
            vertex.attributes.normal.tangent += tangent;
        else
            vertex.attributes.normal.tangent = tangent;

        // Normalize tangent
        vertex.attributes.normal.tangent = normalize(vertex.attributes.normal.tangent);
    }

    // finally normalize all of it
    //for (int vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    //{
    //    MeshBuilderVertex& vertex = this->VertexAt(vertexIndex);
    //    vertex.SetComponent(MeshBuilderVertex::NormalB4NIndex, normalize3(vertex.GetComponent(MeshBuilderVertex::NormalB4NIndex)));
    //    vertex.SetComponent(MeshBuilderVertex::TangentB4NIndex, normalize3(vertex.GetComponent(MeshBuilderVertex::TangentB4NIndex)));
    //    vertex.SetComponent(MeshBuilderVertex::BinormalB4NIndex, normalize3(vertex.GetComponent(MeshBuilderVertex::BinormalB4NIndex)));
    //}
    delete[] tangents1;
}

} // namespace ToolkitUtil
