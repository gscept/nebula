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
    py::class_<Game::Entity>(m, "entity")
        .def(py::init([](Ids::Id32 id) 
        {
            Game::Entity e = id; return e;
        }))
        .def("has_component", [](Game::Entity id, uint32_t fourcc) -> bool
        {
            Util::FourCC fcc(fourcc);
            if (!Core::Factory::Instance()->ClassExists(fcc))
            {
                throw std::exception("unknown class id");
                return false;
            }
            auto comp = Game::ComponentManager::Instance()->GetComponentByFourCC(fcc);
            return (comp->GetInstance(id) != InvalidIndex);
        })               
        .def_property("worldtransform",
            [](const Game::Entity &id) -> Math::matrix44
            {
                return Game::TransformComponent::GetWorldTransform(id);
            },
            [](const Game::Entity &id, const Math::matrix44& mat)
            {   
                Game::TransformComponent::SetWorldTransform(id, mat);
            }
        )
        .def_property("localtransform",
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
