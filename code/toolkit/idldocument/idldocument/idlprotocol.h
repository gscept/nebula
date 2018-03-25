#pragma once
#ifndef TOOLS_IDLPROTOCOL_H
#define TOOLS_IDLPROTOCOL_H
//------------------------------------------------------------------------------
/**
    @class Tools::IDLProtocol
    
    Wraps an IDL protocol definition.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "util/string.h"
#include "io/xmlreader.h"
#include "idldocument/idldependency.h"
#include "idldocument/idlmessage.h"

//------------------------------------------------------------------------------
namespace Tools
{
class IDLProtocol : public Core::RefCounted
{
    __DeclareClass(IDLProtocol);
public:
    /// constructor
    IDLProtocol();
    /// parse the object from an XmlReader
    bool Parse(IO::XmlReader* reader);
    /// get error string
    const Util::String& GetError() const;
    /// get protocol name
    const Util::String& GetName() const;
    /// get protocol namespace
    const Util::String& GetNameSpace() const;
    /// get protocol fourcc code
    const Util::String& GetFourCC() const;
    /// get dependencies
    const Util::Array<Ptr<IDLDependency>>& GetDependencies() const;
    /// get messages
    const Util::Array<Ptr<IDLMessage>>& GetMessages() const;	    
	
private:
    /// set error string
    void SetError(const Util::String& e);

    Util::String error;
    Util::String name;
    Util::String nameSpace;
    Util::String fourcc;	
    Util::Array<Ptr<IDLDependency>> dependencies;
    Util::Array<Ptr<IDLMessage>> messages;  	
};

//------------------------------------------------------------------------------
/**
*/
inline void
IDLProtocol::SetError(const Util::String& e)
{
    this->error = e;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLProtocol::GetError() const
{
    return this->error;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLProtocol::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLProtocol::GetNameSpace() const
{
    return this->nameSpace;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLProtocol::GetFourCC() const
{
    return this->fourcc;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<IDLDependency>>&
IDLProtocol::GetDependencies() const
{
    return this->dependencies;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<IDLMessage>>&
IDLProtocol::GetMessages() const
{
    return this->messages;
}

} // namespace Tools
//------------------------------------------------------------------------------
#endif