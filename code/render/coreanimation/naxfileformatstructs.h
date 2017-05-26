#pragma once
//------------------------------------------------------------------------------
/**
    @file naxfileformatstructs.h
    
    NAX file format structures.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

namespace CoreAnimation
{
#pragma pack(push, 1)

#define NEBULA3_NAX3_MAGICNUMBER 'NA01'

//------------------------------------------------------------------------------
/** 
    NAX3 file format structs.

    NOTE: keep all header-structs 4-byte aligned!
*/
struct Nax3Header
{
    uint magic;
    uint numClips;
    uint numKeys;
};

struct Nax3Clip
{
    ushort numCurves;
    ushort startKeyIndex;
    ushort numKeys;
    ushort keyStride;
    ushort keyDuration; 
    uchar preInfinityType;          // CoreAnimation::InfinityType::Code
    uchar postInfinityType;         // CoreAnimation::InfinityType::Code
    ushort numEvents;
    char name[50];                  // add up to 64 bytes size for Nax3Clip
};

struct Nax3AnimEvent
{
    char name[47];
    char category[15];
    ushort keyIndex;
};

struct Nax3Curve
{
    uint firstKeyIndex;
    uchar isActive;                 // 0 or 1
    uchar isStatic;                 // 0 or 1
    uchar curveType;                // CoreAnimation::CurveType::Code
    uchar _padding;                 // padding byte
    float staticKeyX;
    float staticKeyY;
    float staticKeyZ;
    float staticKeyW;
};

//------------------------------------------------------------------------------
/** 
    legacy NAX2 file format structs
*/    
struct Nax2Header
{
    uint magic;
    SizeT numGroups;
    SizeT numKeys;
};

struct Nax2Group
{
    SizeT numCurves;
    IndexT startKey;
    SizeT numKeys;
    SizeT keyStride;
    float keyTime;
    float fadeInFrames;
    int loopType;
    char metaData[512];
};

struct Nax2Curve
{
    int ipolType;
    int firstKeyIndex;
    int isAnim;
    float keyX;
    float keyY;
    float keyZ;
    float keyW;
};

#pragma pack(pop)
} // namespace CoreAnimation   