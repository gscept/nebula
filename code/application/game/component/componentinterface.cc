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

//------------------------------------------------------------------------------
/**
*/
ComponentInterface::ComponentInterface()
{
	this->enabled = true;

	this->functions.OnActivate = nullptr;
	this->functions.OnDeactivate = nullptr;
	this->functions.OnBeginFrame = nullptr;
	this->functions.OnRender = nullptr;
	this->functions.OnEndFrame = nullptr;
	this->functions.OnRenderDebug = nullptr;
	this->functions.Serialize = nullptr;
	this->functions.Deserialize = nullptr;
	this->functions.DestroyAll = nullptr;
	this->functions.SetParents = nullptr;
	this->functions.OnLoad = nullptr;
	this->functions.OnSave = nullptr;
	this->functions.OnInstanceMoved = nullptr;
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
const Util::StringAtom&
ComponentInterface::GetName() const
{
	return this->componentName;
}

//------------------------------------------------------------------------------
/**
*/
Util::FourCC
ComponentInterface::GetIdentifier() const
{
	return this->fourcc;
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
const Attr::Attribute&
ComponentInterface::GetAttribute(IndexT index) const
{
	return this->attributes[index];
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<Attr::Attribute>&
ComponentInterface::GetAttributes() const
{
	return this->attributes;
}

//------------------------------------------------------------------------------
/**
*/
void ComponentInterface::EnableEvent(ComponentEvent eventId)
{
	this->events.SetBit(eventId);
}

bool
ComponentInterface::Enabled() const
{
	return this->enabled;
}

} // namespace Game