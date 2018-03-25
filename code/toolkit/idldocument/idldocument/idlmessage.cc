//------------------------------------------------------------------------------
//  idlmessage.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "idlmessage.h"

namespace Tools
{
__ImplementClass(Tools::IDLMessage, 'ILMS', Core::RefCounted);

using namespace IO;

//------------------------------------------------------------------------------
/**
*/
IDLMessage::IDLMessage() : 
	wrap(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
IDLMessage::Parse(XmlReader* reader)
{
    n_assert(0 != reader);
    n_assert(reader->GetCurrentNodeName() == "Message");

    // parse attributes
    this->name = reader->GetString("name");
    this->fourcc = reader->GetString("fourcc");
	this->wrap = reader->GetOptBool("scripted",false);

    if (reader->HasAttr("derivedFrom"))
    {
        this->parentClass = reader->GetString("derivedFrom");
    }
    else
    {
		this->parentClass = "Messaging::Message";        
    }

    // parse input args
    if (reader->SetToFirstChild("InArg")) do
    {
        Ptr<IDLArg> arg = IDLArg::Create();
        if (!arg->Parse(reader))
        {
            this->SetError(arg->GetError());
            return false;
        }
        this->inArgs.Append(arg);
    }
    while (reader->SetToNextChild("InArg"));

    // parse output args
    if (reader->SetToFirstChild("OutArg")) do
    {
        Ptr<IDLArg> arg = IDLArg::Create();
        if (!arg->Parse(reader))
        {
            this->SetError(arg->GetError());
            return false;
        }
        this->outArgs.Append(arg);
    }
    while (reader->SetToNextChild("OutArg"));
	if(reader->SetToFirstChild("Desc"))
	{
		this->description = reader->GetContent();

        // replace line feeds with HTML line feeds, and tabs with HTML horizontal rules
        this->description.SubstituteString("\\n", "<br />");
		reader->SetToParent();
	}
    return true;
}

} // namespace Tools