//------------------------------------------------------------------------------
//  typeregistry.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "attributeregistry.h"
namespace MemDb
{

AttributeRegistry* AttributeRegistry::Singleton = 0;

//------------------------------------------------------------------------------
/**
    This creates a singleton if needed, unlike the macro
*/
AttributeRegistry*
AttributeRegistry::Instance()
{
    if (0 == Singleton)
    {
        Singleton = new AttributeRegistry;
        n_assert(0 != Singleton);
    }
    return Singleton;
}

//------------------------------------------------------------------------------
/**
*/
bool
AttributeRegistry::HasInstance()
{
    return Singleton != nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
AttributeRegistry::Destroy()
{
    if (0 != Singleton)
    {
        delete Singleton;
        Singleton = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
AttributeRegistry::AttributeRegistry()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AttributeRegistry::~AttributeRegistry()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Do not use. This function generates a new attribute id.
*/
uint16_t
GenerateNewAttributeId()
{
    static uint16_t idCounter = -1;
    return ++idCounter;
}

} // namespace MemDb
