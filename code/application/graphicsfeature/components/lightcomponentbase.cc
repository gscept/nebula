// NIDL #version:12#
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
    this->data.data.Get<RANGE>(instance) = Attr::Range.GetDefaultValue().GetFloat();
    this->data.data.Get<COLOR>(instance) = Attr::Color.GetDefaultValue().GetFloat4();
    this->data.data.Get<CASTSHADOWS>(instance) = Attr::CastShadows.GetDefaultValue().GetBool();
    this->data.data.Get<DEBUGNAME>(instance) = Attr::DebugName.GetDefaultValue().GetString();
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
PointLightComponentBase::GetAttributeValue(uint32_t instance, AttributeIndex attributeIndex) const
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
PointLightComponentBase::SetAttributeValue(uint32_t instance, AttributeIndex index, Util::Variant value)
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
PointLightComponentBase::GetRange(const uint32_t& instance)
{
    return this->data.data.Get<1>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::SetRange(const uint32_t& instance, const float& value)
{
    this->data.data.Get<1>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const Math::float4&
PointLightComponentBase::GetColor(const uint32_t& instance)
{
    return this->data.data.Get<2>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::SetColor(const uint32_t& instance, const Math::float4& value)
{
    this->data.data.Get<2>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const bool&
PointLightComponentBase::GetCastShadows(const uint32_t& instance)
{
    return this->data.data.Get<3>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::SetCastShadows(const uint32_t& instance, const bool& value)
{
    this->data.data.Get<3>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const Util::String&
PointLightComponentBase::GetDebugName(const uint32_t& instance)
{
    return this->data.data.Get<4>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::SetDebugName(const uint32_t& instance, const Util::String& value)
{
    this->data.data.Get<4>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
uint32_t
PointLightComponentBase::NumRegistered() const
{
    return this->data.Size();
}

//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::Allocate(uint num)
{
    SizeT first = this->data.data.Size();
    this->data.data.Reserve(first + num);
    this->data.data.GetArray<OWNER>().SetSize(first + num);
    this->data.data.GetArray<RANGE>().Fill(first, num, Attr::Range.GetDefaultValue().GetFloat());
    this->data.data.GetArray<COLOR>().Fill(first, num, Attr::Color.GetDefaultValue().GetFloat4());
    this->data.data.GetArray<CASTSHADOWS>().Fill(first, num, Attr::CastShadows.GetDefaultValue().GetBool());
    this->data.data.GetArray<DEBUGNAME>().Fill(first, num, Attr::DebugName.GetDefaultValue().GetString());
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::OnRangeUpdated(const uint32_t& instance, const float& value)
{
    // Empty - override if necessary
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::OnColorUpdated(const uint32_t& instance, const Math::float4& value)
{
    // Empty - override if necessary
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::OnCastShadowsUpdated(const uint32_t& instance, const bool& value)
{
    // Empty - override if necessary
}


//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::OnDebugNameUpdated(const uint32_t& instance, const Util::String& value)
{
    // Empty - override if necessary
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
    this->data.data.Get<RANGE>(instance) = Attr::Range.GetDefaultValue().GetFloat();;
    this->data.data.Get<ANGLE>(instance) = Attr::Angle.GetDefaultValue().GetFloat();;
    this->data.data.Get<DIRECTION>(instance) = Attr::Direction.GetDefaultValue().GetFloat4();;
    this->data.data.Get<COLOR>(instance) = Attr::Color.GetDefaultValue().GetFloat4();;
    this->data.data.Get<CASTSHADOWS>(instance) = Attr::CastShadows.GetDefaultValue().GetBool();;
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
SpotLightComponentBase::GetAttributeValue(uint32_t instance, AttributeIndex attributeIndex) const
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
SpotLightComponentBase::SetAttributeValue(uint32_t instance, AttributeIndex index, Util::Variant value)
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
SpotLightComponentBase::GetRange(const uint32_t& instance)
{
    return this->data.data.Get<1>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::SetRange(const uint32_t& instance, const float& value)
{
    this->data.data.Get<1>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const float&
SpotLightComponentBase::GetAngle(const uint32_t& instance)
{
    return this->data.data.Get<2>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::SetAngle(const uint32_t& instance, const float& value)
{
    this->data.data.Get<2>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const Math::float4&
SpotLightComponentBase::GetDirection(const uint32_t& instance)
{
    return this->data.data.Get<3>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::SetDirection(const uint32_t& instance, const Math::float4& value)
{
    this->data.data.Get<3>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const Math::float4&
SpotLightComponentBase::GetColor(const uint32_t& instance)
{
    return this->data.data.Get<4>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::SetColor(const uint32_t& instance, const Math::float4& value)
{
    this->data.data.Get<4>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const bool&
SpotLightComponentBase::GetCastShadows(const uint32_t& instance)
{
    return this->data.data.Get<5>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::SetCastShadows(const uint32_t& instance, const bool& value)
{
    this->data.data.Get<5>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
uint32_t
SpotLightComponentBase::NumRegistered() const
{
    return this->data.Size();
}

//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::Allocate(uint num)
{
    SizeT first = this->data.data.Size();
    this->data.data.Reserve(first + num);
    this->data.data.GetArray<OWNER>().SetSize(first + num);
    this->data.data.GetArray<RANGE>().Fill(first, num, Attr::Range.GetDefaultValue().GetFloat());
    this->data.data.GetArray<ANGLE>().Fill(first, num, Attr::Angle.GetDefaultValue().GetFloat());
    this->data.data.GetArray<DIRECTION>().Fill(first, num, Attr::Direction.GetDefaultValue().GetFloat4());
    this->data.data.GetArray<COLOR>().Fill(first, num, Attr::Color.GetDefaultValue().GetFloat4());
    this->data.data.GetArray<CASTSHADOWS>().Fill(first, num, Attr::CastShadows.GetDefaultValue().GetBool());
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::OnRangeUpdated(const uint32_t& instance, const float& value)
{
    // Empty - override if necessary
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::OnAngleUpdated(const uint32_t& instance, const float& value)
{
    // Empty - override if necessary
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::OnDirectionUpdated(const uint32_t& instance, const Math::float4& value)
{
    // Empty - override if necessary
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::OnColorUpdated(const uint32_t& instance, const Math::float4& value)
{
    // Empty - override if necessary
}


//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::OnCastShadowsUpdated(const uint32_t& instance, const bool& value)
{
    // Empty - override if necessary
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
    this->data.data.Get<DIRECTION>(instance) = Attr::Direction.GetDefaultValue().GetFloat4();;
    this->data.data.Get<COLOR>(instance) = Attr::Color.GetDefaultValue().GetFloat4();;
    this->data.data.Get<CASTSHADOWS>(instance) = Attr::CastShadows.GetDefaultValue().GetBool();;
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
DirectionalLightComponentBase::GetAttributeValue(uint32_t instance, AttributeIndex attributeIndex) const
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
DirectionalLightComponentBase::SetAttributeValue(uint32_t instance, AttributeIndex index, Util::Variant value)
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
DirectionalLightComponentBase::GetDirection(const uint32_t& instance)
{
    return this->data.data.Get<1>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::SetDirection(const uint32_t& instance, const Math::float4& value)
{
    this->data.data.Get<1>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const Math::float4&
DirectionalLightComponentBase::GetColor(const uint32_t& instance)
{
    return this->data.data.Get<2>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::SetColor(const uint32_t& instance, const Math::float4& value)
{
    this->data.data.Get<2>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
const bool&
DirectionalLightComponentBase::GetCastShadows(const uint32_t& instance)
{
    return this->data.data.Get<3>(instance);
}


//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::SetCastShadows(const uint32_t& instance, const bool& value)
{
    this->data.data.Get<3>(instance) = value;
}


//------------------------------------------------------------------------------
/**
*/
uint32_t
DirectionalLightComponentBase::NumRegistered() const
{
    return this->data.Size();
}

//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::Allocate(uint num)
{
    SizeT first = this->data.data.Size();
    this->data.data.Reserve(first + num);
    this->data.data.GetArray<OWNER>().SetSize(first + num);
    this->data.data.GetArray<DIRECTION>().Fill(first, num, Attr::Direction.GetDefaultValue().GetFloat4());
    this->data.data.GetArray<COLOR>().Fill(first, num, Attr::Color.GetDefaultValue().GetFloat4());
    this->data.data.GetArray<CASTSHADOWS>().Fill(first, num, Attr::CastShadows.GetDefaultValue().GetBool());
}


//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::OnDirectionUpdated(const uint32_t& instance, const Math::float4& value)
{
    // Empty - override if necessary
}


//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::OnColorUpdated(const uint32_t& instance, const Math::float4& value)
{
    // Empty - override if necessary
}


//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::OnCastShadowsUpdated(const uint32_t& instance, const bool& value)
{
    // Empty - override if necessary
}

} // namespace GraphicsFeature
