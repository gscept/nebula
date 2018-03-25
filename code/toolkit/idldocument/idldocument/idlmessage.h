#pragma once
#ifndef TOOLS_IDLMESSAGE_H
#define TOOLS_IDLMESSAGE_H
//------------------------------------------------------------------------------
/**
    @class Tools::IDLMessage
    
    Wrap an IDL message definition.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "io/xmlreader.h"
#include "idldocument/idlarg.h"

//------------------------------------------------------------------------------
namespace Tools
{
class IDLMessage : public Core::RefCounted
{
    __DeclareClass(IDLMessage);
public:
    /// constructor
    IDLMessage();
    /// parse from XmlReader
    bool Parse(IO::XmlReader* reader);
    /// get error string
    const Util::String& GetError() const;
    /// get command name
    const Util::String& GetName() const;
    /// get fourcc string (optional if Mangalore message protocol)
    const Util::String& GetFourCC() const;
    /// get input arguments
    const Util::Array<Ptr<IDLArg>>& GetInputArgs() const;
    /// get output arguments
    const Util::Array<Ptr<IDLArg>>& GetOutputArgs() const;
    /// get parent class name
    const Util::String& GetParentClass() const;
	/// get description
	const Util::String& GetDescription() const;
	/// message should be scriptwrapped
	const bool WantsWrap() const;

private:
    /// set error string
    void SetError(const Util::String& e);

    Util::String error;
    Util::String name;
    Util::String fourcc;
	Util::String description;
    Util::Array<Ptr<IDLArg>> inArgs;
    Util::Array<Ptr<IDLArg>> outArgs;
    Util::String parentClass;
	bool wrap;
};

//------------------------------------------------------------------------------
/**
*/
inline void
IDLMessage::SetError(const Util::String& e)
{
    this->error = e;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLMessage::GetError() const
{
    return this->error;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLMessage::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLMessage::GetFourCC() const
{
    return this->fourcc;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<IDLArg>>&
IDLMessage::GetInputArgs() const
{
    return this->inArgs;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<IDLArg>>&
IDLMessage::GetOutputArgs() const
{
    return this->outArgs;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLMessage::GetParentClass() const
{
    return this->parentClass;
}
//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLMessage::GetDescription() const
{
	return this->description;
}
//------------------------------------------------------------------------------
/**
*/
inline const bool
IDLMessage::WantsWrap() const
{
	return this->wrap;
}
} // namespace Tools

//------------------------------------------------------------------------------
#endif
