#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::VertexLayoutBase
    
    Base class for platform-specific vertex component subclasses. This
    allows subclasses to add platform-specific information to vertex
    components.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
struct VertexLayoutId;
struct VertexLayoutCreateInfo;
const VertexLayoutId CreateVertexLayout(const VertexLayoutCreateInfo& info);
}

namespace Base
{
class VertexComponentBase
{
public:
    /// component semantic
    enum SemanticName
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

        Invalid,
    };

    /// component format
    enum Format
    {
        Float,      //> one-component float
        Float2,     //> two-component float
        Float3,     //> three-component float
        Float4,     //> four-component float
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
        Byte4,		//> four-component signed byte
        UByte4N,    //> four-component normalized unsigned byte (value / 255.0f)
		Byte4N,		//> four-component normalized signed byte (value / 127.0f)

        Short2N,    //> two-component normalized signed short (value / 32767.0f)
        Short4N,    //> four-component normalized signed short (value / 32767.0f)
		UShort2N,	//> two-component unnormalized signed short
		UShort4N,	//> four-component unnormalized signed short


        InvalidFormat,
    };
    
    /// access type hint, this is only relevant on the Wii
    enum AccessType
    {
        None,
        Direct,     //> component has direct value (non-indexed)
        Index8,     //> component is indexed with 8-bit indices           
        Index16,    //> component is indexed with 16-bit indices
        Index32,    //> component is indexed with 32-bit indices
    };

	/// stride type tells if the compoent should be per-instance or per-vertex
	enum StrideType
	{
		PerVertex,
		PerInstance
	};

    /// default constructor
    VertexComponentBase();
    /// constructor
    VertexComponentBase(SemanticName semName, IndexT semIndex, Format format, IndexT streamIndex=0, StrideType strideType=PerVertex, SizeT stride=0);
    /// get semantic name
    SemanticName GetSemanticName() const;
    /// get semantic index
    IndexT GetSemanticIndex() const;
    /// get vertex component format
    Format GetFormat() const;
    /// get stream index
    IndexT GetStreamIndex() const;
    /// get the byte size of the vertex component
    SizeT GetByteSize() const;
    /// get a unique signature of the vertex component
    Util::String GetSignature() const;
    /// get access type
    AccessType GetAccessType() const;
	/// get stride type
	StrideType GetStrideType() const;
	/// get stride between instances
	SizeT GetStride() const;
    /// convert string to semantic name
    static SemanticName StringToSemanticName(const Util::String& str);
    /// convert semantic name to string
    static Util::String SemanticNameToString(SemanticName n);
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

    SemanticName semName;
    IndexT semIndex;
    Format format;
    AccessType accessType;
	StrideType strideType;
	SizeT stride;
    IndexT streamIndex;
    IndexT byteOffset;
};

//------------------------------------------------------------------------------
/**
*/
inline
VertexComponentBase::VertexComponentBase() :
    semName(Invalid),
    semIndex(0),
    format(Float),
    accessType(Index16),
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
VertexComponentBase::VertexComponentBase(SemanticName semName_, IndexT semIndex_, Format format_, IndexT streamIndex_, StrideType strideType_, SizeT stride_) :
    semName(semName_),
    semIndex(semIndex_),
    format(format_),
    accessType(Index16),
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
inline VertexComponentBase::SemanticName
VertexComponentBase::GetSemanticName() const
{
    return this->semName;
}

