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
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "imgui.h"

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
    if constexpr (TYPE::Traits::num_fields > 0)
    {
        for (size_t i = 0; i < TYPE::Traits::num_fields; i++)
        {
            ImGui::Text(TYPE::Traits::field_names[i]);
        }
    }
    return;
}

template<> void ComponentDrawFuncT<Game::Entity>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<int>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<uint>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<float>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Util::StringAtom>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Math::mat4>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Math::vec3>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Game::Position>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Game::Orientation>(ComponentId, void*, bool*);
template<> void ComponentDrawFuncT<Game::Scale>(ComponentId, void*, bool*);

} // namespace Game
