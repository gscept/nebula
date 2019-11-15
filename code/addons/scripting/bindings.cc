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
#include "pybind11/operators.h"
#include "basegamefeature/managers/componentmanager.h"
#include "basegamefeature/components/transformcomponent.h"
#include "attr/attrid.h"

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


PYBIND11_EMBEDDED_MODULE(attr, m)
{
    py::class_<Attr::AttrId>(m, "AttrId")
        .def(py::init([](Util::FourCC fourcc) 
        {
            if (!Attr::AttrId::IsValidFourCC(fourcc))
            {
                return Attr::AttrId();
            }
            
            return Attr::AttrId(fourcc);
        }))
        .def(py::init([](Util::String const& name) 
        {
            if (!Attr::AttrId::IsValidName(name))
            {
                return Attr::AttrId();
            }
            
            return Attr::AttrId(name);
        }))
        .def(py::self == py::self)
		.def(py::self != py::self)
        .def(py::self >= py::self)
		.def(py::self <= py::self)
        .def(py::self < py::self)
		.def(py::self > py::self)
        .def("is_valid", &Attr::AttrId::IsValid, "Check if this attribute id is valid")
        .def("is_dynamic", &Attr::AttrId::IsDynamic, "Check if this attribute was defined at runtime")
        .def_property_readonly("name", &Attr::AttrId::GetName, "Name of this attribute definition")
        .def_property_readonly("type_name", &Attr::AttrId::GetTypeName, "A string representing the type name in C++")
        .def_property_readonly("fourcc", &Attr::AttrId::GetFourCC, "The unique identifier for this attribute definition")
        .def_property_readonly("access_mode", &Attr::AttrId::GetAccessMode, "Specifies how this attribute definition can be accessed")
        .def_property_readonly("value_type", &Attr::AttrId::GetValueType, "A enumerated type representing type value type of the attribute definition")
        .def_property_readonly("default_value", &Attr::AttrId::GetDefaultValue, "The default value of this attribute definition");
    
    py::enum_<Attr::AccessMode>(m, "AccessMode")
        .value("READ_ONLY", Attr::AccessMode::ReadOnly)
        .value("READ_WRITE", Attr::AccessMode::ReadWrite)
        .export_values();

    py::enum_<Attr::ValueType>(m, "ValueType")
        .value("VOID", Attr::ValueType::VoidType)
        .value("BYTE", Attr::ValueType::ByteType)
        .value("SHORT", Attr::ValueType::ShortType)
        .value("USHORT", Attr::ValueType::UShortType)
        .value("INT", Attr::ValueType::IntType)
        .value("UINT", Attr::ValueType::UIntType)
        .value("INT64", Attr::ValueType::Int64Type)
        .value("UINT64", Attr::ValueType::UInt64Type)
        .value("FLOAT", Attr::ValueType::FloatType)
        .value("DOUBLE", Attr::ValueType::DoubleType)
        .value("BOOL", Attr::ValueType::BoolType)
        .value("FLOAT2", Attr::ValueType::Float2Type)
        .value("FLOAT4", Attr::ValueType::Float4Type)
        .value("QUATERNION", Attr::ValueType::QuaternionType)
        .value("STRING", Attr::ValueType::StringType)
        .value("MATRIX44", Attr::ValueType::Matrix44Type)
        .value("TRANSFORM44", Attr::ValueType::Transform44Type)
        .value("BLOB", Attr::ValueType::BlobType)
        .value("GUID", Attr::ValueType::GuidType)
        .value("VOID_PTR", Attr::ValueType::VoidPtrType)
        .value("INT_ARRAY", Attr::ValueType::IntArrayType)
        .value("FLOAT_ARRAY", Attr::ValueType::FloatArrayType)
        .value("BOOL_ARRAY", Attr::ValueType::BoolArrayType)
        .value("FLOAT2_ARRAY", Attr::ValueType::Float2ArrayType)
        .value("FLOAT4_ARRAY", Attr::ValueType::Float4ArrayType)
        .value("STRING_ARRAY", Attr::ValueType::StringArrayType)
        .value("MATRIX44_ARRAY", Attr::ValueType::Matrix44ArrayType)
        .value("BLOB_ARRAY", Attr::ValueType::BlobArrayType)
        .value("GUID_ARRAY", Attr::ValueType::GuidArrayType)
        .value("ENTITY", Attr::ValueType::EntityType)
        .export_values();
}
