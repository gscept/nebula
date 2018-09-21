//------------------------------------------------------------------------------
//  emitterattrs.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "particles/emitterattrs.h"

namespace Particles
{

//------------------------------------------------------------------------------
/**
*/
EmitterAttrs::EmitterAttrs()
{
    Memory::Clear(this->floatValues, sizeof(this->floatValues));
    Memory::Clear(&this->intAttributes, sizeof(this->intAttributes));
	Memory::Clear(&this->boolAttributes, sizeof(this->boolAttributes));
	Memory::Clear(this->float4Values, sizeof(this->float4Values));
}


} // namespace Particles