#pragma once
//------------------------------------------------------------------------------
/**
    Allocates memory using the TLSF method (http://www.gii.upv.es/tlsf/files/ecrts04_tlsf.pdf), 
    with extended handling of padding to better suit GPUs.

    Doesn't manage memory itself but is meant to be used with a memory allocation of 
    maxSize. Fast O(1) insertion and deletion and with minimal fragmentation.

    @copyright
    (C) 2023 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "util/array.h"
#include <utility>
namespace Memory
{

/// Get bucket from index
uint BucketFromSize(uint size);
/// Get bin from index
uint BinFromSize(uint size, uint bucket);
/// Get least significant bit from mask
uint Lsb(uint mask, byte bit);
/// Get first one in mask
uint FirstOne(uint value);

struct SizeClassificationAllocation
{
    uint offset;
    uint node;

    static constexpr uint OOM = 0xFFFFFFFF;
};

class SizeClassificationAllocator
{
public:
    /// Default constructor
    SizeClassificationAllocator();
    /// Constructor
    SizeClassificationAllocator(SizeT size, SizeT maxNumAllocs);
    /// Destructor
    ~SizeClassificationAllocator();

    /// Clear allocator
    void Clear();

    /// Empty
    bool Empty();

    /// Allocate memory
    SizeClassificationAllocation Alloc(uint size, uint alignment = 1);
    /// Deallocate memory
    void Dealloc(const SizeClassificationAllocation& allocation);

private:
    /// Insert node at bin location, return node index
    uint InsertNode(uint size, uint offset);
    /// Remove node from bin
    void RemoveNode(uint nodeIndex);

    struct SizeClassificationNode
    {
        SizeClassificationNode() :
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
            uint16 bin : 4;
            uint16 bucket : 12;
        };
        uint16 index;
    };

    /// Get bin index from size
    static BinIndex IndexFromSize(uint size);

    uint size;
    uint freeStorage;
    uint freeNodeIterator;

    static constexpr uint NUM_BUCKETS = 0x20;
    static constexpr uint NUM_BINS_PER_BUCKET = 0xF;
    uint bucketUsageMask;
    uint binMasks[NUM_BUCKETS];
    uint binHeads[NUM_BUCKETS * NUM_BINS_PER_BUCKET];

    Util::Array<SizeClassificationNode> nodes;
    Util::Array<uint> freeNodes;

    using SCNode = SizeClassificationNode;
};

using SCAlloc = SizeClassificationAllocation;
using SCAllocator = SizeClassificationAllocator;

//------------------------------------------------------------------------------
/**
*/
inline 
SizeClassificationAllocator::SizeClassificationAllocator() :
    SizeClassificationAllocator(0,0)
{
}

//------------------------------------------------------------------------------
/**
*/
inline
SizeClassificationAllocator::SizeClassificationAllocator(SizeT size, SizeT maxNumAllocs)
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
SizeClassificationAllocator::~SizeClassificationAllocator()
{
}

