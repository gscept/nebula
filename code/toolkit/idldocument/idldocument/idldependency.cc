//------------------------------------------------------------------------------
//  idldependency.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "idldependency.h"

namespace Tools
{
__ImplementClass(Tools::IDLDependency, 'ILDP', Core::RefCounted);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
IDLDependency::IDLDependency()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
IDLDependency::CreateFromString(const Util::String & iheader)
{
	this->header = iheader;
}


//------------------------------------------------------------------------------
/**
*/
bool
IDLDependency::Parse(XmlReader* reader)
{
    n_assert(0 != reader);
    n_assert(reader->GetCurrentNodeName() == "Dependency");
    this->header = reader->GetString("header");
    return true;
}

} // namespace Tools