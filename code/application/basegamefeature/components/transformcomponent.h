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
#include "game/component/component.h"
#include "game/attr/attrid.h"
#include "basegamefeature/messages/basegameprotocol.h"

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

class TransformComponent : public Component<
	decltype(Attr::LocalTransform), 
	decltype(Attr::WorldTransform), 
	decltype(Attr::Parent),
	decltype(Attr::FirstChild),
	decltype(Attr::NextSibling),
	decltype(Attr::PreviousSibling)
>
{
	__DeclareClass(TransformComponent)
public:
	TransformComponent();
	~TransformComponent();

	enum Attributes
	{
		OWNER,
		LOCALTRANSFORM,
		WORLDTRANSFORM,
		PARENT,
		FIRSTCHILD,
		NEXTSIBLING,
		PREVIOUSSIBLING,
		NumAttributes
	};

	/// Setup callbacks.
	void SetupAcceptedMessages();

	/// Set the local transform of instance. Will update hierarchy
	void SetLocalTransform(const uint32_t& instance, const Math::matrix44& val);

	/// Set the local transform of entity. Will update hierarchy
	void SetLocalTransform(const Game::Entity& entity, const Math::matrix44& val);

	/// Return the local transform of an instance.
	Math::matrix44 GetLocalTransform(const uint32_t& instance);

	/// Return the local transform of an entity.
	Math::matrix44 GetLocalTransform(const Game::Entity& entity);

	/// Return the world transform of an instance.
	Math::matrix44 GetWorldTransform(const uint32_t& instance);

	/// Return the world transform of an entity.
	Math::matrix44 GetWorldTransform(const Game::Entity& entity);

	/// Update relationships
	void SetParents(const uint32_t& start, const uint32_t& end, const Util::Array<Entity>& entities, const Util::Array<uint32_t>& parentIndices);

	/// Set parent of specific entity
	void SetParent(const Game::Entity& entity, const Game::Entity& parent);

	/// Optimize data array and pack data
	SizeT Optimize();

	/// Returns an attribute value as a variant from index.
	Util::Variant GetAttributeValue(uint32_t instance, IndexT attributeIndex) const;
	/// Returns an attribute value as a variant from attribute id.
	Util::Variant GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const;

	void SetAttributeValue(uint32_t instance, IndexT attributeIndex, Util::Variant value);
	
	void SetAttributeValue(uint32_t instance, Attr::AttrId attributeId, Util::Variant value);
private:

	void UpdateHierarchy(uint32_t instance);

	/// Read/write access to attributes.
	Math::matrix44& LocalTransform(const uint32_t& instance);
	Math::matrix44& WorldTransform(const uint32_t& instance);
	uint32_t& Parent(const uint32_t& instance);
	uint32_t& FirstChild(const uint32_t& instance);
	uint32_t& NextSibling(const uint32_t& instance);
	uint32_t& PrevSibling(const uint32_t& instance);

	Msg::UpdateTransform::MessageQueueId messageQueue;
};

} // namespace Game