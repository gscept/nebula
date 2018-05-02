#pragma once
//------------------------------------------------------------------------------
/**
	@file componentmacros.h

	Defines some useful macros for quickly setting up components.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "basegamefeature/managers/componentmanager.h"
#include "game/component/componentdata.h"

namespace Game
{

/// Place this at the top of your component to implement common methods and your data structure.
#define __DeclareComponentData(STRUCT) \
public: \
	void RegisterEntity(const Entity& entity) { this->data.RegisterEntity(entity); }\
	void UnregisterEntity(const Entity& entity) { this->data.DeregisterEntity(entity); }\
	uint32_t GetInstance(const Entity& entity) const { return this->data.GetInstance(entity); }\
	Entity GetOwner(const uint32_t& instance) const { return this->data[instance].owner; }\
	SizeT Optimize() { return this->data.Optimize(); } \
private:\
	ComponentData<STRUCT> data;

//------------------------------------------------------------------------------
/**
	@macro __PerValidInstance

	This iterates data and calls METHOD per component instance.
	Note that this requires METHOD to accept argument STRUCT*.

	This version checks whether this components entity is alive before executing METHOD.
	This is useful where it is imperative that an entity exists, ex. when rendering something.
*/
#define __PerValidInstance(STRUCT, METHOD)\
{\
	SizeT i = 0;\
	STRUCT* instance;\
	while (i < this->data.Size())\
	{\
		instance = &this->data[i];\
		if (!Game::EntityManager::Instance()->IsAlive(instance->owner))\
		{\
			this->data.DeregisterEntityImmediate(instance->owner);\
			continue;\
		}\
		METHOD(instance);\
		i++;\
	}\
}\

//------------------------------------------------------------------------------
/**
	@macro __PerInstance

	This iterates data and calls METHOD per component instance.
	Note that this requires METHOD to accept argument STRUCT*.

	ASSURE_VALIDITY defines whether this components entity is required to be alive before executing METHOD.
	This is useful where it is imperative that an entity exists, ex. when rendering something.
*/
#define __PerInstance(STRUCT, METHOD)\
{\
	SizeT i = 0;\
	STRUCT* instance;\
	while (i < this->data.Size())\
	{\
		instance = &this->data[i];\
		METHOD(instance);\
		i++;\
	}\
}\

} // namespace Game
