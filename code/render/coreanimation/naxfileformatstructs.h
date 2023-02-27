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

namespace CoreAnimation
{
#pragma pack(push, 1)

#define NEBULA_NAX3_MAGICNUMBER 'NA01'

//------------------------------------------------------------------------------
/** 
    NAX3 file format structs.

    NOTE: keep all header-structs 4-byte aligned!
*/
struct Nax3Header
{
    uint magic;
    ushort numAnimations;
};

struct Nax3Anim
{
    ushort numClips;
    ushort numCurves;
    ushort numEvents;
    uint numKeys;
    uint numIntervals;
};

struct Nax3Interval
{
    uint start, end;
    uint key0, key1;
    float duration;
};

struct Nax3Clip
{
    ushort numCurves;
    ushort firstCurve;
    ushort numEvents;
    ushort firstEvent;
    ushort numVelocityCurves;
    ushort firstVelocityCurve;

    uint duration;

    char name[48];                  // add up to 64 bytes size for Nax3Clip
};

struct Nax3AnimEvent
{
    char name[47];
    char category[15];
    ushort keyIndex;
};

struct Nax3Curve
{
    uint firstIntervalOffset;
    uint numIntervals;
    uchar preInfinityType;          // CoreAnimation::InfinityType::Code
    uchar postInfinityType;         // CoreAnimation::InfinityType::Code
    uchar curveType;                // CoreAnimation::CurveType::Code
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
