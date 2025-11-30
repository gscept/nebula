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
namespace Terrain
{


struct IndirectionEntry
{
    uint mip : 4;
    uint physicalOffsetX : 14;
    uint physicalOffsetY : 14;
};
static_assert(alignof(IndirectionEntry) == 4);
static_assert(sizeof(IndirectionEntry) == 4);

struct TileCacheEntry
{
    struct Entry
    {
        uint64_t tiles : 10;                // must hold at least SubTextureMaxTiles from TerrainContext.h
        uint64_t tileX : 11;                // must hold at least SubTextureMaxTiles
        uint64_t tileY : 11;                // same as above
        uint64_t subTextureIndex : 32;      // the index of the subtexture
    };
    union
    {
        Entry entry;
        uint64_t hash;
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

static const TileCacheEntry InvalidTileCacheEntry = TileCacheEntry{ 0x3FF, 0x7FF, 0x7FF, 0xFFFFFFFF };

class TextureTileCache
{
public:

    struct CacheResult
    {
        Math::uint2 cached;
        bool didCache;
        TileCacheEntry evicted;
    };
    /// constructor
    TextureTileCache();
    /// destructor
    ~TextureTileCache();
    
    /// setup cache
    void Setup(uint tileSize, uint textureSize);
    /// get tile, if invalid coord, gets the last used, otherwise, bumps the use of it
    CacheResult Cache(TileCacheEntry entry);
    /// clear cache
    void Clear();
    /// Reset the cache without freeing memory
    void Reset();
private:
    struct Node
    {
        TileCacheEntry entry;
        Math::uint2 offset;
        Node* prev;
        Node* next;
    };

    /// insert at beginning of list
    void InsertBeginning(Node* node);
    /// insert before another node
    void InsertBefore(Node* node, Node* newNode);
    /// detach node and fix tail & head
    void Remove(Node* node);

    Util::FixedArray<Node> nodes;

    Node* head;
    Node* tail;
    Util::Dictionary<TileCacheEntry, Node*> lookup;

    uint tiles;
    uint tileSize;
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
TextureTileCache::Setup(uint tileSize, uint textureSize)
{
    this->head = this->tail = nullptr;
    this->tiles = textureSize / tileSize;
    this->tileSize = tileSize;

    // keep all nodes linearly in memory!
    this->nodes.Resize(this->tiles * this->tiles);

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
            node->entry = InvalidTileCacheEntry;
            node->prev = nullptr;
            node->next = nullptr;

            // Add to linked list
            this->InsertBeginning(node);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
inline TextureTileCache::CacheResult
TextureTileCache::Cache(TileCacheEntry entry)
{
    n_assert(entry != InvalidTileCacheEntry);

    CacheResult result;
    result.didCache = false;
    result.evicted = InvalidTileCacheEntry;

    IndexT index = this->lookup.FindIndex(entry);
    if (index == InvalidIndex)
    {
        // Grab the tail
        Node* node = this->tail;
        this->Remove(node);
        if (node->entry != InvalidTileCacheEntry)
        {
            this->lookup.Erase(node->entry);
            result.evicted = node->entry;
        }

        // make most recent
        this->InsertBeginning(node);
        node->entry = entry;
        this->lookup.Add(entry, node);

        // update entry
        result.cached = node->offset;
        result.didCache = true;
    }
    else
    {
        Node* node = this->lookup.ValueAtIndex(index);
        result.cached = node->offset;
        this->Remove(node);
        this->InsertBeginning(node);
    }
    return result;
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
        this->Remove(node);
        node = next;
    }
    this->head = nullptr;
    this->tail = nullptr;

    // setup storage
    for (uint x = 0; x < this->tiles; x++)
    {
        for (uint y = 0; y < this->tiles; y++)
        {
            // calculate node index 2D->1D
            uint index = x + y * this->tiles;

            // set the data, this will be static
            Node* node = &this->nodes[index];
            node->offset = Math::uint2{ x * this->tileSize, y * this->tileSize };
            node->entry = InvalidTileCacheEntry;
            node->prev = nullptr;
            node->next = nullptr;

            // Add to linked list
            this->InsertBeginning(node);
        }
    }

    this->nodes.Clear();
    this->lookup.Clear();
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureTileCache::Reset()
{
    // setup storage
    this->head = nullptr;
    this->tail = nullptr;
    this->lookup.Clear();
    for (uint x = 0; x < this->tiles; x++)
    {
        for (uint y = 0; y < this->tiles; y++)
        {
            // calculate node index 2D->1D
            uint index = x + y * this->tiles;

            // set the data, this will be static
            Node* node = &this->nodes[index];
            node->offset = Math::uint2{ x * tileSize, y * tileSize };
            node->entry = InvalidTileCacheEntry;
            node->prev = nullptr;
            node->next = nullptr;

            // Add to linked list
            this->InsertBeginning(node);
        }
    }
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
