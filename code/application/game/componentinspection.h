#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::ComponentInspection
    
    Component inspection functions.

    Implements various inspection functions for different types of components
    
    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/delegate.h"
#include "util/stringatom.h"
#include "game/entity.h"
#include "game/componentid.h"

namespace Game
{

class ComponentInspection
{
public:
    using DrawFunc = void(*)(Game::ComponentId, void*, bool*);
    
    static ComponentInspection* Instance();
    static void Destroy();

    static void Register(ComponentId component, DrawFunc);

    static void DrawInspector(ComponentId component, void* data, bool* commit);

private:
    ComponentInspection();
    ~ComponentInspection();

    static ComponentInspection* Singleton;

    Util::Array<DrawFunc> inspectors;
};

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
void
ComponentDrawFuncT(ComponentId, void*, bool*)
{
    return;
}

template<> void ComponentDrawFuncT<Game::Entity>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<int>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<uint>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<float>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Util::StringAtom>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Math::mat4>(ComponentId, void*, bool*);

} // namespace Game
