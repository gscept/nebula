#pragma once
//------------------------------------------------------------------------------
/**
	TransformComponent

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "game/component/basecomponent.h"
#include "util/dictionary.h"
#include "game/component/componentdata.h"
#include "game/attr/attrid.h"

//------------------------------------------------------------------------------
namespace Attr
{
	DeclareMatrix44(LocalTransform, 'TRLT', Attr::ReadWrite);
	DeclareMatrix44(WorldTransform, 'TRWT', Attr::ReadWrite);
	DeclareUInt(Parent, 'TRPT', Attr::ReadOnly);
	DeclareUInt(FirstChild, 'TRFC', Attr::ReadOnly);
	DeclareUInt(NextSibling, 'TRNS', Attr::ReadOnly);
	DeclareUInt(PreviousSibling, 'TRPS', Attr::ReadOnly);
};
//------------------------------------------------------------------------------

namespace Game
{

class TransformComponent : public BaseComponent
{
	__DeclareClass(TransformComponent)
public:
	TransformComponent();
	~TransformComponent();

	void SetLocalTransform(const uint32_t& instance, const Math::matrix44& val);

	/// Update relationships
	void SetParents(const uint32_t& start, const uint32_t& end, const Util::Array<Entity>& entities, const Util::Array<uint32_t>& parentIndices);

	/// Registers an entity to this component. Entity is inactive to begin with.
	void RegisterEntity(const Entity& entity);

	/// Deregister Entity. This checks both active and inactive component instances.
	void DeregisterEntity(const Entity& entity);

	uint32_t GetNumInstances() const;

	/// Allocate multiple instances
	void AllocInstances(uint num);

	void SetDataFromBlobs(uint from, uint to, const Util::Array<Util::Blob>& data);

	Util::Array<Util::Blob> GetDataAsBlobs();

	Util::Array<Util::Array<Game::Entity>*> GetEntityAttributes();

	/// Deregister all entities from both inactive and active. Garbage collection will take care of freeing up data.
	void DeregisterAll();

	/// Deregister all non-alive entities from both inactive and active. This can be extremely slow!
	void DeregisterAllDead();

	/// Cleans up right away and frees any memory that does not belong to an entity. This can be extremely slow!
	void CleanData();

	/// Destroys all instances of this component, and deregisters every entity.
	void DestroyAll();

	/// Checks whether the entity is registered. Checks both inactive and active datasets.
	bool IsRegistered(const Entity& entity) const;

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

private:
	/// Read/write access to attributes.
	Math::matrix44& LocalTransform(const uint32_t& instance);
	Math::matrix44& WorldTransform(const uint32_t& instance);
	uint32_t& Parent(const uint32_t& instance);
	uint32_t& FirstChild(const uint32_t& instance);
	uint32_t& NextSibling(const uint32_t& instance);
	uint32_t& PrevSibling(const uint32_t& instance);

	/// Holds all entities data
	/// 1 = localTransform, 2 = worldTransform, 3 = parent, 4 = firstChild, 5 = nextSibling, 6 = prevSibling
	ComponentData<Math::matrix44, Math::matrix44, uint32_t, uint32_t, uint32_t, uint32_t> data;
};

} // namespace Game