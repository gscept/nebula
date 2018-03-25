#pragma once
#ifndef TOOLS_IDLCOMMAND_H
#define TOOLS_IDLCOMMAND_H
//------------------------------------------------------------------------------
/**
    @class Tools::IDLCommand
    
    Wrap an IDL command definition.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "io/xmlreader.h"
#include "idldocument/idlarg.h"
#include "idldocument/idlmessage.h"

//------------------------------------------------------------------------------
namespace Tools
{
class IDLCommand : public Core::RefCounted
{
    __DeclareClass(IDLCommand);
public:
    /// constructor
    IDLCommand();
    /// parse from XmlReader
    bool Parse(IO::XmlReader* reader);
	/// create from wrapping a message
	bool WrapMessage(const Ptr<IDLMessage> & msg, const Util::String & mnamespace);
    /// get error string
    const Util::String& GetError() const;
    /// get command name
    const Util::String& GetName() const;
    /// get fourcc string
    const Util::String& GetFourCC() const;
    /// get command description
    const Util::String& GetDesc() const;
    /// get input arguments
    const Util::Array<Ptr<IDLArg>>& GetInputArgs() const;
    /// get output arguments
    const Util::Array<Ptr<IDLArg>>& GetOutputArgs() const;
    /// get code fragment
    const Util::String& GetCode() const;	

private:
    /// set error string
    void SetError(const Util::String& e);

    Util::String error;
    Util::String name;
    Util::String fourcc;
    Util::String desc;
    Util::String code;
    Util::Array<Ptr<IDLArg>> inArgs;
    Util::Array<Ptr<IDLArg>> outArgs;	
};

//------------------------------------------------------------------------------
/**
*/
inline void
IDLCommand::SetError(const Util::String& e)
{
    this->error = e;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLCommand::GetError() const
{
    return this->error;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLCommand::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLCommand::GetFourCC() const
{
    return this->fourcc;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLCommand::GetDesc() const
{
    return this->desc;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
IDLCommand::GetCode() const
{
    return this->code;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<IDLArg>>&
IDLCommand::GetInputArgs() const
{
    return this->inArgs;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<IDLArg>>&
IDLCommand::GetOutputArgs() const
{
    return this->outArgs;
}

} // namespace Tools
//------------------------------------------------------------------------------
#endif
