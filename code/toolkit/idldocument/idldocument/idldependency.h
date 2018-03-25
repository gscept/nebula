#pragma once
#ifndef TOOLS_IDLDEPENDENCY_H
#define TOOLS_IDLDEPENDENCY_H
//------------------------------------------------------------------------------
/**
    @class Tools::IDLDependency
    
    Wraps a IDL dependency.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "io/xmlreader.h"

//------------------------------------------------------------------------------
namespace Tools
{
class IDLDependency : public Core::RefCounted
{
    __DeclareClass(IDLDependency);
public:
    /// constructor
    IDLDependency();
	/// constructor
	void CreateFromString(const Util::String & header);
    /// parse from XML reader
    bool Parse(IO::XmlReader* reader);
    /// get error string
    const Util::String& GetError() const;
    /// get header filename
    const Util::String& GetHeader() const;

private:
    /// set error string
    void SetError(const Util::String& e);

    Util::String header;
    Util::String error;
};

//------------------------------------------------------------------------------
/**
*/
inline void
IDLDependency::SetError(const Util::String& e)
{
    this->error = e;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLDependency::GetError() const
{
    return this->error;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLDependency::GetHeader() const
{
    return this->header;
}

}; // namespace Tool
//------------------------------------------------------------------------------
#endif
    