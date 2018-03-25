#pragma once
//------------------------------------------------------------------------------
/**
    @class Tools::IDLArg
    
    Wrap an IDL argument definition.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#include "core/refcounted.h"
#include "io/xmlreader.h"

//------------------------------------------------------------------------------
namespace Tools
{
class IDLArg : public Core::RefCounted
{
    __DeclareClass(IDLArg);
public:
    /// constructor
    IDLArg();	
    /// parse from XmlReader
    bool Parse(IO::XmlReader* reader);
	/// manually create an arg
	bool CreateFromArgs(const Util::String & name, const Util::String & type, const Util::String & defaultValue, const Util::String & wrappingType);
    /// get error string
    const Util::String& GetError() const;
    /// get argument type
    const Util::String& GetType() const;    
    /// get argument name
    const Util::String& GetName() const;
    /// get optional default value
    const Util::String& GetDefaultValue() const;
    /// get optional serialization code
    const Util::String& GetEncodeCode() const;
    /// get optional deserialization code
    const Util::String& GetDecodeCode() const;
	/// get optional wrapping type used when converting typedefed basic types to the scripting interface
	const Util::String& GetWrappingType() const;
    /// get serialize flag
    bool IsSerialized() const;

    /// check if string is a valid simple type
    static bool IsValidType(const Util::String& str);

private:
    /// set error string
    void SetError(const Util::String& e);

    Util::String type;
    Util::String name;
    Util::String error;
    Util::String defaultValue;
    Util::String encodeCode;
    Util::String decodeCode;
	Util::String wrappingType;	
    bool serialize;	
};

//------------------------------------------------------------------------------
/**
*/
inline void
IDLArg::SetError(const Util::String& e)
{
    this->error = e;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLArg::GetError() const
{
    return this->error;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLArg::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLArg::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLArg::GetDefaultValue() const
{
    return this->defaultValue;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLArg::GetWrappingType() const
{
	return this->wrappingType;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
IDLArg::IsSerialized() const
{
    return this->serialize;
}

} // namespace Tools
//------------------------------------------------------------------------------