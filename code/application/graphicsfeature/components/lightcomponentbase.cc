// NIDL #version:7#
//------------------------------------------------------------------------------
//  lightcomponentbase.cc
//  (C) Individual contributors, see AUTHORS file
//
//  MACHINE GENERATED, DON'T EDIT!
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "lightcomponentbase.h"
//------------------------------------------------------------------------------
namespace Attr
{
    DefineStringWithDefault(DebugName, 'tStr', Attr::ReadWrite, Util::String("tjene"));
    DefineFloatWithDefault(Range, 'LRAD', Attr::ReadOnly, float(10.0f));
    DefineFloat4WithDefault(Color, 'LCLR', Attr::ReadWrite, Math::float4(1.0f, 0.88f, 0.65f, 1.0f));
    DefineBoolWithDefault(CastShadows, 'SHDW', Attr::ReadWrite, bool(true));
    DefineFloatWithDefault(Angle, 'LNGL', Attr::ReadWrite, float(30.0f));
    DefineFloat4WithDefault(Direction, 'LDIR', Attr::ReadWrite, Math::float4(0.0f, 0.0f, 1.0f, 0.0f));
} // namespace Attr
//------------------------------------------------------------------------------
namespace GraphicsFeature
{

__ImplementClass(GraphicsFeature::PointLightComponentBase, 'plcp', Core::RefCounted)


//------------------------------------------------------------------------------
/**
*/
PointLightComponentBase::PointLightComponentBase()
{
    this->attributeIds.SetSize(5);
    this->attributeIds[0] = Attr::Owner;
    this->attributeIds[1] = Attr::Range;
    this->attributeIds[2] = Attr::Color;
    this->attributeIds[3] = Attr::CastShadows;
    this->attributeIds[4] = Attr::DebugName;
}


//------------------------------------------------------------------------------
/**
*/
PointLightComponentBase::~PointLightComponentBase()
{
    // empty
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::RegisterEntity(const Game::Entity& entity)
{
    auto instance = this->data.RegisterEntity(entity);
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::DeregisterEntity(const Game::Entity& entity)
{
    uint32_t index = this->data.GetInstance(entity);
    if (index != InvalidIndex)
    {
        this->data.DeregisterEntity(entity);
        return;
    }
}


//------------------------------------------------------------------------------
/**
    @todo	if needed: deregister deletion callbacks
*/
void
PointLightComponentBase::DeregisterAllDead()
{
    this->data.DeregisterAllInactive();
}


//------------------------------------------------------------------------------
/**
    @todo	if needed: deregister deletion callbacks
*/
void
PointLightComponentBase::CleanData()
{
    this->data.Clean();
}


//------------------------------------------------------------------------------
/**
    @todo	if needed: deregister deletion callbacks
*/
void
PointLightComponentBase::DestroyAll()
{
    this->data.DestroyAll();
}


//------------------------------------------------------------------------------
/**
*/
bool
PointLightComponentBase::IsRegistered(const Game::Entity& entity) const
{
    return this->data.GetInstance(entity) != InvalidIndex;
}


//------------------------------------------------------------------------------
/**
*/
uint32_t
PointLightComponentBase::GetInstance(const Game::Entity& entity) const
{
    return this->data.GetInstance(entity);
}


//------------------------------------------------------------------------------
/**
*/
Game::Entity
PointLightComponentBase::GetOwner(const uint32_t& instance) const
{
    return this->data.GetOwner(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::SetOwner(const uint32_t & i, const Game::Entity & entity)
{
    this->data.SetOwner(i, entity);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
PointLightComponentBase::Optimize()
{
    return this->data.Optimize();;
}


//------------------------------------------------------------------------------
/**
*/
Util::Variant
PointLightComponentBase::GetAttributeValue(uint32_t instance, IndexT attributeIndex) const
{
    switch (attributeIndex)
    {
        case 0: return Util::Variant(this->data.data.Get<0>(instance).id);
        case 1: return Util::Variant(this->data.data.Get<1>(instance));
        case 2: return Util::Variant(this->data.data.Get<2>(instance));
        case 3: return Util::Variant(this->data.data.Get<3>(instance));
        case 4: return Util::Variant(this->data.data.Get<4>(instance));
        default:
            n_assert2(false, "Component doesn't contain this attribute!");
            return Util::Variant();
    }
}


//------------------------------------------------------------------------------
/**
*/
Util::Variant
PointLightComponentBase::GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const
{
    if (attributeId == Attr::Owner) return Util::Variant(this->data.data.Get<0>(instance).id);
    else if (attributeId == Attr::Range) return Util::Variant(this->data.data.Get<1>(instance));
    else if (attributeId == Attr::Color) return Util::Variant(this->data.data.Get<2>(instance));
    else if (attributeId == Attr::CastShadows) return Util::Variant(this->data.data.Get<3>(instance));
    else if (attributeId == Attr::DebugName) return Util::Variant(this->data.data.Get<4>(instance));
    n_assert2(false, "Component does not contain this attribute!");
    return Util::Variant();
}

//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::SetAttributeValue(uint32_t instance, IndexT index, Util::Variant value)
{
    switch (index)
    {
        case 2: this->data.data.Get<2>(instance) = value.GetFloat4();
        case 3: this->data.data.Get<3>(instance) = value.GetBool();
        case 4: this->data.data.Get<4>(instance) = value.GetString();
    }
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::SetAttributeValue(uint32_t instance, Attr::AttrId attributeId, Util::Variant value)
{
    if (attributeId == Attr::Color) this->data.data.Get<2>(instance) = value.GetFloat4();
    else if (attributeId == Attr::CastShadows) this->data.data.Get<3>(instance) = value.GetBool();
    else if (attributeId == Attr::DebugName) this->data.data.Get<4>(instance) = value.GetString();
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::Serialize(const Ptr<IO::BinaryWriter>& writer) const
{
    this->data.Serialize(writer);
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances)
{
    this->data.Deserialize(reader, offset, numInstances);
}

//------------------------------------------------------------------------------
/**
*/
const float&
PointLightComponentBase::GetAttrRange(const uint32_t& instance)
{
    return this->data.data.Get<1>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::SetAttrRange(const uint32_t& instance, const float& value)
{
    this->data.data.Get<1>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const Math::float4&
PointLightComponentBase::GetAttrColor(const uint32_t& instance)
{
    return this->data.data.Get<2>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::SetAttrColor(const uint32_t& instance, const Math::float4& value)
{
    this->data.data.Get<2>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const bool&
PointLightComponentBase::GetAttrCastShadows(const uint32_t& instance)
{
    return this->data.data.Get<3>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::SetAttrCastShadows(const uint32_t& instance, const bool& value)
{
    this->data.data.Get<3>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const Util::String&
PointLightComponentBase::GetAttrDebugName(const uint32_t& instance)
{
    return this->data.data.Get<4>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::SetAttrDebugName(const uint32_t& instance, const Util::String& value)
{
    this->data.data.Get<4>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
uint32_t
PointLightComponentBase::GetNumInstances() const
{
    return this->data.Size();
}

//------------------------------------------------------------------------------
/**
    @todo	we should reserve per array here.
*/
void
PointLightComponentBase::AllocInstances(uint num)
{
    for (size_t i = 0; i < num; i++)
    {
        this->data.data.Alloc();
    }
}


__ImplementClass(GraphicsFeature::SpotLightComponentBase, 'splc', Core::RefCounted)


//------------------------------------------------------------------------------
/**
*/
SpotLightComponentBase::SpotLightComponentBase()
{
    this->attributeIds.SetSize(6);
    this->attributeIds[0] = Attr::Owner;
    this->attributeIds[1] = Attr::Range;
    this->attributeIds[2] = Attr::Angle;
    this->attributeIds[3] = Attr::Direction;
    this->attributeIds[4] = Attr::Color;
    this->attributeIds[5] = Attr::CastShadows;
}


//------------------------------------------------------------------------------
/**
*/
SpotLightComponentBase::~SpotLightComponentBase()
{
    // empty
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::RegisterEntity(const Game::Entity& entity)
{
    auto instance = this->data.RegisterEntity(entity);
    Game::EntityManager::Instance()->RegisterDeletionCallback(entity, this);
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::DeregisterEntity(const Game::Entity& entity)
{
    uint32_t index = this->data.GetInstance(entity);
    if (index != InvalidIndex)
    {
        this->data.DeregisterEntityImmediate(entity);
        Game::EntityManager::Instance()->DeregisterDeletionCallback(entity, this);
        return;
    }
}


//------------------------------------------------------------------------------
/**
    @todo	if needed: deregister deletion callbacks
*/
void
SpotLightComponentBase::DeregisterAllDead()
{
    this->data.DeregisterAllInactive();
}


//------------------------------------------------------------------------------
/**
    @todo	if needed: deregister deletion callbacks
*/
void
SpotLightComponentBase::CleanData()
{
    this->data.Clean();
}


//------------------------------------------------------------------------------
/**
    @todo	if needed: deregister deletion callbacks
*/
void
SpotLightComponentBase::DestroyAll()
{
    this->data.DestroyAll();
}


//------------------------------------------------------------------------------
/**
*/
bool
SpotLightComponentBase::IsRegistered(const Game::Entity& entity) const
{
    return this->data.GetInstance(entity) != InvalidIndex;
}


//------------------------------------------------------------------------------
/**
*/
uint32_t
SpotLightComponentBase::GetInstance(const Game::Entity& entity) const
{
    return this->data.GetInstance(entity);
}


//------------------------------------------------------------------------------
/**
*/
Game::Entity
SpotLightComponentBase::GetOwner(const uint32_t& instance) const
{
    return this->data.GetOwner(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::SetOwner(const uint32_t & i, const Game::Entity & entity)
{
    this->data.SetOwner(i, entity);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
SpotLightComponentBase::Optimize()
{
    return 0;
}


//------------------------------------------------------------------------------
/**
*/
Util::Variant
SpotLightComponentBase::GetAttributeValue(uint32_t instance, IndexT attributeIndex) const
{
    switch (attributeIndex)
    {
        case 0: return Util::Variant(this->data.data.Get<0>(instance).id);
        case 1: return Util::Variant(this->data.data.Get<1>(instance));
        case 2: return Util::Variant(this->data.data.Get<2>(instance));
        case 3: return Util::Variant(this->data.data.Get<3>(instance));
        case 4: return Util::Variant(this->data.data.Get<4>(instance));
        case 5: return Util::Variant(this->data.data.Get<5>(instance));
        default:
            n_assert2(false, "Component doesn't contain this attribute!");
            return Util::Variant();
    }
}


//------------------------------------------------------------------------------
/**
*/
Util::Variant
SpotLightComponentBase::GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const
{
    if (attributeId == Attr::Owner) return Util::Variant(this->data.data.Get<0>(instance).id);
    else if (attributeId == Attr::Range) return Util::Variant(this->data.data.Get<1>(instance));
    else if (attributeId == Attr::Angle) return Util::Variant(this->data.data.Get<2>(instance));
    else if (attributeId == Attr::Direction) return Util::Variant(this->data.data.Get<3>(instance));
    else if (attributeId == Attr::Color) return Util::Variant(this->data.data.Get<4>(instance));
    else if (attributeId == Attr::CastShadows) return Util::Variant(this->data.data.Get<5>(instance));
    n_assert2(false, "Component does not contain this attribute!");
    return Util::Variant();
}

//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::SetAttributeValue(uint32_t instance, IndexT index, Util::Variant value)
{
    switch (index)
    {
        case 2: this->data.data.Get<2>(instance) = value.GetFloat();
        case 3: this->data.data.Get<3>(instance) = value.GetFloat4();
        case 4: this->data.data.Get<4>(instance) = value.GetFloat4();
        case 5: this->data.data.Get<5>(instance) = value.GetBool();
    }
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::SetAttributeValue(uint32_t instance, Attr::AttrId attributeId, Util::Variant value)
{
    if (attributeId == Attr::Angle) this->data.data.Get<2>(instance) = value.GetFloat();
    else if (attributeId == Attr::Direction) this->data.data.Get<3>(instance) = value.GetFloat4();
    else if (attributeId == Attr::Color) this->data.data.Get<4>(instance) = value.GetFloat4();
    else if (attributeId == Attr::CastShadows) this->data.data.Get<5>(instance) = value.GetBool();
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::Serialize(const Ptr<IO::BinaryWriter>& writer) const
{
    this->data.Serialize(writer);
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances)
{
    this->data.Deserialize(reader, offset, numInstances);
}

//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::OnEntityDeleted(Game::Entity entity)
{
    uint32_t index = this->data.GetInstance(entity);
    if (index != InvalidIndex)
    {
        this->data.DeregisterEntityImmediate(entity);
        return;
    }
}


//------------------------------------------------------------------------------
/**
*/
const float&
SpotLightComponentBase::GetAttrRange(const uint32_t& instance)
{
    return this->data.data.Get<1>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::SetAttrRange(const uint32_t& instance, const float& value)
{
    this->data.data.Get<1>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const float&
SpotLightComponentBase::GetAttrAngle(const uint32_t& instance)
{
    return this->data.data.Get<2>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::SetAttrAngle(const uint32_t& instance, const float& value)
{
    this->data.data.Get<2>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const Math::float4&
SpotLightComponentBase::GetAttrDirection(const uint32_t& instance)
{
    return this->data.data.Get<3>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::SetAttrDirection(const uint32_t& instance, const Math::float4& value)
{
    this->data.data.Get<3>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const Math::float4&
SpotLightComponentBase::GetAttrColor(const uint32_t& instance)
{
    return this->data.data.Get<4>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::SetAttrColor(const uint32_t& instance, const Math::float4& value)
{
    this->data.data.Get<4>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const bool&
SpotLightComponentBase::GetAttrCastShadows(const uint32_t& instance)
{
    return this->data.data.Get<5>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::SetAttrCastShadows(const uint32_t& instance, const bool& value)
{
    this->data.data.Get<5>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
uint32_t
SpotLightComponentBase::GetNumInstances() const
{
    return this->data.Size();
}

//------------------------------------------------------------------------------
/**
    @todo	we should reserve per array here.
*/
void
SpotLightComponentBase::AllocInstances(uint num)
{
    for (size_t i = 0; i < num; i++)
    {
        this->data.data.Alloc();
    }
}


__ImplementClass(GraphicsFeature::DirectionalLightComponentBase, 'drlc', Core::RefCounted)


//------------------------------------------------------------------------------
/**
*/
DirectionalLightComponentBase::DirectionalLightComponentBase()
{
    this->attributeIds.SetSize(4);
    this->attributeIds[0] = Attr::Owner;
    this->attributeIds[1] = Attr::Direction;
    this->attributeIds[2] = Attr::Color;
    this->attributeIds[3] = Attr::CastShadows;
}


//------------------------------------------------------------------------------
/**
*/
DirectionalLightComponentBase::~DirectionalLightComponentBase()
{
    // empty
}


//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::RegisterEntity(const Game::Entity& entity)
{
    auto instance = this->data.RegisterEntity(entity);
    Game::EntityManager::Instance()->RegisterDeletionCallback(entity, this);
}


//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::DeregisterEntity(const Game::Entity& entity)
{
    uint32_t index = this->data.GetInstance(entity);
    if (index != InvalidIndex)
    {
        this->data.DeregisterEntityImmediate(entity);
        Game::EntityManager::Instance()->DeregisterDeletionCallback(entity, this);
        return;
    }
}


//------------------------------------------------------------------------------
/**
    @todo	if needed: deregister deletion callbacks
*/
void
DirectionalLightComponentBase::DeregisterAllDead()
{
    this->data.DeregisterAllInactive();
}


//------------------------------------------------------------------------------
/**
    @todo	if needed: deregister deletion callbacks
*/
void
DirectionalLightComponentBase::CleanData()
{
    this->data.Clean();
}


//------------------------------------------------------------------------------
/**
    @todo	if needed: deregister deletion callbacks
*/
void
DirectionalLightComponentBase::DestroyAll()
{
    this->data.DestroyAll();
}


//------------------------------------------------------------------------------
/**
*/
bool
DirectionalLightComponentBase::IsRegistered(const Game::Entity& entity) const
{
    return this->data.GetInstance(entity) != InvalidIndex;
}


//------------------------------------------------------------------------------
/**
*/
uint32_t
DirectionalLightComponentBase::GetInstance(const Game::Entity& entity) const
{
    return this->data.GetInstance(entity);
}


//------------------------------------------------------------------------------
/**
*/
Game::Entity
DirectionalLightComponentBase::GetOwner(const uint32_t& instance) const
{
    return this->data.GetOwner(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::SetOwner(const uint32_t & i, const Game::Entity & entity)
{
    this->data.SetOwner(i, entity);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
DirectionalLightComponentBase::Optimize()
{
    return 0;
}


//------------------------------------------------------------------------------
/**
*/
Util::Variant
DirectionalLightComponentBase::GetAttributeValue(uint32_t instance, IndexT attributeIndex) const
{
    switch (attributeIndex)
    {
        case 0: return Util::Variant(this->data.data.Get<0>(instance).id);
        case 1: return Util::Variant(this->data.data.Get<1>(instance));
        case 2: return Util::Variant(this->data.data.Get<2>(instance));
        case 3: return Util::Variant(this->data.data.Get<3>(instance));
        default:
            n_assert2(false, "Component doesn't contain this attribute!");
            return Util::Variant();
    }
}


//------------------------------------------------------------------------------
/**
*/
Util::Variant
DirectionalLightComponentBase::GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const
{
    if (attributeId == Attr::Owner) return Util::Variant(this->data.data.Get<0>(instance).id);
    else if (attributeId == Attr::Direction) return Util::Variant(this->data.data.Get<1>(instance));
    else if (attributeId == Attr::Color) return Util::Variant(this->data.data.Get<2>(instance));
    else if (attributeId == Attr::CastShadows) return Util::Variant(this->data.data.Get<3>(instance));
    n_assert2(false, "Component does not contain this attribute!");
    return Util::Variant();
}

//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::SetAttributeValue(uint32_t instance, IndexT index, Util::Variant value)
{
    switch (index)
    {
        case 1: this->data.data.Get<1>(instance) = value.GetFloat4();
        case 2: this->data.data.Get<2>(instance) = value.GetFloat4();
        case 3: this->data.data.Get<3>(instance) = value.GetBool();
    }
}


//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::SetAttributeValue(uint32_t instance, Attr::AttrId attributeId, Util::Variant value)
{
    if (attributeId == Attr::Direction) this->data.data.Get<1>(instance) = value.GetFloat4();
    else if (attributeId == Attr::Color) this->data.data.Get<2>(instance) = value.GetFloat4();
    else if (attributeId == Attr::CastShadows) this->data.data.Get<3>(instance) = value.GetBool();
}


//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::Serialize(const Ptr<IO::BinaryWriter>& writer) const
{
    this->data.Serialize(writer);
}


//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances)
{
    this->data.Deserialize(reader, offset, numInstances);
}

//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::OnEntityDeleted(Game::Entity entity)
{
    uint32_t index = this->data.GetInstance(entity);
    if (index != InvalidIndex)
    {
        this->data.DeregisterEntityImmediate(entity);
        return;
    }
}


//------------------------------------------------------------------------------
/**
*/
const Math::float4&
DirectionalLightComponentBase::GetAttrDirection(const uint32_t& instance)
{
    return this->data.data.Get<1>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::SetAttrDirection(const uint32_t& instance, const Math::float4& value)
{
    this->data.data.Get<1>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const Math::float4&
DirectionalLightComponentBase::GetAttrColor(const uint32_t& instance)
{
    return this->data.data.Get<2>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::SetAttrColor(const uint32_t& instance, const Math::float4& value)
{
    this->data.data.Get<2>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const bool&
DirectionalLightComponentBase::GetAttrCastShadows(const uint32_t& instance)
{
    return this->data.data.Get<3>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::SetAttrCastShadows(const uint32_t& instance, const bool& value)
{
    this->data.data.Get<3>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
uint32_t
DirectionalLightComponentBase::GetNumInstances() const
{
    return this->data.Size();
}

//------------------------------------------------------------------------------
/**
    @todo	we should reserve per array here.
*/
void
DirectionalLightComponentBase::AllocInstances(uint num)
{
    for (size_t i = 0; i < num; i++)
    {
        this->data.data.Alloc();
    }
}

} // namespace GraphicsFeature
