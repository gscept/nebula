#pragma once
//------------------------------------------------------------------------------
/**
    A quad tree designed to return regions of free 2D space.

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "math/scalar.h"
#include "util/fixedarray.h"
#include "memory/arenaallocator.h"

namespace Util   
{

class OccupancyQuadTree
{
public:
    /// constructor 
    OccupancyQuadTree();
    /// destructor
    ~OccupancyQuadTree();

    /// setup with a world size and a biggest allocation size
    void Setup(uint textureSize, uint maxSize, uint minSize);
    /// allocate a region, return region
    Math::uint2 Allocate(uint size);
    /// deallocate region
    bool Deallocate(const Math::uint2 coord, uint size);
    /// check if region is alloced
    bool IsOccupied(const Math::uint2 coord, uint size);
    /// Clear the tree
    void Clear();

    struct Node
    {
        Node()
            : topLeft(nullptr)
            , topRight(nullptr)
            , bottomLeft(nullptr)
            , bottomRight(nullptr)
            , occupied(false)
            , occupancyCounter(0)
            , size(1)
            , x(UINT32_MAX)
            , y(UINT32_MAX)
        {
        }
        Node* topLeft;
        Node* topRight;
        Node* bottomLeft;
        Node* bottomRight;
        bool occupied;
        uint occupancyCounter;
        uint size;
        uint x, y;
    };

    const Util::FixedArray<Util::FixedArray<Node>>& GetTopLevelNodes();

private:

    /// recursively traverse tree to allocate node from tree
    bool RecursiveAllocate(Node* node, uint size, Math::uint2& outCoord);
    /// recursively traverse tree and deallocate
    bool RecursiveDeallocate(Node* node, Math::uint2 coord, uint size);
    /// recursively traverse tree and find if allocated
    bool RecursiveSearch(Node* node, Math::uint2 coord, uint size);

    Memory::ArenaAllocator<sizeof(Node) * 64> allocator;
    Util::FixedArray<Util::FixedArray<Node>> topLevelNodes;
    uint minSize;
};

//------------------------------------------------------------------------------
/**
*/
inline 
OccupancyQuadTree::OccupancyQuadTree()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline 
OccupancyQuadTree::~OccupancyQuadTree()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void 
OccupancyQuadTree::Setup(uint textureSize, uint maxSize, uint minSize)
{
    this->minSize = minSize;
    uint numNodes = textureSize / maxSize;
    this->topLevelNodes.Resize(numNodes);
    for (uint x = 0; x < numNodes; x++)
    {
        this->topLevelNodes[x].Resize(numNodes);
        for (uint y = 0; y < numNodes; y++)
        {
            this->topLevelNodes[x][y].x = x * maxSize;
            this->topLevelNodes[x][y].y = y * maxSize;
            this->topLevelNodes[x][y].size = maxSize;
            this->topLevelNodes[x][y].occupied = false;
            this->topLevelNodes[x][y].occupancyCounter = 0;
            this->topLevelNodes[x][y].topLeft = nullptr;
            this->topLevelNodes[x][y].topRight = nullptr;
            this->topLevelNodes[x][y].bottomLeft = nullptr;
            this->topLevelNodes[x][y].bottomRight = nullptr;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
inline bool
OccupancyQuadTree::RecursiveAllocate(OccupancyQuadTree::Node* node, uint size, Math::uint2& outCoord)
{
    // if the node is not occupied, proceed
    if (!node->occupied)
    {
        // if size matches and no children are used, return
        if (node->size == size && node->occupancyCounter == 0)
        {
            n_assert(node->occupied == false);
            outCoord = Math::uint2{ node->x, node->y };
            node->occupied = true;
            return true;
        }
        else if (node->size > size && node->size != this->minSize)
        {
            // if the size of the node is bigger than the requested size, visit the children
            if (node->topLeft == nullptr)
            {
                // initialize children, topleft is the origin point
                const uint halfNodeSize = node->size / 2;
                node->topLeft = this->allocator.Alloc<Node>();
                node->topLeft->x = node->x;
                node->topLeft->y = node->y;
                node->topLeft->size = halfNodeSize;

                node->topRight = this->allocator.Alloc<Node>();
                node->topRight->x = node->x + halfNodeSize;
                node->topRight->y = node->y;
                node->topRight->size = halfNodeSize;

                node->bottomLeft = this->allocator.Alloc<Node>();
                node->bottomLeft->x = node->x;
                node->bottomLeft->y = node->y + halfNodeSize;
                node->bottomLeft->size = halfNodeSize;

                node->bottomRight = this->allocator.Alloc<Node>();
                node->bottomRight->x = node->x + halfNodeSize;
                node->bottomRight->y = node->y + halfNodeSize;
                node->bottomRight->size = halfNodeSize;
            }

            if (RecursiveAllocate(node->topLeft, size, outCoord))
            {
                node->occupancyCounter++;
                return true;
            }
            if (RecursiveAllocate(node->topRight, size, outCoord))
            {
                node->occupancyCounter++;
                return true;
            }
            if (RecursiveAllocate(node->bottomLeft, size, outCoord))
            {
                node->occupancyCounter++;
                return true;
            }
            if (RecursiveAllocate(node->bottomRight, size, outCoord))
            {
                node->occupancyCounter++;
                return true;
            }
        }
    }

    return false;
}

//------------------------------------------------------------------------------
/**
*/
inline Math::uint2 
OccupancyQuadTree::Allocate(uint size)
{
    Math::uint2 ret{ UINT32_MAX, UINT32_MAX };
    for (int x = 0; x < this->topLevelNodes.Size(); x++)
    {
        for (int y = 0; y < this->topLevelNodes[x].Size(); y++)
        {
            // try to allocate node in this subtree, if failed, just skip to the next top-level node
            if (RecursiveAllocate(&this->topLevelNodes[x][y], size, ret))
                return ret;
        }
    }
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
OccupancyQuadTree::RecursiveDeallocate(Node* node, Math::uint2 coord, uint size)
{
    // if node matches size, look for the coordinate
    if (node->size == size)
    {
        if (node->x == coord.x && node->y == coord.y)
        {
            node->occupied = false;
            return true;
        }
    }
    else if (node->size > size)
    {
        // if node size is bigger than the requested size, visit children and unset the bits 
        if (node->topLeft == nullptr)
            return false;

        if (RecursiveDeallocate(node->topLeft, coord, size))
        {
            node->occupancyCounter--;
            return true;
        }
        if (RecursiveDeallocate(node->topRight, coord, size))
        {
            node->occupancyCounter--;
            return true;
        }
        if (RecursiveDeallocate(node->bottomLeft, coord, size))
        {
            node->occupancyCounter--;
            return true;
        }
        if (RecursiveDeallocate(node->bottomRight, coord, size))
        {
            node->occupancyCounter--;
            return true;
        }
    }

    return false;
}


//------------------------------------------------------------------------------
/**
*/
inline bool 
OccupancyQuadTree::Deallocate(const Math::uint2 coord, uint size)
{
    // find root node where the coord belongs
    for (int x = 0; x < this->topLevelNodes.Size(); x++)
    {
        for (int y = 0; y < this->topLevelNodes[x].Size(); y++)
        {
            Node& node = this->topLevelNodes[x][y];
            if (RecursiveDeallocate(&node, coord, size))
                return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
OccupancyQuadTree::RecursiveSearch(Node* node, Math::uint2 coord, uint size)
{
    // if node matches size, look for the coordinate
    if (node->size == size)
    {
        if (node->x == coord.x && node->y == coord.y)
        {
            return node->occupied;
        }
    }
    else if (node->size > size)
    {
        // if node size is bigger than the requested size, visit children and unset the bits 
        if (node->topLeft == nullptr)
            return false;

        if (RecursiveDeallocate(node->topLeft, coord, size))
        {
            return true;
        }
        if (RecursiveDeallocate(node->topRight, coord, size))
        {
            return true;
        }
        if (RecursiveDeallocate(node->bottomLeft, coord, size))
        {
            return true;
        }
        if (RecursiveDeallocate(node->bottomRight, coord, size))
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
/**
*/
inline void
OccupancyQuadTree::Clear()
{
    this->allocator.Release();
    for (uint x = 0; x < this->topLevelNodes.Size(); x++)
    {
        for (uint y = 0; y < this->topLevelNodes[x].Size(); y++)
        {
            this->topLevelNodes[x][y].topLeft = nullptr;
            this->topLevelNodes[x][y].topRight = nullptr;
            this->topLevelNodes[x][y].bottomLeft = nullptr;
            this->topLevelNodes[x][y].bottomRight = nullptr;
            this->topLevelNodes[x][y].occupied = false;
            this->topLevelNodes[x][y].occupancyCounter = 0;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
OccupancyQuadTree::IsOccupied(const Math::uint2 coord, uint size)
{
    // find root node where the coord belongs
    for (int x = 0; x < this->topLevelNodes.Size(); x++)
    {
        for (int y = 0; y < this->topLevelNodes[x].Size(); y++)
        {
            Node& node = this->topLevelNodes[x][y];
            if (RecursiveSearch(&node, coord, size))
                return true;
        }
    }
    return false;
}

} // namespace Terrain
