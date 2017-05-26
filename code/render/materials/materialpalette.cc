//------------------------------------------------------------------------------
//  materialpalette.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "materials/materialpalette.h"

namespace Materials
{
__ImplementClass(Materials::MaterialPalette, 'MTRP', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
MaterialPalette::MaterialPalette()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
MaterialPalette::~MaterialPalette()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialPalette::Discard()
{
	for (int i = 0; i < this->materials.Size(); i++)
	{
		this->materials[i]->Discard();
	}
	this->materials.Clear();
	this->materialsByName.Clear();
}

} // namespace Materials
