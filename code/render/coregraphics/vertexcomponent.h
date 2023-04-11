#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::VertexComponent
  
    Describes a single vertex component in a vertex layout description.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#include "util/string.h"
namespace CoreGraphics
{

struct VertexLayoutId;
struct VertexLayoutCreateInfo;
const VertexLayoutId CreateVertexLayout(const VertexLayoutCreateInfo& info);

class VertexComponent
{
public:
     /// component format
    enum Format
    {
        Float,      //> one-component float
        Float2,     //> two-component float
        Float3,     //> three-component float
        Float4,     //> four-component float
        Half,       //> one-component 16-bit float
        Half2,      //> two-component 16-bit float
        Half3,      //> three-component 16-bit float
        Half4,      //> four-component 16-bit float
        UInt,       //> one-component unsigned integer
        UInt2,      //> two-component unsigned integer
        UInt3,      //> three-component unsigned integer
        UInt4,      //> four-component unsigned integer
        Int,        //> one-component unsigned integer
        Int2,       //> two-component unsigned integer
        Int3,       //> three-component unsigned integer
        Int4,       //> four-component unsigned integer
        Short,      //> one-component signed short
        Short2,     //> two-component signed short
        Short3,     //> three-component signed short
        Short4,     //> four-component signed short
        UShort,     //> one-component unsigned short
        UShort2,    //> two-component unsigned short
        UShort3,    //> three-component unsigned short
        UShort4,    //> four-component unsigned short

        UByte4,     //> four-component unsigned byte
        Byte4,      //> four-component signed byte
        UByte4N,    //> four-component normalized unsigned byte (value / 255.0f)
        Byte4N,     //> four-component normalized signed byte (value / 127.0f)

        Short2N,    //> two-component normalized signed short (value / 32767.0f)
        Short4N,    //> four-component normalized signed short (value / 32767.0f)
        UShort2N,   //> two-component unnormalized signed short
        UShort4N,   //> four-component unnormalized signed short


        InvalidFormat,
    };

    enum IndexName
    {
        Position = 0,
        Normal = 1,
        TexCoord1 = 2,
        Tangent = 3,
        Binormal = 4,
        Color = 5,
        TexCoord2 = 6,
        SkinWeights = 7,
        SkinJIndices = 8,
        TexCoord3 = 9,
        TexCoord4 = 10,

        Invalid,
    };

    /// stride type tells if the compoent should be per-instance or per-vertex
    enum StrideType
    {
        PerVertex,
        PerInstance
    };

    /// default constructor
    VertexComponent();
    /// constructor
    VertexComponent(IndexT slot, Format format, IndexT streamIndex = 0, StrideType strideType = PerVertex, SizeT stride = 0);

    /// get semantic index
    IndexT GetIndex() const;
    /// get vertex component format
    Format GetFormat() const;
    /// get stream index
    IndexT GetStreamIndex() const;
    /// get the byte size of the vertex component
    SizeT GetByteSize() const;
    /// get a unique signature of the vertex component
    Util::String GetSignature() const;
    /// get stride type
    StrideType GetStrideType() const;
    /// get stride between instances
    SizeT GetStride() const;
    /// convert string to format
    static Format StringToFormat(const Util::String& str);
    /// convert format to string
    static Util::String FormatToString(Format f);
    /// convert format to signature
    static Util::String FormatToSignature(Format f);
    /// get the byte offset of this component (only valid when part of a VertexLayout)
    IndexT GetByteOffset() const;

protected:

    friend const CoreGraphics::VertexLayoutId CoreGraphics::CreateVertexLayout(const CoreGraphics::VertexLayoutCreateInfo& info);

    /// set the vertex byte offset (called from VertexLayoutBase::Setup())
    void SetByteOffset(IndexT offset);

