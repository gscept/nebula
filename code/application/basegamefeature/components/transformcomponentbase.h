#pragma once
//------------------------------------------------------------------------------
/**
	TransformComponentBase

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "util/string.h"
#include "util/hashtable.h"
#include "math/float4.h"
#include "math/scalar.h"
#include "game/component/basecomponent.h"
#include "game/entity.h"
#include "game/attr/attributedefinitionbase.h"
#include "attributes/transformattr.h"
#include "util/dictionary.h"

namespace Game
{

struct TransformAttributes
{
	Game::Entity owner;
	Math::matrix44 localTransform;
	Math::matrix44 worldTransform;
	uint32_t parent;
	uint32_t firstChild;
	uint32_t nextSibling;
	uint32_t prevSibling;
};

class TransformComponentBase : public BaseComponent
{
	__DeclareClass(TransformComponentBase)
public:
	TransformComponentBase();
	~TransformComponentBase();

	/// Registers an entity to this component. Entity is inactive to begin with.
	void RegisterEntity(const Entity& entity);

	/// Deregister Entity. This checks both active and inactive component instances.
	void DeregisterEntity(const Entity& entity);

	/// Checks whether the entity is registered. Checks both inactive and active datasets.
	bool IsRegistered(const Entity& entity) const;

	virtual void OnActivate(const uint32_t& instance);
	virtual void OnDeactivate(const uint32_t& instance);

	/// Activate entity component instance.
	void Activate(const Entity& entity);

	/// Deactivate entity component instance. Instance state will remain after reactivation but not after deregistering.
	void Deactivate(const Entity& entity);

	/// Returns the index of the data array to the component instance
	/// Note that this only checks the active dataset
	uint32_t GetInstance(const Entity& entity) const;

	/// Returns the owner entity id of provided instance id
	Entity GetOwner(const uint32_t& instance) const;

	/// Optimize data array and pack data
	SizeT Optimize();

	/// Returns an attribute value as a variant from index.
	Util::Variant GetAttributeValue(uint32_t instance, IndexT attributeIndex) const;
	/// Returns an attribute value as a variant from attribute id.
	Util::Variant GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const;

protected:
	/// Read/write access to attributes.
	Math::matrix44& LocalTransform(const uint32_t& instance);
	Math::matrix44& WorldTransform(const uint32_t& instance);
	uint32_t& Parent(const uint32_t& instance);
	uint32_t& FirstChild(const uint32_t& instance);
	uint32_t& NextSibling(const uint32_t& instance);
	uint32_t& PrevSibling(const uint32_t& instance);

private:
	/// Holds all active entities data
	ComponentData<TransformAttributes> data;

	/// Holds all inactive component instances.
	ComponentData<TransformAttributes> inactiveData;

	/// Attributes
	/*
	Ids::IdAllocator<
		Entity,
		Math::matrix44,
		Math::matrix44,
		uint32_t,
		uint32_t,
		uint32_t,
		uint32_t,		
	> data;

	/// Contains the link between InstanceData and Id
	Util::Dictionary<Ids::Id32, uint32_t> idMap;
	*/
};

} // namespace Game