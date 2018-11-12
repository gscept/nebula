//------------------------------------------------------------------------------
//  componentinterface.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "componentinterface.h"
#include "io/binaryreader.h"
#include "io/binarywriter.h"

namespace Game
{

__ImplementAbstractRootClass(ComponentInterface, 'BsCm')

//------------------------------------------------------------------------------
/**
*/
ComponentInterface::ComponentInterface()
{
	functions.OnActivate = nullptr;
	functions.OnDeactivate = nullptr;
	functions.OnBeginFrame = nullptr;
	functions.OnRender = nullptr;
	functions.OnEndFrame = nullptr;
	functions.OnRenderDebug = nullptr;
	functions.Serialize = nullptr;
	functions.Deserialize = nullptr;
	functions.Optimize = nullptr;
	functions.DestroyAll = nullptr;
	functions.SetParents = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
ComponentInterface::~ComponentInterface()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
const Util::BitField<ComponentEvent::NumEvents>&
ComponentInterface::SubscribedEvents() const
{
	return this->events;
}

//------------------------------------------------------------------------------
/**
*/
const Attr::AttrId&
ComponentInterface::GetAttributeId(IndexT index) const
{
	return this->attributeIds[index];
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<Attr::AttrId>&
ComponentInterface::GetAttributeIds() const
{
	return this->attributeIds;
}

} // namespace Game