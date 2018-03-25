//------------------------------------------------------------------------------
//  idlattribute.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "idlattribute.h"

namespace Tools
{
__ImplementClass(Tools::IDLAttribute, 'ILAT', Core::RefCounted);

using namespace IO;

//------------------------------------------------------------------------------
/**
*/
IDLAttribute::IDLAttribute() : 
hasDefault(false),system(false),hasDisplay(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
IDLAttribute::Parse(XmlReader* reader)
{
    n_assert(0 != reader);
    n_assert(reader->GetCurrentNodeName() == "Attribute");

    // parse attributes
    this->name = reader->GetString("name");
    if (reader->HasAttr("fourcc"))
    {
        this->fourcc = reader->GetString("fourcc");
    }
    if (reader->HasAttr("type"))
    {
        this->attrType = reader->GetString("type");
        if (!this->IsValidType(this->attrType))
        {
            Util::String fmt;
            fmt.Format("Invalid type %s for attribute %s", this->attrType.AsCharPtr(), this->name.AsCharPtr());
            this->SetError(fmt);
            return false;
        }
    }
    if (reader->HasAttr("accessMode"))
    {
        this->accessMode = reader->GetString("accessMode");
    }
    if (reader->HasAttr("system"))
    {
        this->system = reader->GetInt("system") != 0;
    }
    if (reader->HasAttr("default"))
    {
        this->hasDefault = true;
        this->defaultValue = reader->GetString("default");
    }
    if (reader->HasAttr("displayName"))
    {
        this->hasDisplay = true;
        this->displayName = reader->GetString("displayName");
    }
    Util::Array<Util::String> attrs = reader->GetAttrs();
    IndexT i;
    for (i = 0; i < attrs.Size(); i++)
    {
        this->attributes.Add(attrs[i], reader->GetString(attrs[i].AsCharPtr()));
    }

    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
IDLAttribute::IsValidType(const Util::String& str)
{
    if ((str == "String")
        || (str == "Float")
        || (str == "Float4")
        || (str == "Int")
        || (str == "Bool")
        || (str == "Matrix44")
        || (str == "Transform44")
        || (str == "Blob")
        || (str == "Guid"))
    {
        return true;
    }
    return false;
}

} // namespace Tools