//------------------------------------------------------------------------------
/**
*/
inline void 
SizeClassificationAllocator::Clear()
{
    // Reset bit masks
    this->bucketUsageMask = 0x0;
    memset(this->binMasks, 0x0, sizeof(this->binMasks));

    // Reset bin heads
    for (uint i = 0; i < NUM_BUCKETS * NUM_BINS_PER_BUCKET; i++)
    {
        this->binHeads[i] = SCNode::END;
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
SizeClassificationAllocator::Empty()
{
    return this->freeStorage == this->size;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeClassificationAllocation 
SizeClassificationAllocator::Alloc(uint size, uint alignment)
{
    // We are not allowed any more allocations
    if (this->freeStorage < size)
    {
        return { SCAlloc::OOM, SCAlloc::OOM };
    }

    BinIndex minIndex = IndexFromSize(size);

    uint bin = 0xFF;
    uint bucket = minIndex.bucket;

    if (this->bucketUsageMask & (1 << bucket))
    {
        bin = Lsb(this->binMasks[bucket], minIndex.bin);
    }

    if (bin > NUM_BINS_PER_BUCKET)
    {
        // Find next bit set after bucket in the usage mask
        bucket = Lsb(this->bucketUsageMask, bucket + 1);

        // If this means we get 32, it means we're out of buckets that fit
        if (bucket > NUM_BUCKETS)
        {
            return { SCAlloc::OOM, SCAlloc::OOM };
        }

        // Find any bit, since this bucket has to fit
        bin = FirstOne(this->binMasks[bucket]);
    }

    BinIndex binIndex;
    binIndex.bucket = bucket;
    binIndex.bin = bin;

    // Get linked list head
    uint nodeIndex = this->binHeads[binIndex.index];
    SCNode& node = this->nodes[nodeIndex];

    // Calculate padding required by alignment
    uint padding = Math::align(node.offset, alignment) - node.offset;

    // Since we get a new size, need to check there is space with padding
    if (this->freeStorage < (size + padding))
    {
        return { SCAlloc::OOM, SCAlloc::OOM };
    }

    // Save total size of node
    uint totalSize = node.size;
    uint baseOffset = node.offset;
    node.size = size;
    node.resident = true;

    // Offset by padding
    node.offset += padding;

    // Bump head of bin to next node
    this->binHeads[binIndex.index] = node.binNext;
    if (node.binNext != SCNode::END)
        this->nodes[node.binNext].binPrev = SCNode::END;

    // If bin head is empty after we grab the node, unmark the bin
    if (this->binHeads[binIndex.index] == SCNode::END)
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
    uint remainder = totalSize - padding - node.size;
    if (remainder > 0)
    {
        uint newNodeIndex = this->InsertNode(remainder, node.offset + size);
        SCNode& newNode = this->nodes[newNodeIndex];

        // If the current node has a next, repoint it to the new block
        if (node.blockNext != SCNode::END)
        {
            this->nodes[node.blockNext].blockPrev = newNodeIndex;
        }
        newNode.blockPrev = nodeIndex;
        newNode.blockNext = node.blockNext;
        node.blockNext = newNodeIndex;
    }

    // If there was any padding offset, that memory is also added back
    if (padding > 0)
    {
        uint newNodeIndex = this->InsertNode(padding, baseOffset);
        SCNode& newNode = this->nodes[newNodeIndex];

        if (node.blockPrev != SCNode::END)
        {
            this->nodes[node.blockPrev].blockNext = newNodeIndex;
        }
        if (node.blockNext != SCNode::END)
        {
            this->nodes[node.blockNext].blockPrev = newNodeIndex;
        }
        newNode.blockPrev = node.blockPrev;
        newNode.blockNext = nodeIndex;
        node.blockPrev = newNodeIndex;
    }
    
    return { node.offset, nodeIndex };
}

//------------------------------------------------------------------------------
/**
*/
inline void 
SizeClassificationAllocator::Dealloc(const SizeClassificationAllocation& allocation)
{
    SCNode& node = this->nodes[allocation.node];
    n_assert(node.resident);
    node.resident = false;

    uint size = node.size;
    uint offset = node.offset;

    // Try to merge with left
    if (node.blockPrev != SCNode::END)
    {
        SCNode& prev = this->nodes[node.blockPrev];
        if (!prev.resident)
        {
            offset = prev.offset;
            size += prev.size;

            this->RemoveNode(node.blockPrev);
            node.blockPrev = prev.blockPrev;
        }
    }

    if (node.blockNext != SCNode::END)
    {
        SCNode& next = this->nodes[node.blockNext];
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
    if (leftNode != SCNode::END)
    {
        this->nodes[leftNode].blockNext = mergedNode;
        this->nodes[mergedNode].blockPrev = leftNode;
    }

    if (rightNode != SCNode::END)
    {
        this->nodes[rightNode].blockPrev = mergedNode;
        this->nodes[mergedNode].blockNext = rightNode;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline uint 
SizeClassificationAllocator::InsertNode(uint size, uint offset)
{
    BinIndex index = IndexFromSize(size);

    if (this->binHeads[index.index] == SCNode::END)
    {
        // Bin isn't used, flip bit for bucket and bin
        this->binMasks[index.bucket] |= (1 << index.bin);
        this->bucketUsageMask |= (1 << index.bucket);
    }

    // Get new node
    uint headNodeIndex = this->binHeads[index.index];
    uint newNodeIndex = this->freeNodes[this->freeNodeIterator--];
    SCNode& newNode = this->nodes[newNodeIndex];

    // Set the size and offset of the node memory
    newNode.size = size;
    newNode.offset = offset;
    newNode.binNext = headNodeIndex;
    newNode.binPrev = SCNode::END;

    this->freeStorage += size;

    // Get node at head of bin
    if (headNodeIndex != SCNode::END)
    {
        // If valid, repoint head node to point to the new node in the bin
        SCNode& headNode = this->nodes[headNodeIndex];
        headNode.binPrev = newNodeIndex;
    }
    this->binHeads[index.index] = newNodeIndex;

    return newNodeIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
SizeClassificationAllocator::RemoveNode(uint nodeIndex)
{
    // Get node to free
    SCNode& node = this->nodes[nodeIndex];
    node.resident = false;

    // Add back the storage to the allocator
    this->freeStorage -= node.size;
    
    if (node.binPrev == SCNode::END)
    {
        BinIndex index = IndexFromSize(node.size);

        // Point head of 
        this->binHeads[index.index] = node.binNext;
        if (node.binNext != SCNode::END)
        {
            this->nodes[node.binNext].binPrev = SCNode::END;
        }

        if (node.binNext == SCNode::END)
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
        if (node.binNext != SCNode::END)
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
BucketFromSize(uint size)
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
BinFromSize(uint size, uint bucket)
{
    uint mask = (size >> (bucket - 4)) & 0xF;
    return mask;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
Lsb(uint value, byte bit)
{
    uint mask = value & ~((1 << bit) - 1);
#if __WIN32__
    DWORD count = 0;
    _BitScanForward(&count, mask);
#else
    int count = __builtin_ctz(mask);
#endif
    return mask ? count : 0xFF;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
FirstOne(uint value)
{
#if __WIN32__
    DWORD count = 0;
    _BitScanForward(&count, value);
#else
    int count = __builtin_ctz(value);
#endif
    return count;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeClassificationAllocator::BinIndex
SizeClassificationAllocator::IndexFromSize(uint size)
{
    SizeClassificationAllocator::BinIndex ret;
    ret.bucket = BucketFromSize(size);
    ret.bin = BinFromSize(size, ret.bucket);
    return ret;
}

} // namespace Memory
