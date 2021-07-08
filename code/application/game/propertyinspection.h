#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::PropertyInspection
    
    Property serialization functions.

    Implements various serialization functions for different types of properties
    
    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/delegate.h"
#include "category.h"
#include "util/stringatom.h"
#include "game/entity.h"

namespace Game
{

class PropertyInspection
{
public:
    using DrawFunc = void(*)(PropertyId, void*, bool*);
    
    static PropertyInspection* Instance();
    static void Destroy();

    static void Register(PropertyId pid, DrawFunc);

    static void DrawInspector(PropertyId pid, void* data, bool* commit);

private:
    PropertyInspection();
    ~PropertyInspection();

    static PropertyInspection* Singleton;

    Util::Array<DrawFunc> inspectors;
};

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
void
PropertyDrawFuncT(PropertyId, void*, bool*)
{
    return;
}

template<> void PropertyDrawFuncT<Game::Entity>(PropertyId, void*, bool*);
template<> void PropertyDrawFuncT<int>(PropertyId, void*, bool*);
template<> void PropertyDrawFuncT<uint>(PropertyId, void*, bool*);
template<> void PropertyDrawFuncT<float>(PropertyId, void*, bool*);
template<> void PropertyDrawFuncT<Util::StringAtom>(PropertyId, void*, bool*);
template<> void PropertyDrawFuncT<Math::mat4>(PropertyId, void*, bool*);

} // namespace Game
