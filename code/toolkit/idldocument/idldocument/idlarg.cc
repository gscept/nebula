//------------------------------------------------------------------------------
//  idlarg.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "idlarg.h"  

namespace Tools
{
__ImplementClass(Tools::IDLArg, 'ILAG', Core::RefCounted);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
IDLArg::IDLArg()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
IDLArg::Parse(XmlReader* reader)
{
    n_assert(0 != reader);
    this->name = reader->GetString("name");
    this->type = reader->GetString("type");
    this->defaultValue = reader->GetOptString("default", "");
    this->wrappingType = reader->GetOptString("wrappedType","");
    this->serialize = reader->GetOptBool("serialize", false);
    if (reader->SetToFirstChild("Encode"))
    {
        this->encodeCode = reader->GetContent();
        reader->SetToParent();
    }
    if (reader->SetToFirstChild("Decode"))
    {
        this->decodeCode = reader->GetContent();
        reader->SetToParent();
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
IDLArg::CreateFromArgs(const Util::String & iname, const Util::String & itype, const Util::String & idefaultValue, const Util::String & iwrappingType)
{
    this->name = iname;
    this->type = itype;
    this->defaultValue = idefaultValue;
    this->serialize = false;
    this->wrappingType = iwrappingType;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
IDLArg::IsValidType(const Util::String& str)
{
    if ((str == "Char")
        ||(str == "String")
        ||(str == "Float")
        ||(str == "Float2")
        ||(str == "Vector")
        ||(str == "Float4")
        ||(str == "Int")
        ||(str == "UInt")
        ||(str == "Int64")
        ||(str == "UInt64")
        ||(str == "Bool")
        ||(str == "Matrix44")
        ||(str == "Transform44")
        ||(str == "Point")
        ||(str == "Vector")
        ||(str == "Double")
        ||(str == "Short")
        ||(str == "UShort")
        ||(str == "Blob")
        ||(str == "Guid")
        ||(str == "Ptr<Game::Entity>")
		||(str == "StringAtom"))
    {
        return true;
    }   

    return false;
}

} // namespace Tools
