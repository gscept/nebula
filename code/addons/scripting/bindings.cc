//------------------------------------------------------------------------------
//  bindings.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "util/string.h"
#include "math/matrix44.h"
#include "math/float4.h"
#include "math/vector.h"
#include "math/point.h"
#include "util/array.h"
#include "game/entity.h"
#include "pybind11/embed.h"
#include "scripting/bindings.h"
#include "pybind11/numpy.h"
#include "basegamefeature/managers/componentmanager.h"
#include "basegamefeature/components/transformcomponent.h"

namespace py = pybind11;


PYBIND11_EMBEDDED_MODULE(game, m)
{
    m.def("create_entity",
        []()->Game::Entity
        {
            return Game::EntityManager::Instance()->NewEntity();
        }
    );

    m.def("destroy_entity",
        [](Game::Entity e)
        {
            return Game::EntityManager::Instance()->DeleteEntity(e);
        }
    );

    py::class_<Game::Entity>(m, "entity")
        .def(py::init([](Ids::Id32 id) 
        {
            Game::Entity e = id; return e;
        }))
        .def("is_valid", [](Game::Entity id){
            return Game::EntityManager::Instance()->IsAlive(id);
        })
        .def("register_component", [](Game::Entity id, Util::String const& componentName)
        {
            Game::ComponentInterface* component = Game::ComponentManager::Instance()->GetComponentByName(componentName);
            if (component != nullptr)
            {
                component->RegisterEntity(id);
            }
        })
        .def("deregister_component", [](Game::Entity id, Util::String const& componentName)
        {
            Game::ComponentInterface* component = Game::ComponentManager::Instance()->GetComponentByName(componentName);
            if (component != nullptr)
            {
                component->DeregisterEntity(id);
            }
        })
        .def("has_component", [](Game::Entity id, Util::String const& componentName) -> bool
        {
            Game::ComponentInterface* component = Game::ComponentManager::Instance()->GetComponentByName(componentName);
            if (component != nullptr)
            {
                return (component->GetInstance(id) != InvalidIndex);
            }
            return false;
        })               
        .def_property("world_transform",
            [](const Game::Entity &id) -> Math::matrix44
            {
                return Game::TransformComponent::GetWorldTransform(id);
            },
            [](const Game::Entity &id, const Math::matrix44& mat)
            {   
                Game::TransformComponent::SetWorldTransform(id, mat);
            }
        )
        .def_property("local_transform",
            [](const Game::Entity &id) -> Math::matrix44
            {
                return Game::TransformComponent::GetLocalTransform(id);
            },
            [](const Game::Entity &id, const Math::matrix44& mat)
            {
                Game::TransformComponent::SetLocalTransform(id, mat);
            }            
        );
}
