#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::Id

    Provides a system for creating array friendly id numbers with reuse and 
    generations. Loosely inspired by bitsquid's blog
    
    (C) 2017 Individual contributors, see AUTHORS file
*/

#include "stdneb.h"
#include "util/array.h"
#include "util/queue.h"

const uint32_t GAME_ID_BITS = 24;
const uint32_t GAME_GEN_BITS = 8;
const uint32_t GAME_ID_MASK = (1<<GAME_ID_BITS) - 1;
const uint32_t GAME_GEN_MASK = (1<<GAME_GEN_BITS) - 1;

#if GAME_GEN_BITS <= 8
typedef uint8_t generation_t;
#elif GAME_GEN_BITS <= 16
typedef uint16_t generation_t;
#else 
typedef uint32_t generation_t;
#endif
#if GAME_ID_BITS + GAME_GEN_BITS > 32
#error bits in game id generation exceeds 32
#endif

//------------------------------------------------------------------------------
namespace Game
{

//------------------------------------------------------------------------------
struct Id
{    
    /// index part of the id
    uint32_t Index() const;
    /// generation 
    generation_t Generation() const;
    /// create an id from idx and generation
    static Id Create(uint32_t idx, generation_t gen);
    /// actual id
    uint32_t id;    
};

class IdSystem
{
    public:
    ///
    IdSystem();
    ///
    ~IdSystem();

    /// allocate a new id
    Id Allocate();
    /// remove an id
    void Deallocate(Id id);
    /// check if valid
    bool IsValid(Id id);

    private:
    /// array containing generation value for every index
    Util::Array<generation_t> generations;

    /// stores freed indices
    Util::Queue<uint32_t> freeIds;
};

//------------------------------------------------------------------------------
/**    
*/
inline uint32_t
Id::Index() const
{
    return (this->id & GAME_ID_MASK);
}

//------------------------------------------------------------------------------
/** 
*/
inline generation_t
Id::Generation() const
{
    return (this->id >> GAME_ID_BITS) & GAME_GEN_MASK;
}

//------------------------------------------------------------------------------
/** 
*/
inline Id
Id::Create(uint32_t idx, generation_t gen)
{
    Id id;
    id.id = (gen << GAME_ID_BITS) | idx;
    n_assert2(idx < (1<<GAME_ID_BITS), "index overflow");
    return id;
}
} // namespace Game
//------------------------------------------------------------------------------