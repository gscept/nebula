//NIDL #version:1#
#pragma once
//------------------------------------------------------------------------------
/**
    This file was generated with Nebula's IDL compiler tool.
    DO NOT EDIT
*/
#include "game/component/componentdata.h"
#include "game/component/basecomponent.h"
#include "game/attr/attrid.h"

//------------------------------------------------------------------------------
namespace Attr
{
    DeclareFloat(Range, 'LRAD', Attr::ReadOnly);
    DeclareFloat4(Color, 'LCLR', Attr::ReadWrite);
    DeclareBool(CastShadows, 'SHDW', Attr::ReadWrite);
    DeclareFloat(Angle, 'LNGL', Attr::ReadWrite);
    DeclareFloat4(Direction, 'LDIR', Attr::ReadWrite);
} // namespace Attr

//------------------------------------------------------------------------------
namespace GraphicsFeature
{
class PointLightComponentBase : public Game::BaseComponent
{
    __DeclareClass(PointLightComponentBase)
public:
    /// Default constructor
    PointLightComponentBase();
    /// Default destructor
    ~PointLightComponentBase();
    
    /// Registers an entity to this component. Entity is inactive to begin with.
    void RegisterEntity(const Game::Entity& entity);
    
    /// Deregister Entity. This checks both active and inactive component instances.
    void DeregisterEntity(const Game::Entity& entity);
    
    /// Deregister all non-alive entities, both inactive and active. This can be extremely slow!
    void DeregisterAllDead();
    
    /// Cleans up right away and frees any memory that does not belong to an entity. This can be extremely slow!
    void CleanData();
    
    /// Destroys all instances of this component, and deregisters every entity.
    void DestroyAll();
    
    /// Checks whether the entity is registered. Checks both inactive and active datasets.
    bool IsRegistered(const Game::Entity& entity) const;
    
    /// Returns the index of the data array to the component instance
    /// Note that this only checks the active dataset
    uint32_t GetInstance(const Game::Entity& entity) const;
    
    /// Returns the owner entity id of provided instance id
    Game::Entity GetOwner(const uint32_t& instance) const;
    
    /// Set the owner of a given instance. This does not care if the entity is registered or not!
    void SetOwner(const uint32_t& i, const Game::Entity& entity);
    
    /// Optimize data array and pack data
    SizeT Optimize();
    
    /// Returns an attribute value as a variant from index.
    Util::Variant GetAttributeValue(uint32_t instance, IndexT attributeIndex) const;
    /// Returns an attribute value as a variant from attribute id.
    Util::Variant GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const;
    
    /// Set an attribute value from index
    void SetAttributeValue(uint32_t instance, IndexT attributeIndex, Util::Variant value);
    /// Set an attribute value from attribute id
    void SetAttributeValue(uint32_t instance, Attr::AttrId attributeId, Util::Variant value);
    
    /// Get data as a blob. @note Copies all memory
    Util::Blob GetBlob() const;
    /// Set data from a blob.
    void SetBlob(const Util::Blob& blob, uint offset, uint numInstances);
    /// Get the total number of instances of this component
    uint32_t GetNumInstances() const;
    /// Allocate multiple instances
    void AllocInstances(uint num);
    /// Called from entitymanager if this component is registered with a deletion callback.
    /// Removes entity immediately from component instances.
    void OnEntityDeleted(Game::Entity entity);
    
protected:
    /// Read/write access to attributes.
    const float& GetAttrRange(const uint32_t& instance);
    void SetAttrRange(const uint32_t& instance, const float& value);
    const Math::float4& GetAttrColor(const uint32_t& instance);
    void SetAttrColor(const uint32_t& instance, const Math::float4& value);
    const bool& GetAttrCastShadows(const uint32_t& instance);
    void SetAttrCastShadows(const uint32_t& instance, const bool& value);
    
private:
    /// Holds all entity instances data
    Game::ComponentData<float, Math::float4, bool> data;
};

class SpotLightComponentBase : public Game::BaseComponent
{
    __DeclareClass(SpotLightComponentBase)
public:
    /// Default constructor
    SpotLightComponentBase();
    /// Default destructor
    ~SpotLightComponentBase();
    
    /// Registers an entity to this component. Entity is inactive to begin with.
    void RegisterEntity(const Game::Entity& entity);
    
    /// Deregister Entity. This checks both active and inactive component instances.
    void DeregisterEntity(const Game::Entity& entity);
    
    /// Deregister all non-alive entities, both inactive and active. This can be extremely slow!
    void DeregisterAllDead();
    
    /// Cleans up right away and frees any memory that does not belong to an entity. This can be extremely slow!
    void CleanData();
    
    /// Destroys all instances of this component, and deregisters every entity.
    void DestroyAll();
    
    /// Checks whether the entity is registered. Checks both inactive and active datasets.
    bool IsRegistered(const Game::Entity& entity) const;
    
    /// Returns the index of the data array to the component instance
    /// Note that this only checks the active dataset
    uint32_t GetInstance(const Game::Entity& entity) const;
    
    /// Returns the owner entity id of provided instance id
    Game::Entity GetOwner(const uint32_t& instance) const;
    
    /// Set the owner of a given instance. This does not care if the entity is registered or not!
    void SetOwner(const uint32_t& i, const Game::Entity& entity);
    
