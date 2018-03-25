#pragma once
//------------------------------------------------------------------------------
/**
    @class Tools::IDLProperty
    
    Wrap an IDL property definition
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "io/xmlreader.h"

//------------------------------------------------------------------------------
namespace Tools
{
class IDLProperty : public Core::RefCounted
{
    __DeclareClass(IDLProperty);
public:
    /// constructor
    IDLProperty();
    /// parse from XmlReader
    bool Parse(IO::XmlReader* reader);
    /// get error string
    const Util::String& GetError() const;
    /// get command name
    const Util::String& GetName() const;    
    /// get attribute names
	const Util::Array<Util::String>& GetAttributes() const;    
	/// get serialize flags
	const Util::Array<bool>& GetSerialize() const;
    /// get parent class name
    const Util::String& GetParentClass() const;	
	/// get property header
	const Util::String& GetHeader() const;

private:
    /// set error string
    void SetError(const Util::String& e);

    Util::String error;
    Util::String name;    
	Util::String header;
	Util::Array<Util::String> attributes; 
	Util::Array<bool> serializeFlags;
    Util::String parentClass;	
};

//------------------------------------------------------------------------------
/**
*/
inline void
IDLProperty::SetError(const Util::String& e)
{
    this->error = e;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLProperty::GetError() const
{
    return this->error;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLProperty::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Util::String>&
IDLProperty::GetAttributes() const
{
    return this->attributes;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<bool>&
IDLProperty::GetSerialize() const
{
	return this->serializeFlags;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLProperty::GetParentClass() const
{
    return this->parentClass;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLProperty::GetHeader() const
{
	return this->header;
}



} // namespace Tools
