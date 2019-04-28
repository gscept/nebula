#pragma once
//------------------------------------------------------------------------------
/**
    @class BaseGameFeature::EntityLoaderBase
    
    Abstract loader helper for game entities. 
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "util/string.h"
#include "core/refcounted.h"

//------------------------------------------------------------------------------
namespace BaseGameFeature
{
class EntityLoaderBase : public Core::RefCounted
{
	__DeclareClass(EntityLoaderBase);
public:
    /// construtcor
    EntityLoaderBase();
    /// destructor
    ~EntityLoaderBase();
    /// load entity objects into the level
    virtual bool Load(const Util::String& file);
    /// is loader currently inside Load Function
    static bool IsLoading();

protected:
    static bool insideLoading;
};

} // namespace BaseGameFeature
//------------------------------------------------------------------------------
