//------------------------------------------------------------------------------
//  idldocument.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "idldocument.h"

namespace Tools
{
__ImplementClass(Tools::IDLDocument, 'ILDC', Core::RefCounted);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
IDLDocument::IDLDocument()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
IDLDocument::Parse(XmlReader* reader)
{
    n_assert(0 != reader);
    n_assert(reader->GetCurrentNodeName() == "Nebula3");

    // parse Library elements
    if (reader->SetToFirstChild("Library")) do
    {
        Ptr<IDLLibrary> lib = IDLLibrary::Create();
        if (!lib->Parse(reader))
        {
            this->SetError(lib->GetError());
            return false;
        }
        this->libraries.Append(lib);
    }
    while (reader->SetToNextChild("Library"));

    // parse protocol elements
    if (reader->SetToFirstChild("Protocol")) do
    {
        Ptr<IDLProtocol> protocol = IDLProtocol::Create();
        if (!protocol->Parse(reader))
        {
            this->SetError(protocol->GetError());
            return false;
        }
        this->protocols.Append(protocol);		
		
		Ptr<IDLLibrary> lib = IDLLibrary::Create();
		if (!lib->WrapProtocol(protocol))
		{
			this->SetError(lib->GetError());
			return false;
		}
		if(lib->GetCommands().Size() > 0)
		{			
			this->libraries.Append(lib);
		}				
    }
    while (reader->SetToNextChild("Protocol"));

	// parse Attribute elements
	if (reader->SetToFirstChild("AttributeLib")) do
	{
		Ptr<IDLAttributeLib> lib = IDLAttributeLib::Create();
		if (!lib->Parse(reader))
		{
			this->SetError(lib->GetError());
			return false;
		}
		this->attributes.Append(lib);
	}
	while (reader->SetToNextChild("AttributeLib"));

	// parse property elements
	if (reader->SetToFirstChild("Property")) do
	{
		Ptr<IDLProperty> prop = IDLProperty::Create();
		if (!prop->Parse(reader))
		{
			this->SetError(prop->GetError());
			return false;
		}
		this->properties.Append(prop);
	}
	while (reader->SetToNextChild("Property"));

    return true;
}

} // namespace Tools