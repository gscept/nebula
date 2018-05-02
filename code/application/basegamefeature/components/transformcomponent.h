#pragma once
//------------------------------------------------------------------------------
/**
	TransformComponentBase

	This is an example component.
	Components that use reloadable shared libraries be automatically generated later.

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

namespace Game
{

typedef struct
{
	// general component attributes
	Entity owner; // Owner entity

	// specific component attributes
	Math::matrix44 localTransform;
	Math::matrix44 worldTransform;
	uint32_t parent;
	uint32_t firstChild;
	uint32_t nextSibling;
	uint32_t prevSibling;
} TransformComponentInstance;

class TransformComponent : public BaseComponent
{
	__DeclareClass(TransformComponent)
	__DeclareComponentData(TransformComponentInstance)
public:
	TransformComponent();
	~TransformComponent();

	void OnBeginFrame();

	void SetLocalTransform(const uint32_t& instance, const Math::matrix44& val);
	Math::matrix44 GetLocalTransform(const uint32_t& instance) const;
	void SetWorldTransform(const uint32_t& instance, const Math::matrix44& val);
	Math::matrix44 GetWorldTransform(const uint32_t& instance) const;
	void SetParent(const uint32_t& instance, const uint32_t& val);
	uint32_t GetParent(const uint32_t& instance) const;
	uint32_t GetFirstChild(const uint32_t& instance) const;
	uint32_t GetNextSibling(const uint32_t& instance) const;
	uint32_t GetPrevSibling(const uint32_t& instance) const;
};

} // namespace Game