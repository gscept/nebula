#pragma once
#ifndef TOOLS_IDLDOCUMENT_H
#define TOOLS_IDLDOCUMENT_H
//------------------------------------------------------------------------------
/**
    @class Tools::IDLDocument
    
    Wraps a IDL document definition.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "util/string.h"
#include "io/xmlreader.h"
#include "idldocument/idllibrary.h"
#include "idldocument/idlprotocol.h"
#include "idldocument/idlproperty.h"
#include "idlattributelib.h"

//------------------------------------------------------------------------------
namespace Tools
{
class IDLDocument : public Core::RefCounted
{
    __DeclareClass(IDLDocument);
public:
    /// constructor
    IDLDocument();
    /// parse from XML reader
    bool Parse(IO::XmlReader* reader);
    /// get error string
    const Util::String& GetError() const;
    /// set document name (extracted from filename)
    void SetName(const Util::String& n);
    /// get document name
    const Util::String& GetName() const;
    /// get parsed command libraries
    const Util::Array<Ptr<IDLLibrary>>& GetLibraries() const;
    /// get parsed message protocols
    const Util::Array<Ptr<IDLProtocol>>& GetProtocols() const;
	/// get parsed attributes
	const Util::Array<Ptr<IDLAttributeLib>>& GetAttributeLibs() const;
	/// get parsed property
	const Util::Array<Ptr<IDLProperty>>& GetProperties() const;

private:    
    /// set error string
    void SetError(const Util::String& e);
        
    Util::String name;
    Util::String error;
    Util::Array<Ptr<IDLLibrary>> libraries;
    Util::Array<Ptr<IDLProtocol>> protocols;
	Util::Array<Ptr<IDLAttributeLib>> attributes;
	Util::Array<Ptr<IDLProperty>> properties;
};

//------------------------------------------------------------------------------
/**
*/
inline void
IDLDocument::SetError(const Util::String& e)
{
    this->error = e;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLDocument::GetError() const
{
    return this->error;
}

//------------------------------------------------------------------------------
/**
*/
inline void
IDLDocument::SetName(const Util::String& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLDocument::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<IDLLibrary>>&
IDLDocument::GetLibraries() const
{
    return this->libraries;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<IDLProtocol>>&
IDLDocument::GetProtocols() const
{
    return this->protocols;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<IDLAttributeLib>>&
IDLDocument::GetAttributeLibs() const
{
	return this->attributes;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<IDLProperty>>&
IDLDocument::GetProperties() const
{
	return this->properties;
}

} // namespace Tools
//------------------------------------------------------------------------------
#endif
