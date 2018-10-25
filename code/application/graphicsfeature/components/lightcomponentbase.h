// NIDL #version:15#
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
    DeclareString(DebugName, 'tStr', Attr::ReadWrite);
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

    enum AttributeIndex
    {
        OWNER,
        RANGE,
        COLOR,
        CASTSHADOWS,
        DEBUGNAME,

        NumAttributes
    };

    /// Registers an entity to this component.
    void RegisterEntity(const Game::Entity& entity);
    
    /// Deregister Entity.
    void DeregisterEntity(const Game::Entity& entity);
    
    /// Cleans up right away and frees any memory that does not belong to an entity. (slow!)
    void CleanData();
    
    /// Destroys all instances of this component, and deregisters every entity.
    void DestroyAll();
    
    /// Checks whether the entity is registered.
    bool IsRegistered(const Game::Entity& entity) const;
    
    /// Returns the index of the data array to the component instance
    uint32_t GetInstance(const Game::Entity& entity) const;
    
    /// Returns the owner entity id of provided instance id
    Game::Entity GetOwner(const uint32_t& instance) const;
    
    /// Set the owner of a given instance. This does not care if the entity is registered or not!
    void SetOwner(const uint32_t& i, const Game::Entity& entity);
    
    /// Optimize data array and pack data
    SizeT Optimize();
    
    /// Returns an attribute value as a variant from index.
    Util::Variant GetAttributeValue(uint32_t instance, AttributeIndex attributeIndex) const;
    
    /// Returns an attribute value as a variant from attribute id.
    Util::Variant GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const;
    
    /// Set an attribute value from index
    void SetAttributeValue(uint32_t instance, AttributeIndex attributeIndex, Util::Variant value);
    
    /// Set an attribute value from attribute id
    void SetAttributeValue(uint32_t instance, Attr::AttrId attributeId, Util::Variant value);
    
    /// Serialize component into binary stream
    void Serialize(const Ptr<IO::BinaryWriter>& writer) const;

    /// Deserialize from binary stream and set data.
    void Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances);

    /// Get the total number of instances of this component
    uint32_t NumRegistered() const;

    /// Allocate multiple instances
    void Allocate(uint num);
    

    /// Read/write access to attributes.
    const float& GetRange(const uint32_t& instance);
    void SetRange(const uint32_t& instance, const float& value);
    const Math::float4& GetColor(const uint32_t& instance);
    void SetColor(const uint32_t& instance, const Math::float4& value);
    const bool& GetCastShadows(const uint32_t& instance);
    void SetCastShadows(const uint32_t& instance, const bool& value);
    const Util::String& GetDebugName(const uint32_t& instance);
    void SetDebugName(const uint32_t& instance, const Util::String& value);


protected:
    /// Callbacks for reacting to updated attributes.
    virtual void OnRangeUpdated(const uint32_t& instance, const float& value);
    virtual void OnColorUpdated(const uint32_t& instance, const Math::float4& value);
    virtual void OnCastShadowsUpdated(const uint32_t& instance, const bool& value);
    virtual void OnDebugNameUpdated(const uint32_t& instance, const Util::String& value);


private:
    /// Holds all entity instances data")
    Game::ComponentData<float, Math::float4, bool, Util::String> data;
};
        

class SpotLightComponentBase : public Game::BaseComponent
{
    __DeclareClass(SpotLightComponentBase)

public:
    /// Default constructor
    SpotLightComponentBase();
    /// Default destructor
    ~SpotLightComponentBase();

    enum AttributeIndex
    {
        OWNER,
        RANGE,
        ANGLE,
        DIRECTION,
        COLOR,
        CASTSHADOWS,

        NumAttributes
    };

    /// Registers an entity to this component.
    void RegisterEntity(const Game::Entity& entity);
    
    /// Deregister Entity.
    void DeregisterEntity(const Game::Entity& entity);
    
    /// Cleans up right away and frees any memory that does not belong to an entity. (slow!)
    void CleanData();
    
    /// Destroys all instances of this component, and deregisters every entity.
    void DestroyAll();
    
    /// Checks whether the entity is registered.
    bool IsRegistered(const Game::Entity& entity) const;
    
    /// Returns the index of the data array to the component instance
    uint32_t GetInstance(const Game::Entity& entity) const;
    
    /// Returns the owner entity id of provided instance id
    Game::Entity GetOwner(const uint32_t& instance) const;
    
    /// Set the owner of a given instance. This does not care if the entity is registered or not!
    void SetOwner(const uint32_t& i, const Game::Entity& entity);
    
    /// Optimize data array and pack data
    SizeT Optimize();
    
    /// Returns an attribute value as a variant from index.
    Util::Variant GetAttributeValue(uint32_t instance, AttributeIndex attributeIndex) const;
    
    /// Returns an attribute value as a variant from attribute id.
    Util::Variant GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const;
    
    /// Set an attribute value from index
    void SetAttributeValue(uint32_t instance, AttributeIndex attributeIndex, Util::Variant value);
    
    /// Set an attribute value from attribute id
    void SetAttributeValue(uint32_t instance, Attr::AttrId attributeId, Util::Variant value);
    
