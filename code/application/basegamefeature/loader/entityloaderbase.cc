//------------------------------------------------------------------------------
//  loader/entityloaderbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "entityloaderbase.h"

namespace BaseGameFeature
{

__ImplementClass(BaseGameFeature::EntityLoaderBase, 'elbs', Core::RefCounted);

bool EntityLoaderBase::insideLoading = false;

//------------------------------------------------------------------------------
/**
*/
EntityLoaderBase::EntityLoaderBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
EntityLoaderBase::~EntityLoaderBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
EntityLoaderBase::Load(const Util::String& file)
{    
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool 
EntityLoaderBase::IsLoading()
{
    return insideLoading;
}
} // namespace BaseGameFeature


