#pragma once
//------------------------------------------------------------------------------
/**
	@class Util::Bit
	
	Implements helper functions for checking bits
	
	(C) 2009 Radon Labs GmbH
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

namespace Util
{

//------------------------------------------------------------------------------
/**
*/
constexpr uint64 
SetBit(uint64 mask, uint8 bit)
{
	return mask | (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr bool
HasBit(uint64 mask, uint8 bit)
{
	return mask & (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr uint32
SetBit(uint32 mask, uint8 bit)
{
	return mask | (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr bool
HasBit(uint32 mask, uint8 bit)
{
	return mask & (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr uint16
SetBit(uint16 mask, uint8 bit)
{
	return mask | (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr bool
HasBit(uint16 mask, uint8 bit)
{
	return mask & (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr int64
SetBit(int64 mask, uint8 bit)
{
	return mask | (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr bool
HasBit(int64 mask, uint8 bit)
{
	return mask & (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr int32
SetBit(int32 mask, uint8 bit)
{
	return mask | (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr bool
HasBit(int32 mask, uint8 bit)
{
	return mask & (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr int16
SetBit(int16 mask, uint8 bit)
{
	return mask | (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr bool
HasBit(int16 mask, uint8 bit)
{
	return mask & (1 << bit);
}

} // namespace Util

	