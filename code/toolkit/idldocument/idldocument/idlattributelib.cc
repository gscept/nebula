//------------------------------------------------------------------------------
//  idlattributelib.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "idlattributelib.h"

namespace Tools
{
__ImplementClass(Tools::IDLAttributeLib, 'ILAL', Core::RefCounted);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
IDLAttributeLib::IDLAttributeLib() 
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool
IDLAttributeLib::Parse(XmlReader* reader)
{
    n_assert(0 != reader);
    n_assert(reader->GetCurrentNodeName() == "AttributeLib");

    // parse dependency definitions
    if (reader->SetToFirstChild("Dependency")) do
    {
        Ptr<IDLDependency> dep = IDLDependency::Create();
        if (!dep->Parse(reader))
        {
            this->SetError(dep->GetError());
            return false;
        }
        this->dependencies.Append(dep);
    }
    while (reader->SetToNextChild("Dependency"));

    // parse attribute definitions
    if (reader->SetToFirstChild("Attribute")) do
    {
        Ptr<IDLAttribute> attr = IDLAttribute::Create();
        if (!attr->Parse(reader))
        {
            this->SetError(attr->GetError());
            return false;
        }attributes.Append(attr);
    }
    while (reader->SetToNextChild("Attribute"));
    return true;
}

} // namespace Tools