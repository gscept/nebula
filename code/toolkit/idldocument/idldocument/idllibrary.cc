//------------------------------------------------------------------------------
//  idllibrary.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "idllibrary.h"
#include "idlmessage.h"

namespace Tools
{
__ImplementClass(Tools::IDLLibrary, 'ILLB', Core::RefCounted);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
IDLLibrary::IDLLibrary()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
IDLLibrary::Parse(XmlReader* reader)
{
    n_assert(0 != reader);
    n_assert(reader->GetCurrentNodeName() == "Library");

    // parse attributes
    this->name = reader->GetString("name");
    
    // parse dependency definitions
    if (reader->SetToFirstChild("Dependency")) do
    {
        Ptr<IDLDependency> dep = IDLDependency::Create();
        if (!dep->Parse(reader))
        {
            this->SetError(dep->GetError());
            return false;
        }
        this->dependencies.Append(dep);
    }
    while (reader->SetToNextChild("Dependency"));
    
    // parse command definitions
    if (reader->SetToFirstChild("Command")) do
    {
        Ptr<IDLCommand> cmd = IDLCommand::Create();
        if (!cmd->Parse(reader))
        {
            this->SetError(cmd->GetError());
            return false;
        }
        this->commands.Append(cmd);
    }
    while (reader->SetToNextChild("Command"));
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool 
IDLLibrary::WrapProtocol(const Ptr<IDLProtocol> & protocol)
{
	this->name = protocol->GetName();
	this->dependencies = protocol->GetDependencies();
	Ptr<IDLDependency> dep = IDLDependency::Create();
	dep->CreateFromString("basegamefeature/managers/entitymanager.h");
	this->dependencies.Append(dep);
	Util::Array<Ptr<IDLMessage>> messages = protocol->GetMessages();
	for(int i=0;i<messages.Size();i++)
	{
		if(messages[i]->WantsWrap())
		{
			Ptr<IDLCommand> cmd = IDLCommand::Create();
			if(!cmd->WrapMessage(messages[i],protocol->GetNameSpace()))
			{
				this->SetError(cmd->GetError());
				return false;
			}
			this->commands.Append(cmd);
		}		
	}
	return true;
}


} // namespace Tools