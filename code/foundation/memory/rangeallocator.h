#pragma once
//------------------------------------------------------------------------------
/**
    @file rangeallocator.h

    @copyright
    (C) 2023-2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "util/bit.h"
#include "util/array.h"
#include <utility>

namespace Memory
{

//------------------------------------------------------------------------------
/**
    @struct Memory::RangeAllocation
    
    @brief Describes a range allocated by the Memory::RangeAllocator
*/
struct RangeAllocation
{
    uint offset;
    uint size;
    uint node;

    static constexpr uint OOM = 0xFFFFFFFF;
};

//------------------------------------------------------------------------------
/**
    @class Memory::RangeAllocator

    @brief Allocates memory ranges using the TLSF method, 
    with extended handling of padding to better suit GPUs.

    @details Doesn't manage memory itself but is meant to be used with a memory allocation/buffer of 
    maxSize. Fast O(1) insertion and deletion and with minimal fragmentation.

    More details: http://www.gii.upv.es/tlsf/files/ecrts04_tlsf.pdf
*/
class RangeAllocator
{
public:
    /// Default constructor
    RangeAllocator();
    /// Construct with predetermined size and max allowed number of allocations
    RangeAllocator(uint size, SizeT maxNumAllocs);
    /// Destructor
    ~RangeAllocator();

    /// Clears the allocator
    void Clear();

    /// Checks if the allocator is empty
    bool Empty();

    /// Allocate range
    RangeAllocation Alloc(uint size, uint alignment = 1);
    /// Deallocate range
    void Dealloc(const RangeAllocation& allocation);

private:
    /// Insert node at bin location, return node index
    uint InsertNode(uint size, uint offset);
    /// Remove node from bin
    void RemoveNode(uint nodeIndex);

    struct RangeAllocatorNode
    {
        RangeAllocatorNode() :
            resident(false)
            , size(0)
            , offset(0)
            , binPrev(END)
            , binNext(END)
            , blockPrev(END)
            , blockNext(END)
        {};
        bool resident;
        uint size;
        uint offset;

        static constexpr uint END = 0xFFFFFFFF;
        uint binPrev, binNext;
        uint blockPrev, blockNext;
    };

    union BinIndex
    {
        BinIndex() : index(0) {};
        struct
        {
            uint16_t bin : 4;
            uint16_t bucket : 12;
        };
        uint16_t index;
    };

    /// Get bin index from size
    static BinIndex IndexFromSize(uint size, bool round = false);
    /// Get bucket from index
    static uint BucketFromSize(uint size);
    /// Get bin from index
    static uint BinFromSize(uint size, uint bucket);

    uint size;
    uint freeStorage;
    uint freeNodeIterator;

    static constexpr uint NUM_BUCKETS = 0x20;
    static constexpr uint NUM_BINS_PER_BUCKET = 0x10;

    uint bucketUsageMask;
    uint binMasks[NUM_BUCKETS];
    uint binHeads[NUM_BUCKETS * NUM_BINS_PER_BUCKET];

    Util::Array<RangeAllocatorNode> nodes;
    Util::Array<uint> freeNodes;
};

//------------------------------------------------------------------------------
/**
*/
inline 
RangeAllocator::RangeAllocator() :
    RangeAllocator(0,0)
{
}

//------------------------------------------------------------------------------
/**
*/
inline
RangeAllocator::RangeAllocator(uint size, SizeT maxNumAllocs)
{   
    this->size = size;
    this->freeNodes.Resize(maxNumAllocs);
    this->nodes.Resize(maxNumAllocs);

    // Clear the allocator
    this->Clear();
}

//------------------------------------------------------------------------------
/**
*/
inline 
RangeAllocator::~RangeAllocator()
{
}