    /// Optimize data array and pack data
    SizeT Optimize();
    
    /// Returns an attribute value as a variant from index.
    Util::Variant GetAttributeValue(uint32_t instance, IndexT attributeIndex) const;
    /// Returns an attribute value as a variant from attribute id.
    Util::Variant GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const;
    
    /// Set an attribute value from index
    void SetAttributeValue(uint32_t instance, IndexT attributeIndex, Util::Variant value);
    /// Set an attribute value from attribute id
    void SetAttributeValue(uint32_t instance, Attr::AttrId attributeId, Util::Variant value);
    
    /// Get data as a blob. @note Copies all memory
    Util::Blob GetBlob() const;
    /// Set data from a blob.
    void SetBlob(const Util::Blob& blob, uint offset, uint numInstances);
    /// Get the total number of instances of this component
    uint32_t GetNumInstances() const;
    /// Allocate multiple instances
    void AllocInstances(uint num);
    /// Called from entitymanager if this component is registered with a deletion callback.
    /// Removes entity immediately from component instances.
    void OnEntityDeleted(Game::Entity entity);
    
protected:
    /// Read/write access to attributes.
    const float& GetAttrRange(const uint32_t& instance);
    void SetAttrRange(const uint32_t& instance, const float& value);
    const float& GetAttrAngle(const uint32_t& instance);
    void SetAttrAngle(const uint32_t& instance, const float& value);
    const Math::float4& GetAttrDirection(const uint32_t& instance);
    void SetAttrDirection(const uint32_t& instance, const Math::float4& value);
    const Math::float4& GetAttrColor(const uint32_t& instance);
    void SetAttrColor(const uint32_t& instance, const Math::float4& value);
    const bool& GetAttrCastShadows(const uint32_t& instance);
    void SetAttrCastShadows(const uint32_t& instance, const bool& value);
    
private:
    /// Holds all entity instances data
    Game::ComponentData<float, float, Math::float4, Math::float4, bool> data;
};

struct DirectionalLightComponentInstance
{
    Math::float4 direction;
    Math::float4 color;
    bool castShadows;
};

//------------------------------------------------------------------------------
class DirectionalLightComponentBase : public Game::BaseComponent
{
    __DeclareClass(DirectionalLightComponentBase)
public:
    /// Default constructor
    DirectionalLightComponentBase();
    /// Default destructor
    ~DirectionalLightComponentBase();
    
    /// Registers an entity to this component. Entity is inactive to begin with.
    void RegisterEntity(const Game::Entity& entity);
    
    /// Deregister Entity. This checks both active and inactive component instances.
    void DeregisterEntity(const Game::Entity& entity);
    
    /// Deregister all non-alive entities, both inactive and active. This can be extremely slow!
    void DeregisterAllDead();
    
    /// Cleans up right away and frees any memory that does not belong to an entity. This can be extremely slow!
    void CleanData();
    
    /// Destroys all instances of this component, and deregisters every entity.
    void DestroyAll();
    
    /// Checks whether the entity is registered. Checks both inactive and active datasets.
    bool IsRegistered(const Game::Entity& entity) const;
    
    /// Returns the index of the data array to the component instance
    /// Note that this only checks the active dataset
    uint32_t GetInstance(const Game::Entity& entity) const;
    
    /// Returns the owner entity id of provided instance id
    Game::Entity GetOwner(const uint32_t& instance) const;
    
    /// Set the owner of a given instance. This does not care if the entity is registered or not!
    void SetOwner(const uint32_t& i, const Game::Entity& entity);
    
    /// Optimize data array and pack data
    SizeT Optimize();
    
    /// Returns an attribute value as a variant from index.
    Util::Variant GetAttributeValue(uint32_t instance, IndexT attributeIndex) const;
    /// Returns an attribute value as a variant from attribute id.
    Util::Variant GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const;
    
    /// Set an attribute value from index
    void SetAttributeValue(uint32_t instance, IndexT attributeIndex, Util::Variant value);
    /// Set an attribute value from attribute id
    void SetAttributeValue(uint32_t instance, Attr::AttrId attributeId, Util::Variant value);
    
    /// Get data as a blob. @note Copies all memory
    Util::Blob GetBlob() const;
    /// Set data from a blob.
    void SetBlob(const Util::Blob& blob, uint offset, uint numInstances);
    /// Get the total number of instances of this component
    uint32_t GetNumInstances() const;
    /// Allocate multiple instances
    void AllocInstances(uint num);
    /// Called from entitymanager if this component is registered with a deletion callback.
    /// Removes entity immediately from component instances.
    void OnEntityDeleted(Game::Entity entity);
    
protected:
    /// Read/write access to attributes.
    const Math::float4& GetAttrDirection(const uint32_t& instance);
    void SetAttrDirection(const uint32_t& instance, const Math::float4& value);
    const Math::float4& GetAttrColor(const uint32_t& instance);
    void SetAttrColor(const uint32_t& instance, const Math::float4& value);
    const bool& GetAttrCastShadows(const uint32_t& instance);
    void SetAttrCastShadows(const uint32_t& instance, const bool& value);
    
private:
    /// Holds all entity instances data
    Game::ComponentData<DirectionalLightComponentInstance> data;
};

} // namespace GraphicsFeature
