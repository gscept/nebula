#pragma once
//------------------------------------------------------------------------------
/**
    @class Tools::IDLAttribute
    
    Wrap an IDL attribute definition.
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "io/xmlreader.h"

//------------------------------------------------------------------------------
namespace Tools
{
class IDLAttribute : public Core::RefCounted
{
    __DeclareClass(IDLAttribute);
public:
    /// constructor
    IDLAttribute();
    /// parse from XmlReader
    bool Parse(IO::XmlReader* reader);	
    /// get error string
    const Util::String& GetError() const;
    /// get command name
    const Util::String& GetName() const;
	/// does a displayname exist
	const bool HasDisplayName() const;
	/// get command display name
	const Util::String& GetDisplayName() const;
    /// get fourcc string
    const Util::String& GetFourCC() const;
    /// get access mode
    const Util::String& GetAccessMode() const;
	/// get attribute type
	const Util::String& GetType() const;
	/// get optional default value
	Util::String GetDefault();	
	/// attribute has default value
	const bool HasDefault() const;	
	/// attribute is a system attribute
	const bool IsSystem() const;
	/// get default value as raw string
	const Util::String& GetDefaultRaw() const;
	/// attribute has a specific xml attribute
	bool HasAttribute(const Util::String & attr) const;
	/// get specific xml attribute
	const Util::String & GetAttribute(const Util::String & attr) const;

	const Util::Dictionary<Util::String,Util::String> & GetAttributes() const;

private:
	/// set error string
	void SetError(const Util::String& e);
	/// check if attribute is a valid type
	bool IsValidType(const Util::String& t);

	Util::String error;
	Util::String name;
	Util::String fourcc;
	Util::String accessMode;
	Util::String attrType;
	Util::String defaultValue;
	Util::String displayName;
	bool system;
	bool hasDefault;	
	bool hasDisplay;
	Util::Dictionary<Util::String,Util::String> attributes;
};


//------------------------------------------------------------------------------
/**
*/
inline void
IDLAttribute::SetError(const Util::String& e)
{
	this->error = e;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLAttribute::GetError() const
{
	return this->error;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLAttribute::GetName() const
{
	return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
IDLAttribute::HasDisplayName() const
{
	return this->hasDisplay;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLAttribute::GetDisplayName() const
{
	if(this->displayName.IsEmpty())
	{
		return this->name;
	}
	else
	{
		return this->displayName;
	}
	
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLAttribute::GetFourCC() const
{
	return this->fourcc;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLAttribute::GetAccessMode() const
{
	return this->accessMode;
}
//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLAttribute::GetType() const
{
	return this->attrType;
}
//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLAttribute::GetDefaultRaw() const
{
	return this->defaultValue;
}
//------------------------------------------------------------------------------
/**
*/
inline const bool
IDLAttribute::HasDefault() const
{
	return this->hasDefault;
}


//------------------------------------------------------------------------------
/**
*/
inline const bool
IDLAttribute::IsSystem() const
{
	return this->system;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
IDLAttribute::HasAttribute(const Util::String & attr) const
{
	return this->attributes.Contains(attr);
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String &
IDLAttribute::GetAttribute(const Util::String & attr) const
{
	return this->attributes[attr];
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::Dictionary<Util::String,Util::String> & 
IDLAttribute::GetAttributes() const
{
	return this->attributes;
}


//------------------------------------------------------------------------------
/**
*/
inline
Util::String
IDLAttribute::GetDefault()
{
	if(this->attrType == "Float4")
	{
		Util::String ret;
		ret.Format("Math::float4(%s)",this->defaultValue.AsCharPtr());
		return ret;
	}
	if(this->attrType == "String")
	{
		Util::String ret;
		ret.Format("Util::String(\"%s\")",this->defaultValue.AsCharPtr());
		return ret;
	}
	return this->defaultValue;
}
}