//------------------------------------------------------------------------------
/**
*/
inline void 
RangeAllocator::Clear()
{
    // Reset bit masks
    this->bucketUsageMask = 0x0;
    memset(this->binMasks, 0x0, sizeof(this->binMasks));

    // Reset bin heads
    for (uint i = 0; i < NUM_BUCKETS * NUM_BINS_PER_BUCKET; i++)
    {
        this->binHeads[i] = RangeAllocatorNode::END;
    }

    // Clear nodes
    for (uint i = 0; i < this->nodes.Size(); i++)
    {
        this->freeNodes[i] = this->nodes.Size() - i - 1;
    }
    this->freeNodeIterator = this->nodes.Size() - 1;
    this->freeStorage = 0;

    // Insert node covering the whole range
    if (this->size > 0)
        this->InsertNode(this->size, 0);
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
RangeAllocator::Empty()
{
    return this->freeStorage == this->size;
}

//------------------------------------------------------------------------------
/**
   @brief Returns a range using the TLSF method.

   @note Note that this doesn't actually allocate any memory, but 
   instead returns a partial range within [0 -> size] that you can use
   with an external buffer to keep track of memory chunks
*/
inline RangeAllocation 
RangeAllocator::Alloc(uint size, uint alignment)
{
    // We are not allowed any more allocations
    uint alignedSize = size + alignment - 1;
    if (this->freeStorage < alignedSize)
    {
        return RangeAllocation{ .offset = RangeAllocation::OOM, .size = 0, .node = RangeAllocatorNode::END };
    }

    BinIndex minIndex = IndexFromSize(alignedSize, true);
    uint bin = 0xFFFFFFFF;
    uint bucket = minIndex.bucket;

    if (this->bucketUsageMask & (1 << bucket))
    {
        bin = Util::Lsb(this->binMasks[bucket], minIndex.bin);
    }

    if (bin == 0xFFFFFFFF)
    {
        // Find next bit set after bucket in the usage mask
        bucket = Util::Lsb(this->bucketUsageMask, bucket + 1);

        // If this means we get 32, it means we're out of buckets that fit
        if (bucket == 0xFFFFFFFF)
        {
            return RangeAllocation{ .offset = RangeAllocation::OOM, .size = 0, .node = RangeAllocatorNode::END };
        }

        // Find any bit, since this bucket has to fit
        bin = Util::FirstOne(this->binMasks[bucket]);
    }

    BinIndex binIndex;
    binIndex.bucket = bucket;
    binIndex.bin = bin;

    // Get linked list head
    uint nodeIndex = this->binHeads[binIndex.index];
    RangeAllocatorNode& node = this->nodes[nodeIndex];
    n_assert(!node.resident);

    // Save total size of node
    uint totalSize = node.size;
    uint baseOffset = node.offset;
    node.size = alignedSize;
    node.resident = true;
    node.offset = Math::align(node.offset, alignment);

    // Bump head of bin to next node
    this->binHeads[binIndex.index] = node.binNext;
    if (node.binNext != RangeAllocatorNode::END)
        this->nodes[node.binNext].binPrev = RangeAllocatorNode::END;

    // If bin head is empty after we grab the node, unmark the bin
    if (this->binHeads[binIndex.index] == RangeAllocatorNode::END)
    {
        this->binMasks[binIndex.bucket] &= ~(1 << binIndex.bin);

        // If all bins are busy, unmark the bucket bits
        if (this->binMasks[binIndex.bucket] == 0)
        {
            this->bucketUsageMask &= ~(1 << binIndex.bucket);
        }
    }

    // We subtract the entire size of the node
    this->freeStorage -= totalSize;
    
    // The remainder in the front is then added back
    uint remainder = totalSize - node.size;
    if (remainder > 0)
    {
        uint newNodeIndex = this->InsertNode(remainder, node.offset + size);
        if (newNodeIndex != RangeAllocatorNode::END)
        {
            RangeAllocatorNode& newNode = this->nodes[newNodeIndex];

            // If the current node has a next, repoint it to the new block
            if (node.blockNext != RangeAllocatorNode::END)
            {
                this->nodes[node.blockNext].blockPrev = newNodeIndex;
            }
            newNode.blockPrev = nodeIndex;
            newNode.blockNext = node.blockNext;
            node.blockNext = newNodeIndex;
        }
    }
    
    return RangeAllocation{ .offset = node.offset, .size = alignedSize, .node = nodeIndex };
}

//------------------------------------------------------------------------------
/**
*/
inline void 
RangeAllocator::Dealloc(const RangeAllocation& allocation)
{
    RangeAllocatorNode& node = this->nodes[allocation.node];
    n_assert(node.resident);
    node.resident = false;

    uint size = node.size;
    uint offset = node.offset;

    // Try to merge with left
    if (node.blockPrev != RangeAllocatorNode::END)
    {
        RangeAllocatorNode& prev = this->nodes[node.blockPrev];
        if (!prev.resident)
        {
            offset = prev.offset;
            size += prev.size;

            this->RemoveNode(node.blockPrev);
            node.blockPrev = prev.blockPrev;
        }
    }

    if (node.blockNext != RangeAllocatorNode::END)
    {
        RangeAllocatorNode& next = this->nodes[node.blockNext];
        if (!next.resident)
        {
            size += next.size;
            this->RemoveNode(node.blockNext);
            node.blockNext = next.blockNext;
        }
    }

    uint leftNode = node.blockPrev;
    uint rightNode = node.blockNext;
    this->freeNodes[++this->freeNodeIterator] = allocation.node;

    // Add merged node
    uint mergedNode = this->InsertNode(size, offset);

    // Connect adjacent nodes with newly inserted node
    if (leftNode != RangeAllocatorNode::END)
    {
        this->nodes[leftNode].blockNext = mergedNode;
        this->nodes[mergedNode].blockPrev = leftNode;
    }

    if (rightNode != RangeAllocatorNode::END)
    {
        this->nodes[rightNode].blockPrev = mergedNode;
        this->nodes[mergedNode].blockNext = rightNode;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline uint 
RangeAllocator::InsertNode(uint size, uint offset)
{
    if (this->freeNodeIterator == 0xFFFFFFFF)
        return RangeAllocatorNode::END;

    BinIndex index = IndexFromSize(size);

    if (this->binHeads[index.index] == RangeAllocatorNode::END)
    {
        // Bin isn't used, flip bit for bucket and bin
        this->binMasks[index.bucket] |= (1 << index.bin);
        this->bucketUsageMask |= (1 << index.bucket);
    }

    // Get new node
    uint headNodeIndex = this->binHeads[index.index];
    uint newNodeIndex = this->freeNodes[this->freeNodeIterator--];
    RangeAllocatorNode& newNode = this->nodes[newNodeIndex];

    // Set the size and offset of the node memory
    newNode.size = size;
    newNode.offset = offset;
    newNode.binNext = headNodeIndex;
    newNode.binPrev = RangeAllocatorNode::END;
    newNode.resident = false;

    this->freeStorage += size;

    // Get node at head of bin
    if (headNodeIndex != RangeAllocatorNode::END)
    {
        // If valid, repoint head node to point to the new node in the bin
        RangeAllocatorNode& headNode = this->nodes[headNodeIndex];
        headNode.binPrev = newNodeIndex;
    }
    this->binHeads[index.index] = newNodeIndex;

    return newNodeIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
RangeAllocator::RemoveNode(uint nodeIndex)
{
    // Get node to free
    RangeAllocatorNode& node = this->nodes[nodeIndex];
    node.resident = false;

    // Add back the storage to the allocator
    this->freeStorage -= node.size;
    
    if (node.binPrev == RangeAllocatorNode::END)
    {
        BinIndex index = IndexFromSize(node.size);

        // Point head of 
        this->binHeads[index.index] = node.binNext;
        if (node.binNext != RangeAllocatorNode::END)
        {
            this->nodes[node.binNext].binPrev = RangeAllocatorNode::END;
        }

        if (node.binNext == RangeAllocatorNode::END)
        {
            // If we are deleting the last node, make sure to unmark the bin
            this->binMasks[index.bucket] &= ~(1 << index.bin);

            if (this->binMasks[index.bucket] == 0x0)
            {
                // If all bins have been cleared, clear the entire bucket
                this->bucketUsageMask &= ~(1 << index.bucket);
            }
        }
    } 
    else
    {
        this->nodes[node.binPrev].binNext = node.binNext;
        if (node.binNext != RangeAllocatorNode::END)
        {
            this->nodes[node.binNext].binPrev = node.binPrev;
        }
    }

    this->freeNodes[++this->freeNodeIterator] = nodeIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
RangeAllocator::BucketFromSize(uint size)
{
#if __WIN32__
    DWORD count = 0;
    _BitScanReverse(&count, size);
#else
    int count = 31 - __builtin_clz(size);
#endif
    return count;
}

//------------------------------------------------------------------------------
/**
*/
inline uint 
RangeAllocator::BinFromSize(uint size, uint bucket)
{
    uint mask = (size >> (bucket - 4)) & (NUM_BINS_PER_BUCKET - 1u);
    return mask;
}

//------------------------------------------------------------------------------
/**
*/
inline RangeAllocator::BinIndex
RangeAllocator::IndexFromSize(uint size, bool round)
{
    RangeAllocator::BinIndex ret;
    ret.bucket = BucketFromSize(size);
    ret.bin = BinFromSize(size, ret.bucket);
    if (round)
    {
        uint mask = (1 << ret.bucket) | (ret.bin << (ret.bucket - 4));
        if ((~mask & size) != 0)
            ret.bin++;
    }
    return ret;
}

} // namespace Memory