    /// Serialize component into binary stream
    void Serialize(const Ptr<IO::BinaryWriter>& writer) const;

    /// Deserialize from binary stream and set data.
    void Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances);

    /// Get the total number of instances of this component
    uint32_t NumRegistered() const;

    /// Allocate multiple instances
    void Allocate(uint num);
    
            /// Called from entitymanager if this component is registered with a deletion callback.
            /// Removes entity immediately from component instances.
            void OnEntityDeleted(Game::Entity entity);
            
            

    /// Read/write access to attributes.
    const float& GetRange(const uint32_t& instance);
    void SetRange(const uint32_t& instance, const float& value);
    const float& GetAngle(const uint32_t& instance);
    void SetAngle(const uint32_t& instance, const float& value);
    const Math::float4& GetDirection(const uint32_t& instance);
    void SetDirection(const uint32_t& instance, const Math::float4& value);
    const Math::float4& GetColor(const uint32_t& instance);
    void SetColor(const uint32_t& instance, const Math::float4& value);
    const bool& GetCastShadows(const uint32_t& instance);
    void SetCastShadows(const uint32_t& instance, const bool& value);


protected:
    /// Callbacks for reacting to updated attributes.
    virtual void OnRangeUpdated(const uint32_t& instance, const float& value);
    virtual void OnAngleUpdated(const uint32_t& instance, const float& value);
    virtual void OnDirectionUpdated(const uint32_t& instance, const Math::float4& value);
    virtual void OnColorUpdated(const uint32_t& instance, const Math::float4& value);
    virtual void OnCastShadowsUpdated(const uint32_t& instance, const bool& value);


private:
    /// Holds all entity instances data")
    Game::ComponentData<float, float, Math::float4, Math::float4, bool> data;
};
        

class DirectionalLightComponentBase : public Game::BaseComponent
{
    __DeclareClass(DirectionalLightComponentBase)

public:
    /// Default constructor
    DirectionalLightComponentBase();
    /// Default destructor
    ~DirectionalLightComponentBase();

    enum AttributeIndex
    {
        OWNER,
        DIRECTION,
        COLOR,
        CASTSHADOWS,

        NumAttributes
    };

    /// Registers an entity to this component.
    void RegisterEntity(const Game::Entity& entity);
    
    /// Deregister Entity.
    void DeregisterEntity(const Game::Entity& entity);
    
    /// Cleans up right away and frees any memory that does not belong to an entity. (slow!)
    void CleanData();
    
    /// Destroys all instances of this component, and deregisters every entity.
    void DestroyAll();
    
    /// Checks whether the entity is registered.
    bool IsRegistered(const Game::Entity& entity) const;
    
    /// Returns the index of the data array to the component instance
    uint32_t GetInstance(const Game::Entity& entity) const;
    
    /// Returns the owner entity id of provided instance id
    Game::Entity GetOwner(const uint32_t& instance) const;
    
    /// Set the owner of a given instance. This does not care if the entity is registered or not!
    void SetOwner(const uint32_t& i, const Game::Entity& entity);
    
    /// Optimize data array and pack data
    SizeT Optimize();
    
    /// Returns an attribute value as a variant from index.
    Util::Variant GetAttributeValue(uint32_t instance, AttributeIndex attributeIndex) const;
    
    /// Returns an attribute value as a variant from attribute id.
    Util::Variant GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const;
    
    /// Set an attribute value from index
    void SetAttributeValue(uint32_t instance, AttributeIndex attributeIndex, Util::Variant value);
    
    /// Set an attribute value from attribute id
    void SetAttributeValue(uint32_t instance, Attr::AttrId attributeId, Util::Variant value);
    
    /// Serialize component into binary stream
    void Serialize(const Ptr<IO::BinaryWriter>& writer) const;

    /// Deserialize from binary stream and set data.
    void Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances);

    /// Get the total number of instances of this component
    uint32_t NumRegistered() const;

    /// Allocate multiple instances
    void Allocate(uint num);
    
            /// Called from entitymanager if this component is registered with a deletion callback.
            /// Removes entity immediately from component instances.
            void OnEntityDeleted(Game::Entity entity);
            
            

    /// Read/write access to attributes.
    const Math::float4& GetDirection(const uint32_t& instance);
    void SetDirection(const uint32_t& instance, const Math::float4& value);
    const Math::float4& GetColor(const uint32_t& instance);
    void SetColor(const uint32_t& instance, const Math::float4& value);
    const bool& GetCastShadows(const uint32_t& instance);
    void SetCastShadows(const uint32_t& instance, const bool& value);


protected:
    /// Callbacks for reacting to updated attributes.
    virtual void OnDirectionUpdated(const uint32_t& instance, const Math::float4& value);
    virtual void OnColorUpdated(const uint32_t& instance, const Math::float4& value);
    virtual void OnCastShadowsUpdated(const uint32_t& instance, const bool& value);


private:
    /// Holds all entity instances data")
    Game::ComponentData<Math::float4, Math::float4, bool> data;
};
        
} // namespace GraphicsFeature
