#pragma once
//------------------------------------------------------------------------------
/**
    @file naxfileformatstructs.h
    
    NAX file format structures.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "math/quaternion.h"

namespace Characters
{
#pragma pack(push, 1)

#define NEBULA_NSK3_MAGICNUMBER 'NS01'

//------------------------------------------------------------------------------
/** 
    NSK3 file format structs.

    NOTE: keep all header-structs 4-byte aligned!
*/

struct Nsk3Header
{
	uint magic;
	uint numJoints;
};

struct Nsk3Joint
{
	Util::String name;
	Math::float4 translation;
	Math::quaternion rotation;
	Math::float4 scale;
	int parent;
	int index;
};

#pragma pack(pop)
} // namespace Characters   