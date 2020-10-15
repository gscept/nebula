//------------------------------------------------------------------------------
//  typeregistry.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "typeregistry.h"
namespace MemDb
{

TypeRegistry* TypeRegistry::Singleton = 0;

//------------------------------------------------------------------------------
/**
	The registry's constructor is called by the Instance() method, and
	nobody else.
*/
TypeRegistry*
TypeRegistry::Instance()
{
	if (0 == Singleton)
	{
		Singleton = n_new(TypeRegistry);
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
TypeRegistry::Destroy()
{
	if (0 != Singleton)
	{
		n_delete(Singleton);
		Singleton = 0;
	}
}

//------------------------------------------------------------------------------
/**
*/
TypeRegistry::TypeRegistry()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
TypeRegistry::~TypeRegistry()
{
	// empty
}

} // namespace MemDb
