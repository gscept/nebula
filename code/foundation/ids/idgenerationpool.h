#pragma once
//------------------------------------------------------------------------------
/**
    @class Ids::IdGenerationPool

    Provides a system for creating array friendly id numbers with reuse and 
    generations. Loosely inspired by bitsquid's blog.

    It provides a way to recycle Ids using a separate list of generation numbers,
    New ids are generated incrementally, but once returned, their generation
    is increased, such that the next time the id is reused, it can be checked for
    validity against any dangling instances of the previous generations. 
    
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/

#include "foundation/stdneb.h"
#include "util/array.h"
#include "id.h"
#include "util/list.h"
#include "util/queue.h"

const uint32_t ID_BITS = 24;
const uint32_t GENERATION_BITS = 8;
const uint32_t ID_MASK = (1<<ID_BITS) - 1;
const uint32_t GENERATION_MASK = (1<<GENERATION_BITS) - 1;

#if GENERATION_BITS <= 8
typedef uint8_t generation_t;
#elif GENERATION_BITS <= 16
typedef uint16_t generation_t;
#else 
typedef uint32_t generation_t;
#endif
#if ID_BITS + GENERATION_BITS > 32
#error bits in game id generation exceeds 32
#endif

//------------------------------------------------------------------------------
namespace Ids
{
class IdGenerationPool
{
public:
    /// constructor
    IdGenerationPool();
    /// destructor
    ~IdGenerationPool();

    /// allocate a new id, returns whether or not the id was reused or new
    bool Allocate(Id32& id);
    /// remove an id
    void Deallocate(Id32 id);
    /// check if valid
    bool IsValid(Id32 id) const;

private:
    /// array containing generation value for every index
    Util::Array<generation_t> generations;

    /// stores freed indices
    Util::Queue<Id32> freeIds;
    SizeT freeIdsSize;
};

//------------------------------------------------------------------------------
/**
*/
static Id24 
Index(const Id32 id)
{
    return id & ID_MASK;
}

//------------------------------------------------------------------------------
/**
*/
static generation_t
Generation(const Id32 id)
{
    return (id >> ID_BITS) & GENERATION_MASK;
}

//------------------------------------------------------------------------------
/**
*/
static Id32
CreateId(const Id24 index, generation_t generation)
{
    Id32 id;
    id = (generation << ID_BITS) | index;
    n_assert2(index < (1 << ID_BITS), "index overflow");
    return id;
}

} // namespace Ids
//------------------------------------------------------------------------------