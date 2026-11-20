//------------------------------------------------------------------------------
//  @file levelextension.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "game/levelextension.h"

namespace Game
{

LevelExtensionRegistry* LevelExtensionRegistry::Singleton = nullptr;

//------------------------------------------------------------------------------
/**
    The registry's constructor is called by the Instance() method, and nobody else.
*/
LevelExtensionRegistry*
LevelExtensionRegistry::Instance()
{
    if (nullptr == Singleton)
    {
        Singleton = new LevelExtensionRegistry;
        n_assert(nullptr != Singleton);
    }
    return Singleton;
}

//------------------------------------------------------------------------------
/**
    This static method is used to destroy the registry object and should be called right before the
    main function exits. It will make sure that no accidental memory leaks are reported by the debug
    heap.
*/
void
LevelExtensionRegistry::Destroy()
{
    if (nullptr != Singleton)
    {
        delete Singleton;
        Singleton = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
*/
LevelExtensionRegistry::LevelExtensionRegistry()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
LevelExtensionRegistry::~LevelExtensionRegistry()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    An extension is registered by name, and the name is what a file carries, so two extensions cannot
    share one. The order they are registered in is the order the formats write them in, which is what
    keeps a saved level from moving its extension nodes between builds.
*/
void
LevelExtensionRegistry::Register(const LevelExtension& extension)
{
    LevelExtensionRegistry* reg = Instance();

    if (reg->byName.Contains(extension.name))
    {
        n_error("Tried to register level extension '%s': Cannot register two extensions with same name!",
            extension.name.Value());
        return;
    }

    reg->byName.Add(extension.name, (IndexT)reg->extensions.Size());
    reg->extensions.Append(extension);
}

//------------------------------------------------------------------------------
/**
*/
const LevelExtension*
LevelExtensionRegistry::GetExtension(Util::StringAtom name)
{
    LevelExtensionRegistry* reg = Instance();

    IndexT index = reg->byName.FindIndex(name);
    if (index == InvalidIndex)
        return nullptr;

    return &reg->extensions[index];
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<LevelExtension>&
LevelExtensionRegistry::GetAllExtensions()
{
    return Instance()->extensions;
}

} // namespace Game
//------------------------------------------------------------------------------
