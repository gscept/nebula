#pragma once
//------------------------------------------------------------------------------
/**
    @class Bvh

    A generic bounding volume (AABB) hierarchy

    @note

    This is a modified version of the BVH by Jacco:
    https://jacco.ompf2.com/2022/04/21/how-to-build-a-bvh-part-3-quick-builds/

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "math/bbox.h"
#include "math/line.h"

namespace Util
{

class Bvh
{
public:
    ~Bvh();

    /// Builds the bvh tree
    void Build(Math::bbox* bboxes, uint32_t numBoxes);

    /// returns all intersected bboxes indices based on the order they were when passed to the Build method.
    Util::Array<uint32_t> Intersect(Math::line line);

//private:
    class Node
    {
    public:
        float CalculateCost() const
        {
            return this->count * this->bbox.area();
        }

        bool IsLeaf() const
        {
            return count > 0;
        }

    //private:
        friend Bvh;
        /// the bounding box of the node
        Math::bbox bbox;
        /// left node, or index to first child if count is zero
        uint32_t index = -1;
        /// number of children
        uint32_t count;
    };

    void UpdateNodeBounds(Bvh::Node* node, Math::bbox* bboxes);
    void Subdivide(Bvh::Node* node, Math::bbox* bboxes);
    float FindBestSplitPlane(Bvh::Node* node, Math::bbox* bboxes, int& axis, float& splitPos);

    void Clear();

    Bvh::Node* nodes = nullptr;
    /// these map to where the original bbox was when passed to the build method.
    uint32_t* externalIndices = nullptr;
    uint32_t rootNodeIndex = 0;
    uint32_t numNodes = 0;
    uint32_t nodesUsed = 0;
};

//------------------------------------------------------------------------------
/**
*/
Bvh::~Bvh()
{
    this->Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
Bvh::Build(Math::bbox* bboxes, uint32_t numBoxes)
{
    this->Clear();

    this->numNodes = numBoxes * 2 - 1;
    this->nodes = new Bvh::Node[numBoxes * 2 - 1];
    this->nodesUsed = 1;
    this->externalIndices = new uint32_t[numBoxes];
    for (uint32_t i = 0; i < numBoxes; i++)
        this->externalIndices[i] = i;

    Bvh::Node& root = this->nodes[this->rootNodeIndex];
    root.index = 0;
    root.count = numBoxes;
    this->UpdateNodeBounds(&root, bboxes);
    // subdivide recursively
    this->Subdivide(&root, bboxes);
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<uint32_t>
Bvh::Intersect(Math::line line)
{
    Util::Array<uint32_t> ret;

    line.m = Math::normalize(line.m);

    Bvh::Node* node = this->nodes;
    Bvh::Node* stack[64];
    uint stackPtr = 0;
    while (true)
    {
        if (node->IsLeaf())
        {
            // We've got an intersection!
            for (uint32_t i = 0; i < node->count; i++)
            {
                ret.Append(this->externalIndices[node->index + i]);
            }

            if (stackPtr == 0)
                break;
            else
                node = stack[--stackPtr];

            continue;
        }
        Bvh::Node* child1 = &this->nodes[node->index];
        Bvh::Node* child2 = &this->nodes[node->index + 1];
        float dist1;
        float dist2;
        
        child1->bbox.intersects(line, dist1);
        child2->bbox.intersects(line, dist2);

        if (dist1 > dist2)
        {
            std::swap(dist1, dist2);
            std::swap(child1, child2);
        }
        if (dist1 >= 1e30f)
        {
            if (stackPtr == 0)
            {
                break;
            }
            else
            {
                node = stack[--stackPtr];
            }
        }
        else
        {
            node = child1;
            if (dist2 < 1e30f)
            {
                stack[stackPtr++] = child2;
            }
        }
    }

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
Bvh::UpdateNodeBounds(Bvh::Node* node, Math::bbox* bboxes)
{
    node->bbox.begin_extend();
    uint32_t const end = node->index + node->count;
	for (uint32_t i = node->index; i < end; i++)
	{
        uint32_t index = this->externalIndices[i];
        Math::bbox const& leafBBox = bboxes[index];
        node->bbox.extend(leafBBox);
	}
    node->bbox.end_extend();
}

//------------------------------------------------------------------------------
/**
*/
void
Bvh::Subdivide(Bvh::Node* node, Math::bbox* bboxes)
{
    if (node->count <= 2)
        return;

    // calculate splitting plane
    int axis;
    float splitPos;
    float const splitCost = FindBestSplitPlane(node, bboxes, axis, splitPos);
    float const nosplitCost = node->CalculateCost();
    if (splitCost >= nosplitCost)
        return;

    // split group into two halves
    // just swap elements to be to the left or right of a split in the aabb array
    int i = node->index;
    int j = i + node->count - 1;
    while (i <= j)
    {
        uint32_t const idx = this->externalIndices[i];
        float center = (bboxes[idx].pmin[axis] + bboxes[idx].pmax[axis]) * 0.5f;
        if (center < splitPos)
            i++;
        else
            std::swap(this->externalIndices[i], this->externalIndices[j--]);
    }

    int leftCount = i - node->index;
    if (leftCount == 0 || leftCount == node->count)
        return;
    // create child nodes
    int leftChildIdx = this->nodesUsed++;
    int rightChildIdx = this->nodesUsed++;
    this->nodes[leftChildIdx].index = node->index;
    this->nodes[leftChildIdx].count = leftCount;
    this->nodes[rightChildIdx].index = i;
    this->nodes[rightChildIdx].count = node->count - leftCount;
    node->index = leftChildIdx;
    node->count = 0;
    UpdateNodeBounds(this->nodes + leftChildIdx, bboxes);
    UpdateNodeBounds(this->nodes + rightChildIdx, bboxes);
    Subdivide(this->nodes + leftChildIdx, bboxes);
    Subdivide(this->nodes + rightChildIdx, bboxes);
}

//------------------------------------------------------------------------------
/**
*/
float
Bvh::FindBestSplitPlane(Bvh::Node* node, Math::bbox* bboxes, int& axis, float& splitPos)
{
    constexpr uint32_t intervals = 8;
    float bestCost = 1e30f;
    for (uint32_t a = 0; a < 3; a++)
    {
        float boundsMin = 1e30f;
        float boundsMax = -1e30f;
        for (uint32_t i = 0; i < node->count; i++)
        {
            Math::bbox const& bbox = bboxes[this->externalIndices[node->index + i]];
            float center = (bbox.pmax[a] + bbox.pmin[a]) * 0.5f;
            boundsMin = Math::min(boundsMin, center);
            boundsMax = Math::max(boundsMax, center);
        }
        
        if (boundsMin == boundsMax)
            continue;
        
        struct Bin
        {
            Math::bbox bounds;
            uint32_t count = 0;
        };

        // populate the bins
        Bin bin[intervals];

        float scale = (float)intervals / (boundsMax - boundsMin);
        for (uint32_t i = 0; i < node->count; i++)
        {
            Math::bbox const& bbox = bboxes[this->externalIndices[node->index + i]];
            float center = (bbox.pmax[a] + bbox.pmin[a]) * 0.5f;
            uint32_t binIdx = Math::min(intervals - 1, (uint32_t)((center - boundsMin) * scale));
            bin[binIdx].count++;
            bin[binIdx].bounds.extend(bbox);
        }
        // gather data for the 7 planes between the 8 bins
        float leftArea[intervals - 1];
        float rightArea[intervals - 1];
        uint32_t leftCount[intervals - 1];
        uint32_t rightCount[intervals - 1];
        Math::bbox leftBox;
        leftBox.begin_extend();
        Math::bbox rightBox;
        rightBox.begin_extend();
        uint32_t leftSum = 0;
        uint32_t rightSum = 0;
        for (uint32_t i = 0; i < intervals - 1; i++)
        {
            leftSum += bin[i].count;
            leftCount[i] = leftSum;
            leftBox.extend(bin[i].bounds);
            leftArea[i] = leftBox.area();
            rightSum += bin[intervals - 1 - i].count;
            rightCount[intervals - 2 - i] = rightSum;
            rightBox.extend(bin[intervals - 1 - i].bounds);
            rightArea[intervals - 2 - i] = rightBox.area();
        }
        // calculate SAH cost for the 7 planes
        scale = (boundsMax - boundsMin) / intervals;
        for (int i = 0; i < intervals - 1; i++)
        {
            float planeCost = leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
            if (planeCost < bestCost)
            {
                axis = a;
                splitPos = boundsMin + scale * (i + 1);
                bestCost = planeCost;
            }
        }
    }
    return bestCost;
}

//------------------------------------------------------------------------------
/**
*/
void
Bvh::Clear()
{
    if (this->nodes != nullptr)
        delete[] this->nodes;
    if (this->externalIndices != nullptr)
        delete[] this->externalIndices;

    this->nodes = nullptr;
    this->externalIndices = nullptr;
    this->nodesUsed = 0;
    this->rootNodeIndex = 0;
    this->numNodes = 0;
}

} // namespace Util
