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
    The registry's constructor is called by the Instance() method, and
    nobody else.
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
    This static method is used to destroy the registry object and should be
    called right before the main function exits. It will make sure that
    no accidential memory leaks are reported by the debug heap.
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
