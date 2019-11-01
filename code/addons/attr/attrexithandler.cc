//------------------------------------------------------------------------------
//  attrexithandler.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "attrexithandler.h"
#include "attributedefinitionbase.h"

namespace Attr
{

//------------------------------------------------------------------------------
/**
*/
void
AttrExitHandler::OnExit() const
{
    // cleanup dynamic attributes
    AttributeDefinitionBase::ClearDynamicAttributes();
}

} // namespace Attr
