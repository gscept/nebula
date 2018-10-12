#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::IndexType
    
    Data type of vertex indices (16 bit or 32 bit).
   
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
class IndexType
{
public:
    /// index types enum
    enum Code
    {
        None,           // index type not defined
        Index16,        // 16 bit indices
        Index32,        // 32 bit indices
    };

    /// get byte size of index
    static SizeT SizeOf(IndexType::Code type);
    /// convert index type to string
    static Util::String ToString(IndexType::Code type);
    /// convert string to index type
    static IndexType::Code FromString(const Util::String& str);
};

//------------------------------------------------------------------------------
/**
*/
inline SizeT
IndexType::SizeOf(IndexType::Code type)
{
    n_assert(type != None);
    switch (type)
    {
        case Index16:   return 2;
        case Index32:   return 4;
        default:        return 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline Util::String
IndexType::ToString(IndexType::Code type)
{
    switch (type)
    {
        case None:      return "None";
        case Index16:   return "Index16";
        case Index32:   return "Index32";
        default:
            n_error("IndexType::ToString(): Invalid IndexType code!");
            return "";
    }
}

//------------------------------------------------------------------------------
/**
*/
inline IndexType::Code
IndexType::FromString(const Util::String& str)
{
    if ("None" == str) return None;
    else if ("Index16" == str) return Index16;
    else if ("Index32" == str) return Index32;
    else
    {
        n_error("IndexType::FromString(): invalid index type string '%s'!", str.AsCharPtr());
        return None;
    }
}

}
//------------------------------------------------------------------------------
