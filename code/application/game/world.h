#pragma once
//------------------------------------------------------------------------------
/**
    @file world.h

    @copyright
    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"

namespace MemDb { class Database; }

namespace Game
{

typedef uint32_t OpBuffer;

struct WorldCreateInfo
{
    uint32_t hash;
};

class World;

World* AllocateWorld(WorldCreateInfo const& info);
void DeallocateWorld(World* world);

// These functions are called from game server
void WorldBeginFrame(World*);
void WorldSimFrame(World*);
void WorldEndFrame(World*);
void WorldOnLoad(World*);
void WorldOnSave(World*);

void WorldPrefilterProcessors(World*);
/// check if the world processor filters has been properly cached
bool WorldPrefiltered(World*);

void WorldManageEntities(World*); // todo: better name

void WorldReset(World*);

void WorldRenderDebug(World*);

OpBuffer WorldGetScratchOpBuffer(World*);

Ptr<MemDb::Database> GetWorldDatabase(World*);




} // namespace Game
