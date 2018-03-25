//------------------------------------------------------------------------------
//  idlproperty.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "idlproperty.h"

namespace Tools
{
__ImplementClass(Tools::IDLProperty, 'ILPO', Core::RefCounted);

using namespace IO;

//------------------------------------------------------------------------------
/**
*/
IDLProperty::IDLProperty()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool
IDLProperty::Parse(XmlReader* reader)
{
	n_assert(0 != reader);
	n_assert(reader->GetCurrentNodeName() == "Property");

	// parse attributes
	this->name = reader->GetString("name");		

	if (reader->HasAttr("derivedFrom"))
	{
		this->parentClass = reader->GetString("derivedFrom");
	}
	else
	{
		this->parentClass = "Game::Property";        
	}

	if (reader->HasAttr("header"))
	{
		this->header = reader->GetString("header");
	}
	// parse input args
	if (reader->SetToFirstChild("Attribute")) do
	{
		this->attributes.Append(reader->GetString("name"));		
		bool serialize = reader->GetOptBool("serialize", false);
		this->serializeFlags.Append(serialize);
	}
	while (reader->SetToNextChild("Attribute"));	
	return true;
}

} // namespace Tools