    IndexT index;
    Format format;
    StrideType strideType;
    SizeT stride;
    IndexT streamIndex;
    IndexT byteOffset;
};

//------------------------------------------------------------------------------
/**
*/
inline
VertexComponent::VertexComponent() :
    index(0),
    format(Float),
    streamIndex(0),
    byteOffset(0),
    strideType(PerVertex),
    stride(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
VertexComponent::VertexComponent(IndexT semIndex_, Format format_, IndexT streamIndex_, StrideType strideType_, SizeT stride_) :
    index(semIndex_),
    format(format_),
    streamIndex(streamIndex_),
    byteOffset(0),
    strideType(strideType_),
    stride(stride_)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
VertexComponent::GetIndex() const
{
    return this->index;
}

//------------------------------------------------------------------------------
/**
*/
inline VertexComponent::Format
VertexComponent::GetFormat() const
{
    return this->format;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
VertexComponent::GetStreamIndex() const
{
    return this->streamIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline VertexComponent::StrideType
VertexComponent::GetStrideType() const
{
    return this->strideType;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
VertexComponent::GetStride() const
{
    return this->stride;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
VertexComponent::GetByteSize() const
{
    switch (this->format)
    {
        case Float:     return 4;
        case Float2:    return 8;
        case Float3:    return 12;
        case Float4:    return 16;
        case Half:      return 2;
        case Half2:     return 4;
        case Half3:     return 6;
        case Half4:     return 8;
        case UInt:      return 4;
        case UInt2:     return 8;
        case UInt3:     return 12;
        case UInt4:     return 16;
        case Int:       return 4;
        case Int2:      return 8;
        case Int3:      return 12;
        case Int4:      return 16;
        case Short:     return 2;
        case Short2:    return 4;
        case Short3:    return 6;
        case Short4:    return 8;
        case UShort:    return 2;
        case UShort2:   return 4;
        case UShort3:   return 6;
        case UShort4:   return 8;

        case UByte4:    return 4;
        case Byte4:     return 4;
        case UByte4N:   return 4;
        case Byte4N:    return 4;
        case UShort2N:  return 4;
        case UShort4N:  return 8;
        case Short2N:   return 4;
        case Short4N:   return 8;
    }
    n_error("Can't happen");
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
inline Util::String
VertexComponent::FormatToString(Format f)
{
    switch (f)
    {
        case Float:     return "Float";
        case Float2:    return "Float2";
        case Float3:    return "Float3";
        case Float4:    return "Float4";
        case Half:      return "Half";
        case Half2:     return "Half2";
        case Half3:     return "Half3";
        case Half4:     return "Half4";
        case UInt:      return "UInt";
        case UInt2:     return "UInt2";
        case UInt3:     return "UInt3";
        case UInt4:     return "UInt4";
        case Int:       return "Int";
        case Int2:      return "Int2";
        case Int3:      return "Int3";
        case Int4:      return "Int4";
        case Short:     return "Short";
        case Short2:    return "Short2";
        case Short3:    return "Short3";
        case Short4:    return "Short4";
        case UShort:    return "UShort";
        case UShort2:   return "UShort2";
        case UShort3:   return "UShort3";
        case UShort4:   return "UShort4";

        case UByte4:    return "UByte4";
        case Byte4:     return "Byte4";
        case UByte4N:   return "UByte4N";
        case Byte4N:    return "Byte4N";
        case UShort2N:  return "UShort2N";
        case UShort4N:  return "UShort4N";
        case Short2N:   return "Short2N";
        case Short4N:   return "Short4N";

        default:
            n_error("VertexComponent::FormatToString(): invalid Format code!");
            return "";
    }
}

//------------------------------------------------------------------------------
/**
*/
inline Util::String
VertexComponent::FormatToSignature(Format f)
{
    switch (f)
    {
        case Float:     return "f";
        case Float2:    return "f2";
        case Float3:    return "f3";
        case Float4:    return "f4";
        case Half:      return "h";
        case Half2:     return "h2";
        case Half3:     return "h3";
        case Half4:     return "h4";
        case UInt:      return "ui";
        case UInt2:     return "ui2";
        case UInt3:     return "ui3";
        case UInt4:     return "ui4";
        case Int:       return "i";
        case Int2:      return "i2";
        case Int3:      return "i3";
        case Int4:      return "i4";
        case Short:     return "s";
        case Short2:    return "s2";
        case Short3:    return "s3";
        case Short4:    return "s4";
        case UShort:    return "us";
        case UShort2:   return "us2";
        case UShort3:   return "us3";
        case UShort4:   return "s4";

        case UByte4:    return "ub4";
        case Byte4:     return "b4";
        case UByte4N:   return "ub4n";
        case Byte4N:    return "b4n";
        case UShort2N:   return "us2n";
        case UShort4N:   return "us4n";
        case Short2N:   return "s2n";
        case Short4N:   return "s4n";


        default:
            n_error("VertexComponent::FormatToString(): invalid Format code!");
            return "";
    }
}

//------------------------------------------------------------------------------
/**
*/
inline VertexComponent::Format
VertexComponent::StringToFormat(const Util::String& str)
{
    if (str == "Float") return Float;
    else if (str == "Float2") return Float2;
    else if (str == "Float3") return Float3;
    else if (str == "Float4") return Float4;
    else if (str == "Half") return Half;
    else if (str == "Half2") return Half2;
    else if (str == "Half3") return Half3;
    else if (str == "Half4") return Half4;
    else if (str == "UInt") return UInt;
    else if (str == "UInt2") return UInt2;
    else if (str == "UInt3") return UInt3;
    else if (str == "UInt4") return UInt4;
    else if (str == "Int") return Int;
    else if (str == "Int2") return Int2;
    else if (str == "Int3") return Int3;
    else if (str == "Int4") return Int4;
    else if (str == "Short") return Short;
    else if (str == "Short2") return Short2;
    else if (str == "Short3") return Short3;
    else if (str == "Short4") return Short4;
    else if (str == "UShort") return UShort;
    else if (str == "UShort2") return UShort2;
    else if (str == "UShort3") return UShort3;
    else if (str == "UShort4") return UShort4;

    else if (str == "UByte4") return UByte4;
    else if (str == "Byte4") return Byte4;
    else if (str == "UByte4N") return UByte4N;
    else if (str == "Byte4N") return Byte4N;
    else if (str == "UShort2N") return UShort2N;
    else if (str == "UShort4N") return UShort4N;
    else if (str == "Short2N") return Short2N;
    else if (str == "Short4N") return Short4N;

    else
    {
        n_error("VertexComponent::StringToFormat(): invalid string '%s'!\n", str.AsCharPtr());
        return Float;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline Util::String
VertexComponent::GetSignature() const
{
    Util::String str;
    str.AppendInt(this->streamIndex);
    str.Append(FormatToSignature(this->format));
    return str;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VertexComponent::SetByteOffset(IndexT offset)
{
    this->byteOffset = offset;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
VertexComponent::GetByteOffset() const
{
    return this->byteOffset;
}

} // namespace CoreGraphics

