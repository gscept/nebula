//------------------------------------------------------------------------------
//  meshbuilder.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "toolkitutil/meshutil/meshbuilder.h"

namespace ToolkitUtil
{
using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
MeshBuilder::MeshBuilder() :
    topology(CoreGraphics::PrimitiveTopology::TriangleList)
{
    this->Reserve(10000, 10000);
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilder::Reserve(SizeT numVertices, SizeT numTriangles)
{
    this->vertexArray.Reserve(numVertices);
    this->triangleArray.Reserve(numTriangles);
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
SizeT
MeshBuilder::CountGroupTriangles(IndexT groupId, IndexT startTriangleIndex) const
{
    IndexT triIndex = startTriangleIndex;
    IndexT maxTriIndex = this->triangleArray.Size();
    IndexT numTris = 0;
    for (; triIndex < maxTriIndex; triIndex++)
    {
        if (this->triangleArray[triIndex].GetGroupId() == groupId)
        {
            numTris++;
        }
    }
    return numTris;
}

//------------------------------------------------------------------------------
/**
    Finds the vertex range for the given group id. Returns false if
    there are no triangles in the mesh builder of the given group id.
*/
bool
MeshBuilder::FindGroupVertexRange(IndexT groupId, IndexT& outMinVertexIndex, IndexT& outMaxVertexIndex) const
{
    outMinVertexIndex = this->GetNumVertices();
    outMaxVertexIndex = 0;
    SizeT numCheckedTris = 0;
    IndexT triIndex;
    SizeT numTriangles = this->GetNumTriangles();
    for (triIndex = 0; triIndex < numTriangles; triIndex++)
    {
        const MeshBuilderTriangle& tri = this->TriangleAt(triIndex);
        if (tri.GetGroupId() == groupId)
        {
            numCheckedTris++;
            IndexT vertexIndex[3];
            tri.GetVertexIndices(vertexIndex[0], vertexIndex[1], vertexIndex[2]);
            IndexT i;
            for (i = 0; i < 3; i++)
            {
                if (vertexIndex[i] < outMinVertexIndex) outMinVertexIndex = vertexIndex[i];
                if (vertexIndex[i] > outMaxVertexIndex) outMaxVertexIndex = vertexIndex[i];
            }
        }
    }
    if (0 == numCheckedTris)
    {
        outMinVertexIndex = 0;
        outMaxVertexIndex = 0;
        return false;
    }
    return true;
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
MeshBuilder::ComputeGroupBoundingBox(IndexT groupId) const
{
    bbox box;
    box.begin_extend();
    SizeT numTriangles = this->GetNumTriangles();
    IndexT triangleIndex;
    for (triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
    {
        const MeshBuilderTriangle& triangle = this->TriangleAt(triangleIndex);
        if (triangle.GetGroupId() == groupId)
        {
            IndexT index[3];
            triangle.GetVertexIndices(index[0], index[1], index[2]);
            IndexT i;
            for (i = 0; i < 3; i++)
            {
                box.extend(this->VertexAt(index[i]).GetComponent(MeshBuilderVertex::CoordIndex).vec);
            }
        }
    }
    box.end_extend();
    return box;
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
        box.extend(this->VertexAt(vertexIndex).GetComponent(MeshBuilderVertex::CoordIndex).vec);
    }
    box.end_extend();
    return box;
}

//------------------------------------------------------------------------------
/**
    Build a group map. The triangle array must have been sorted by group id
    for this method. For each distinctive group id, a map entry will be
    created which contains the group id, the first triangle and
    the number of triangles in the group.
*/
void
MeshBuilder::BuildGroupMap(Array<MeshBuilderGroup>& outGroupMap)
{
    IndexT triIndex = 0;
    SizeT numTriangles = this->GetNumTriangles();
    MeshBuilderGroup newGroup;
    while (triIndex < numTriangles)
    {
        const MeshBuilderTriangle& tri = this->TriangleAt(triIndex);
        IndexT groupId = tri.GetGroupId();
        SizeT numTrisInGroup = this->CountGroupTriangles(groupId, triIndex);
        n_assert(numTrisInGroup > 0);
        newGroup.SetGroupId(groupId);
        newGroup.SetFirstTriangleIndex(triIndex);
        newGroup.SetNumTriangles(numTrisInGroup);
        outGroupMap.Append(newGroup);
        triIndex += numTrisInGroup;
    }
}

//------------------------------------------------------------------------------
/**
    Sort the triangle array by group id, so that clusters of triangles
    with identical group ids are formed.
*/
void
MeshBuilder::SortTriangles()
{
    this->triangleArray.Sort();
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilder::Clear()
{
    this->vertexArray.Clear();
    this->triangleArray.Clear();
}

//------------------------------------------------------------------------------
/**
    This method will erase or create empty vertex component arrays.
    New vertex component arrays will always be set to zeros.
    The method can be used to make sure that a mesh file has the same
    vertex size as expected by a vertex shader program.
*/
void
MeshBuilder::ForceVertexComponents(MeshBuilderVertex::ComponentMask wantedMask)
{
    SizeT numVertices = this->GetNumVertices();
    IndexT vertexIndex;
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        MeshBuilderVertex& vertex = this->VertexAt(vertexIndex);
        MeshBuilderVertex::ComponentMask hasMask = vertex.GetComponentMask();
        if (wantedMask != hasMask)
        {
            MeshBuilderVertex::ComponentMask delMask = 0;
            MeshBuilderVertex::ComponentMask initMask = 0;
            IndexT compIndex;
            for (compIndex = 0; compIndex < MeshBuilderVertex::NumComponents; compIndex++)
            {
                MeshBuilderVertex::ComponentMask curMask = (1 << compIndex);
                if ((hasMask & curMask) && (!(wantedMask & curMask)))
                {
                    // add component to delete mask
                    delMask |= curMask;
                }
                else if ((!(hasMask & curMask)) && (wantedMask & curMask))
                {
                    // add component to init mask
                    initMask |= curMask;
                }
            }
            if (0 != delMask)
            {
                vertex.DeleteComponents(delMask);
            }
            if (0 != initMask)
            {
                vertex.InitComponents(initMask);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Or's the components of all vertices, and forces the whole
    vertex pool to that mask. This ensures that all vertices
    in the mesh builder have the same format.
*/
void
MeshBuilder::ExtendVertexComponents()
{
    // get or'ed mask of all vertex components
    MeshBuilderVertex::ComponentMask mask = 0;
    SizeT numVertices = this->GetNumVertices();
    IndexT vertexIndex;
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        const MeshBuilderVertex& v = this->VertexAt(vertexIndex);
        mask |= v.GetComponentMask();
    }

    // extend all vertices to the or'ed vertex component mask
    this->ForceVertexComponents(mask);
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilder::Transform(const mat4& m)
{
    IndexT i;
    SizeT num = this->GetNumVertices();
    for (i = 0; i < num; i++)
    {
        this->vertexArray[i].Transform(m);
    }
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
    // first make sure all vertices have the same vertex components
    this->ExtendVertexComponents();

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
            (this->vertexArray[sortMap[baseIndex]] == this->vertexArray[sortMap[nextIndex]]))
        {
            // mark the vertex as invalid
            this->vertexArray[sortMap[nextIndex]].SetFlag(MeshBuilderVertex::Redundant);

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
        if (this->vertexArray[vertexIndex].CheckFlag(MeshBuilderVertex::Redundant))
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
    SizeT numTriangles = this->triangleArray.Size();
    IndexT curTriangle;
    for (curTriangle = 0; curTriangle < numTriangles; curTriangle++)
    {
        MeshBuilderTriangle& t = this->triangleArray[curTriangle];
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
    numVertices = this->vertexArray.Size();
    Array<MeshBuilderVertex> newArray;
    newArray.Reserve(numVertices);
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        if (!this->vertexArray[vertexIndex].CheckFlag(MeshBuilderVertex::Redundant))
        {
            newArray.Append(vertexArray[vertexIndex]);
        }
    }
    this->vertexArray = newArray;

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
    this->vertexArray = newVertexArray;
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
        IndexT uvLayer;
        for (uvLayer = 0; uvLayer < 4; uvLayer++)
        {
            IndexT compIndex = MeshBuilderVertex::Uv0Index + uvLayer;
            if (v.HasComponents(1 << compIndex))
            {
                vec4 uv = v.GetComponent(MeshBuilderVertex::ComponentIndex(compIndex));
                uv.y = 1.0f - uv.y;
                v.SetComponent(MeshBuilderVertex::ComponentIndex(compIndex), uv);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Checks the UVs of a triangle whether they are within the provided
    range. If not, all triangle UVs will be moved into the valid range.
    This method expects the mesh to be inflated. If the uv range within
    the triangle exceeds the valid range, an error will be returned.
*/
bool
MeshBuilder::MoveTriangleUvsIntoRange(IndexT triIndex, float minUv, float maxUv)
{
    bool success = true;
    int minQuadrant = int(minUv + 1.5f);
    int maxQuadrant = int(maxUv - 0.5f);

    // for each uv set...
    IndexT uvLayerIndex;
    for (uvLayerIndex = 0; uvLayerIndex < 4; uvLayerIndex++)
    {
        MeshBuilderVertex::ComponentIndex uvCompIndex = MeshBuilderVertex::ComponentIndex(MeshBuilderVertex::Uv0Index + uvLayerIndex);

        // for each uv...
        int maxQuadrantX = -1024;
        int minQuadrantX = 1024;
        int maxQuadrantY = -1024;
        int minQuadrantY = 1024;
        IndexT vIndices[3];
        this->TriangleAt(triIndex).GetVertexIndices(vIndices[0], vIndices[1], vIndices[2]);
        IndexT i;
        for (i = 0; i < 3; i++)
        {
            const MeshBuilderVertex& vertex = this->VertexAt(vIndices[i]);
            const vec4& uv = vertex.GetComponent(uvCompIndex);
            int xQuadrant = int(uv.x);
            int yQuadrant = int(uv.y);
            maxQuadrantX = Math::max(xQuadrant, maxQuadrantX);
            minQuadrantX = Math::min(xQuadrant, minQuadrantX);
            maxQuadrantY = Math::max(yQuadrant, maxQuadrantY);
            minQuadrantY = Math::min(yQuadrant, minQuadrantY);
        }
        if (((maxQuadrantX - minQuadrantX) > (maxQuadrant - minQuadrant)) ||
            ((maxQuadrantY - minQuadrantY) > (maxQuadrant - minQuadrant)))
        {
            // uv range within triangle too big
            success &= false;
        }
        else
        {
            // shift x
            if (maxQuadrantX > maxQuadrant)
            {
                // shift x left
                vec4 shift(float(maxQuadrantX - maxQuadrant), 0.0f, 0.0f, 0.0f);
                for (i = 0; i < 3; i++)
                {
                    MeshBuilderVertex& vertex = this->VertexAt(vIndices[i]);
                    vec4 uv = vertex.GetComponent(uvCompIndex);
                    uv -= shift;
                    vertex.SetComponent(uvCompIndex, uv);
                }
            }
            else if (minQuadrantX < minQuadrant)
            {
                // shift x right
                vec4 shift(float(minQuadrant - minQuadrantX), 0.0f, 0.0f, 0.0f);
                for (i = 0; i < 3; i++)
                {
                    MeshBuilderVertex& vertex = this->VertexAt(vIndices[i]);
                    vec4 uv = vertex.GetComponent(uvCompIndex);
                    uv += shift;
                    vertex.SetComponent(uvCompIndex, uv);
                }
            }

            // shift y
            if (maxQuadrantY > maxQuadrant)
            {
                // shift y down
                vec4 shift(0.0f, float(maxQuadrantY - maxQuadrant), 0.0f, 0.0f);
                for (i = 0; i < 3; i++)
                {
                    MeshBuilderVertex& vertex = this->VertexAt(vIndices[i]);
                    vec4 uv = vertex.GetComponent(uvCompIndex);
                    uv -= shift;
                    vertex.SetComponent(uvCompIndex, uv);
                }
            }
            else if (minQuadrantY < minQuadrant)
            {
                // shift y up
                vec4 shift(0.0f, float(minQuadrant - minQuadrantY), 0.0f, 0.0f);
                for (i = 0; i < 3; i++)
                {
                    MeshBuilderVertex& vertex = this->VertexAt(vIndices[i]);
                    vec4 uv = vertex.GetComponent(uvCompIndex);
                    uv += shift;
                    vertex.SetComponent(uvCompIndex, uv);
                }
            }
        }
    }
    return success;
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
    int numVertices = this->vertexArray.Size();

    // generate a index remapping table and sorted vertex array
    int* indexMap = n_new_array(int, numVertices);
    int* sortMap = n_new_array(int, numVertices);
    int* shiftMap = n_new_array(int, numVertices);

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
            this->vertexArray[sortMap[baseIndex]] == this->vertexArray[sortMap[nextIndex]])
        {
            // mark the vertex as invalid
            this->vertexArray[sortMap[nextIndex]].SetFlag(MeshBuilderVertex::Redundant);

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
        if (this->vertexArray[vertexIndex].CheckFlag(MeshBuilderVertex::Redundant))
            numInvalid++;

        shiftMap[vertexIndex] = numInvalid;
    }

    // fix the triangle's vertex indices, first, remap the old index to a valid index from the indexMap,
    //  then decrement by the shiftMap entry at that index, which contains the number of invalid vertices
    //  in front of that index
    //  fix vertex indices in triangles
    int numTriangles = this->triangleArray.Size();

    int curTriangle;
    for (curTriangle = 0; curTriangle < numTriangles; curTriangle++)
    {
        MeshBuilderTriangle& t = this->triangleArray[curTriangle];
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
    numVertices = this->vertexArray.Size();

    Array<MeshBuilderVertex> newArray(numVertices, numVertices);
    for (vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
    {
        if (!this->vertexArray[vertexIndex].CheckFlag(MeshBuilderVertex::Redundant))
            newArray.Append(vertexArray[vertexIndex]);
    }

    this->vertexArray = newArray;

    // cleanup
    n_delete_array(indexMap);
    n_delete_array(sortMap);
    n_delete_array(shiftMap);
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
MeshBuilder::CalculateTangentsAndBinormals()
{
    int numVertices = this->GetNumVertices();
    int numTriangles = this->GetNumTriangles();
    vec4* tangents1 = new vec4[(size_t)numVertices * 2];
    vec4* tangents2 = tangents1 + numVertices;

    memset(tangents1, 0, numVertices * sizeof(vec4) * 2);
    for (int triangleIndex = 0; triangleIndex < numTriangles; triangleIndex++)
    {
        MeshBuilderTriangle& triangle = this->TriangleAt(triangleIndex);
        int v1Index = triangle.GetVertexIndex(0);
        int v2Index = triangle.GetVertexIndex(1);
        int v3Index = triangle.GetVertexIndex(2);

        MeshBuilderVertex& vertex1 = this->VertexAt(v1Index);
        MeshBuilderVertex& vertex2 = this->VertexAt(v2Index);
        MeshBuilderVertex& vertex3 = this->VertexAt(v3Index);

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

        vec4 sdir = vec4((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r, 0.0f);
        vec4 tdir = vec4((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r, 0.0f);

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
        const MeshBuilderVertex::ComponentIndex normalComponent = vertex.HasComponent(MeshBuilderVertex::ComponentBit::NormalBit) ? MeshBuilderVertex::NormalIndex : MeshBuilderVertex::NormalB4NIndex;
        vec4 n = normalize3(vertex.GetComponent(normalComponent));
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
