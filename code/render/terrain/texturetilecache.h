#pragma once
//------------------------------------------------------------------------------
/**
    The texture tile cache keeps track of which texture tiles should be used based 
    on a last recently used heuristic

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "math/scalar.h"
#include "util/hashtable.h"
#include "memory/arenaallocator.h"
#include "ids/idpool.h"
namespace Terrain
{


struct IndirectionEntry
{
    uint mip : 4;
    uint physicalOffsetX : 14;
    uint physicalOffsetY : 14;
};

struct TileCacheEntry
{
    struct Entry
    {
        uint64 mip : 4;                   // max value 15 (0xF)
        uint64 tiles : 10;                // must hold at least SubTextureMaxTiles from TerrainContext.h
        uint64 tileX : 9;                 // must hold at least SubTextureMaxTiles
        uint64 tileY : 9;                 // same as above
        uint64 subTextureIndex : 24;      //
        uint64 subTextureUpdateKey : 8;
    };
    union
    {
        Entry entry;
        uint64 hash;
    };

    bool operator>(const TileCacheEntry& rhs) const
    {
        return this->hash > rhs.hash;
    }
    bool operator<(const TileCacheEntry& rhs) const
    {
        return this->hash < rhs.hash;
    }
    bool operator==(const TileCacheEntry& rhs) const
    {
        return this->hash == rhs.hash;
    }
    bool operator!=(const TileCacheEntry& rhs) const
    {
        return this->hash != rhs.hash;
    }
};

static const TileCacheEntry InvalidEntry = TileCacheEntry{ UINT64_MAX };

class TextureTileCache
{
public:
    /// constructor
    TextureTileCache();
    /// destructor
    ~TextureTileCache();
    
    /// setup cache
    void Setup(uint tileSize, uint textureSize, uint invalidValue);
    /// get tile, if invalid coord, gets the last used, otherwise, bumps the use of it
    bool Cache(TileCacheEntry entry, Math::uint2& coords);
    /// clear cache
    void Clear();
private:
    struct Node
    {
        TileCacheEntry entry;
        Math::uint2 offset;
        uint id;
        Node* prev;
        Node* next;
    };

    /// insert at beginning of list
    void InsertBeginning(Node* node);
    /// insert before another node
    void InsertBefore(Node* node, Node* newNode);
    /// detach node and fix tail & head
    void Remove(Node* node);

    Ids::IdPool nodePool;
    Util::FixedArray<Node> nodes;

    Node* head;
    Node* tail;
    Util::Dictionary<TileCacheEntry, Node*> lookup;

    uint tiles;
    uint tileSize;
    uint invalidValue;
};

//------------------------------------------------------------------------------
/**
*/
inline 
TextureTileCache::TextureTileCache()
    : head(nullptr)
    , tail(nullptr)
    , tiles(0)
    , tileSize(0)
    , invalidValue(0)
{
}

//------------------------------------------------------------------------------
/**
*/
inline 
TextureTileCache::~TextureTileCache()
{
}

//------------------------------------------------------------------------------
/**
*/
inline void 
TextureTileCache::Setup(uint tileSize, uint textureSize, uint invalidValue)
{
    this->tiles = textureSize / tileSize;
    this->tileSize = tileSize;
    this->invalidValue = invalidValue;

    // keep all nodes linearly in memory!
    this->nodes.Resize(this->tiles * this->tiles);
    this->nodePool.Reserve(this->tiles * this->tiles);

    // setup storage
    for (uint x = 0; x < this->tiles; x++)
    {
        for (uint y = 0; y < this->tiles; y++)
        {
            // calculate node index 2D->1D
            uint index = x + y * this->tiles;

            // set the data, this will be static
            Node* node = &this->nodes[index];
            node->offset = Math::uint2{ x * tileSize, y * tileSize };
            node->entry = InvalidEntry;
            node->prev = nullptr;
            node->next = nullptr;
            node->id = -1;
        }
    }

    this->head = nullptr;
    this->tail = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
TextureTileCache::Cache(TileCacheEntry entry, Math::uint2& coords)
{
    n_assert(entry != InvalidEntry);
    IndexT index = this->lookup.FindIndex(entry);
    if (index == InvalidIndex)
    {
        // if we have no free ids, free up tail
        if (this->nodePool.GetNumFree() == 0)
        {
            this->nodePool.Dealloc(this->tail->id);
            n_assert(this->tail->entry != InvalidEntry);

            // also erase from lookup
            this->lookup.Erase(this->tail->entry);
            this->tail->id = -1;
            this->tail->entry = InvalidEntry;
            this->Remove(this->tail);
        }

        // allocate new node
        uint nodeId = this->nodePool.Alloc();
        Node* node = &this->nodes[nodeId];

        // make most recent
        this->InsertBeginning(node);
        node->entry = entry;
        node->id = nodeId;
        this->lookup.Add(entry, node);

        // update entry
        coords = node->offset;
        return true;
    }
    else
    {
        Node* node = this->lookup.ValueAtIndex(index);
        this->Remove(node);
        this->InsertBeginning(node);
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void 
TextureTileCache::Clear()
{
    Node* node = this->head->next;
    while (node != nullptr)
    {
        Node* next = node->next;
        this->nodePool.Dealloc(node->id);
        this->Remove(node);
        node = next;
    }
    this->head = nullptr;
    this->tail = nullptr;
    this->lookup.Clear();
}

//------------------------------------------------------------------------------
/**
*/
inline void 
TextureTileCache::InsertBeginning(Node* node)
{
    if (this->head == nullptr)
    {
        this->head = node;
        this->tail = node;
        node->prev = nullptr;
        node->next = nullptr;
    }
    else
        this->InsertBefore(this->head, node);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
TextureTileCache::InsertBefore(Node* node, Node* newNode)
{
    newNode->next = node;
    if (node->prev == nullptr)
    {
        newNode->prev = nullptr;
        this->head = newNode;
    }
    else
    {
        newNode->prev = node->prev;
        node->prev->next = newNode;
    }
    node->prev = newNode;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
TextureTileCache::Remove(Node* node)
{
    // reassign neighbors
    if (node->prev == nullptr)
        this->head = node->next;
    else
        node->prev->next = node->next;

    if (node->next == nullptr)
        this->tail = node->prev;
    else
        node->next->prev = node->prev;

    node->next = nullptr;
    node->prev = nullptr;
}

} // namespace Terrain
