#pragma once
#ifndef TOOLS_IDLLIBRARY_H
#define TOOLS_IDLLIBRARY_H
//------------------------------------------------------------------------------
/**
    @class Tools::IDLLibrary
    
    Wraps a IDL library definition.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "util/string.h"
#include "io/xmlreader.h"
#include "idldocument/idldependency.h"
#include "idldocument/idlcommand.h"
#include "idldocument/idlprotocol.h"

//------------------------------------------------------------------------------
namespace Tools
{
class IDLLibrary : public Core::RefCounted
{
    __DeclareClass(IDLLibrary);
public:
    /// constructor
    IDLLibrary();
    /// parse the object from an XmlReader
    bool Parse(IO::XmlReader* reader);
    /// get error string
    const Util::String& GetError() const;
    /// get library name
    const Util::String& GetName() const;
    /// get dependencies
    const Util::Array<Ptr<IDLDependency>>& GetDependencies() const;
    /// get commands
    const Util::Array<Ptr<IDLCommand>>& GetCommands() const;
	/// wrap a protocol file
	bool WrapProtocol(const Ptr<IDLProtocol> & protocol);

private:
    /// set error string
    void SetError(const Util::String& e);

    Util::String error;
    Util::String name;
    Util::Array<Ptr<IDLDependency>> dependencies;
    Util::Array<Ptr<IDLCommand>> commands;
};

//------------------------------------------------------------------------------
/**
*/
inline void
IDLLibrary::SetError(const Util::String& e)
{
    this->error = e;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLLibrary::GetError() const
{
    return this->error;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLLibrary::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<IDLDependency>>&
IDLLibrary::GetDependencies() const
{
    return this->dependencies;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<IDLCommand>>&
IDLLibrary::GetCommands() const
{
    return this->commands;
}

} // namespace Tools
//------------------------------------------------------------------------------
#endif