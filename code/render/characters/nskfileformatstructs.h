#pragma once
//------------------------------------------------------------------------------
/**
    @file naxfileformatstructs.h
    
    NAX file format structures.
    
    @copyright
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "math/quat.h"

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
    uint numSkeletons;
};

struct Nsk3Skeleton
{
    uint numJoints;
};

struct Nsk3Joint
{
    Util::String name;
    float bind[16];
    float rotation[4];
    float translation[3];
    float scale[3];
    int parent;
    int index;
};

#pragma pack(pop)
} // namespace Characters   