//------------------------------------------------------------------------------
/**
*/
inline VertexComponentBase::AccessType
VertexComponentBase::GetAccessType() const
{
    return this->accessType;     
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
VertexComponentBase::GetSemanticIndex() const
{
    return this->semIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline VertexComponentBase::Format
VertexComponentBase::GetFormat() const
{
    return this->format;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
VertexComponentBase::GetStreamIndex() const
{
    return this->streamIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline VertexComponentBase::StrideType
VertexComponentBase::GetStrideType() const
{
	return this->strideType;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
VertexComponentBase::GetStride() const
{
	return this->stride;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
VertexComponentBase::GetByteSize() const
{
    switch (this->format)
    {
        case Float:     return 4;
        case Float2:    return 8;
        case Float3:    return 12;
        case Float4:    return 16;
        case UInt:		return 4;
        case UInt2:	    return 8;
        case UInt3:	    return 12;
        case UInt4:	    return 16;
        case Int:		return 4;
        case Int2:		return 8;
        case Int3:		return 12;
        case Int4:		return 16;
        case Short:     return 2;
        case Short2:    return 4;
        case Short3:    return 6;
        case Short4:    return 8;
        case UShort:    return 2;
        case UShort2:   return 4;
        case UShort3:   return 6;
        case UShort4:   return 8;

        case UByte4:    return 4;
		case Byte4:		return 4;
        case UByte4N:   return 4;
		case Byte4N:	return 4;
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
VertexComponentBase::SemanticNameToString(SemanticName n)
{
    switch (n)
    {
        case Position:      return "Position";
        case Normal:        return "Normal";
        case Tangent:       return "Tangent";
        case Binormal:      return "Binormal";
        case TexCoord1:     return "TexCoord";
        case Color:         return "Color";
        case SkinWeights:   return "SkinWeights";
        case SkinJIndices:  return "SkinJIndices";
        default:
            n_error("VertexComponent::SemanticNameToString(): invalid SemanticName code!");
            return "";
    }
}

//------------------------------------------------------------------------------
/**
*/
inline VertexComponentBase::SemanticName
VertexComponentBase::StringToSemanticName(const Util::String& str)
{
    if (str == "Position") return Position;
    else if (str == "Normal") return Normal;
    else if (str == "Tangent") return Tangent;
    else if (str == "Binormal") return Binormal;
    else if (str == "TexCoord") return TexCoord1;
    else if (str == "Color") return Color;
    else if (str == "SkinWeights") return SkinWeights;
    else if (str == "SkinJIndices") return SkinJIndices;
    else
    {
        n_error("VertexComponent::StringToSemanticName(): invalid string '%s'!", str.AsCharPtr());
        return Invalid;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline Util::String
VertexComponentBase::FormatToString(Format f)
{
    switch (f)
    {
        case Float:     return "Float";
        case Float2:    return "Float2";
        case Float3:    return "Float3";
        case Float4:    return "Float4";
        case UInt:		return "UInt";
        case UInt2:	    return "UInt2";
        case UInt3:	    return "UInt3";
        case UInt4:	    return "UInt4";
        case Int:		return "Int";
        case Int2:		return "Int2";
        case Int3:		return "Int3";
        case Int4:		return "Int4";
        case Short:     return "Short";
        case Short2:    return "Short2";
        case Short3:    return "Short3";
        case Short4:    return "Short4";
        case UShort:    return "UShort";
        case UShort2:   return "UShort2";
        case UShort3:   return "UShort3";
        case UShort4:   return "UShort4";

        case UByte4:    return "UByte4";
		case Byte4:		return "Byte4";
        case UByte4N:   return "UByte4N";
		case Byte4N:	return "Byte4N";
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
VertexComponentBase::FormatToSignature(Format f)
{
	switch (f)
	{
	case Float:     return "f";
	case Float2:    return "f2";
	case Float3:    return "f3";
	case Float4:    return "f4";
    case UInt:		return "ui";
    case UInt2:	    return "ui2";
    case UInt3:	    return "ui3";
    case UInt4:	    return "ui4";
    case Int:		return "i";
    case Int2:		return "i2";
    case Int3:		return "i3";
    case Int4:		return "i4";
    case Short:     return "s";
    case Short2:    return "s2";
    case Short3:    return "s3";
    case Short4:    return "s4";
    case UShort:    return "us";
    case UShort2:   return "us2";
    case UShort3:   return "us3";
    case UShort4:   return "s4";

	case UByte4:    return "ub4";
	case Byte4:		return "b4";
    case UByte4N:   return "ub4n";
    case Byte4N:	return "b4n";
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
inline VertexComponentBase::Format
VertexComponentBase::StringToFormat(const Util::String& str)
{
    if (str == "Float") return Float;
    else if (str == "Float2") return Float2;
    else if (str == "Float3") return Float3;
    else if (str == "Float4") return Float4;
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
VertexComponentBase::GetSignature() const
{
    Util::String str;
    switch (this->semName)
    {
        case Position:      str = "pos"; break;
        case Normal:        str = "nrm"; break;
        case Tangent:       str = "tan"; break;
        case Binormal:      str = "bin"; break;
        case TexCoord1:     str = "tex"; break;
        case Color:         str = "clr"; break;
		case TexCoord2:		str = "lgh"; break;
        case SkinWeights:   str = "skw"; break;
        case SkinJIndices:  str = "sji"; break;
        default:
            str = Util::String::Sprintf("cst%d", this->semName);
            break;
    }
    str.AppendInt(this->semIndex);    
    str.Append(FormatToSignature(this->format));
    return str;
}

//------------------------------------------------------------------------------
/**
*/
inline void
VertexComponentBase::SetByteOffset(IndexT offset)
{
    this->byteOffset = offset;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
VertexComponentBase::GetByteOffset() const
{
    return this->byteOffset;
}

} // namespace Base
//------------------------------------------------------------------------------

