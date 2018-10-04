//------------------------------------------------------------------------------
//  d3d11types.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"
#include "coregraphics/d3d11/d3d11types.h"

namespace Direct3D11
{
using namespace Base;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
DXGI_FORMAT
D3D11Types::AsD3D11PixelFormat(PixelFormat::Code p)
{
    switch (p)
    {

		case PixelFormat::X8R8G8B8:         return DXGI_FORMAT_R8G8B8A8_UNORM;        
		case PixelFormat::A8R8G8B8:         return DXGI_FORMAT_R8G8B8A8_UNORM;
		case PixelFormat::R5G6B5:           return DXGI_FORMAT_B5G6R5_UNORM;
		case PixelFormat::A1R5G5B5:         return DXGI_FORMAT_B5G5R5A1_UNORM;
		case PixelFormat::A4R4G4B4:         return DXGI_FORMAT_B8G8R8A8_UNORM;
		case PixelFormat::DXT1:             return DXGI_FORMAT_BC1_UNORM;
		case PixelFormat::DXT3:             return DXGI_FORMAT_BC2_UNORM;
		case PixelFormat::DXT5:             return DXGI_FORMAT_BC3_UNORM;
		case PixelFormat::R16F:             return DXGI_FORMAT_R16_FLOAT;
		case PixelFormat::G16R16F:          return DXGI_FORMAT_R16G16_FLOAT;
		case PixelFormat::A16B16G16R16F:    return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case PixelFormat::A16B16G16R16:     return DXGI_FORMAT_R16G16B16A16_UNORM;
		case PixelFormat::R32F:             return DXGI_FORMAT_R32_FLOAT;
		case PixelFormat::G32R32F:          return DXGI_FORMAT_R32G32_FLOAT;
		case PixelFormat::A32B32G32R32F:    return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case PixelFormat::R32G32B32F:		return DXGI_FORMAT_R32G32B32_FLOAT;
		case PixelFormat::R11G11B10F:		return DXGI_FORMAT_R11G11B10_FLOAT;
		case PixelFormat::A8:               return DXGI_FORMAT_A8_UNORM;
		case PixelFormat::A2R10G10B10:      return DXGI_FORMAT_R10G10B10A2_UNORM;
		case PixelFormat::G16R16:           return DXGI_FORMAT_R16G16_UNORM;
		case PixelFormat::D24X8:            return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case PixelFormat::D24S8:            return DXGI_FORMAT_D24_UNORM_S8_UINT;

        case PixelFormat::R8G8B8:           return DXGI_FORMAT_UNKNOWN;
        default:                            return DXGI_FORMAT_UNKNOWN;
    }
}

//------------------------------------------------------------------------------
/**
*/
PixelFormat::Code
D3D11Types::AsNebulaPixelFormat(DXGI_FORMAT f)
{
    switch (f)
    {
        case DXGI_FORMAT_B8G8R8X8_UNORM:			return PixelFormat::X8R8G8B8;
        case DXGI_FORMAT_R8G8B8A8_UNORM:			return PixelFormat::A8R8G8B8;
        case DXGI_FORMAT_B5G6R5_UNORM:				return PixelFormat::R5G6B5;
        case DXGI_FORMAT_B5G5R5A1_UNORM:			return PixelFormat::A1R5G5B5;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:         return PixelFormat::A4R4G4B4;
        case DXGI_FORMAT_BC1_UNORM:			        return PixelFormat::DXT1;
        case DXGI_FORMAT_BC2_UNORM:			        return PixelFormat::DXT3;
        case DXGI_FORMAT_BC3_UNORM:		            return PixelFormat::DXT5;		
        case DXGI_FORMAT_R16_FLOAT:					return PixelFormat::R16F;
        case DXGI_FORMAT_R16G16_FLOAT:				return PixelFormat::G16R16F;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:		return PixelFormat::A16B16G16R16F;
		case DXGI_FORMAT_R16G16B16A16_UNORM:		return PixelFormat::A16B16G16R16;
		case DXGI_FORMAT_R16G16B16A16_SNORM:		return PixelFormat::A16B16G16R16;
        case DXGI_FORMAT_R32_FLOAT:					return PixelFormat::R32F;
        case DXGI_FORMAT_R32G32_FLOAT:				return PixelFormat::G32R32F;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:		return PixelFormat::A32B32G32R32F;
        case DXGI_FORMAT_A8_UNORM:					return PixelFormat::A8;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:		return PixelFormat::A2R10G10B10;
		case DXGI_FORMAT_R10G10B10A2_UNORM:			return PixelFormat::A2R10G10B10;
        case DXGI_FORMAT_R16G16_TYPELESS:           return PixelFormat::G16R16;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:         return PixelFormat::D24S8;

        default:                        return PixelFormat::InvalidPixelFormat;
    }
}


//------------------------------------------------------------------------------
/**
*/
UINT 
D3D11Types::AsByteSize( DXGI_FORMAT f )
{
	switch (f)
	{
	case DXGI_FORMAT_B8G8R8X8_UNORM:			return 4;
	case DXGI_FORMAT_R8G8B8A8_UNORM:			return 4;
	case DXGI_FORMAT_B5G6R5_UNORM:				return 2;
	case DXGI_FORMAT_B5G5R5A1_UNORM:			return 2;
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:         return 2;
	case DXGI_FORMAT_BC1_UNORM:			        return 4;
	case DXGI_FORMAT_BC2_UNORM:			        return 4;
	case DXGI_FORMAT_BC3_UNORM:		            return 4;		
	case DXGI_FORMAT_R16_FLOAT:					return 2;
	case DXGI_FORMAT_R16G16_FLOAT:				return 4;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:		return 8;
	case DXGI_FORMAT_R16G16B16A16_UNORM:		return 8;
	case DXGI_FORMAT_R16G16B16A16_SNORM:		return 8;
	case DXGI_FORMAT_R32_FLOAT:					return 4;
	case DXGI_FORMAT_R32G32_FLOAT:				return 8;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:		return 16;
	case DXGI_FORMAT_A8_UNORM:					return 1;
	case DXGI_FORMAT_R10G10B10A2_TYPELESS:		return 4;
	case DXGI_FORMAT_R16G16_TYPELESS:           return 4;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:         return 4;

	default:                        return PixelFormat::InvalidPixelFormat;
	}
}

//------------------------------------------------------------------------------
/**
*/
DXGI_FORMAT
D3D11Types::AsD3D11VertexDeclarationType(VertexComponent::Format f)
{
    switch (f)
    {
        case VertexComponent::Float:    return DXGI_FORMAT_R32_FLOAT;
        case VertexComponent::Float2:   return DXGI_FORMAT_R32G32_FLOAT;
        case VertexComponent::Float3:   return DXGI_FORMAT_R32G32B32_FLOAT;
        case VertexComponent::Float4:   return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case VertexComponent::UByte4:   return DXGI_FORMAT_R8G8B8A8_UINT;
        case VertexComponent::Short2:   return DXGI_FORMAT_R16G16_SINT;
        case VertexComponent::Short4:   return DXGI_FORMAT_R16G16B16A16_SINT;
        case VertexComponent::UByte4N:  return DXGI_FORMAT_R8G8B8A8_UNORM;
        case VertexComponent::Short2N:  return DXGI_FORMAT_R16G16_SINT;
        case VertexComponent::Short4N:  return DXGI_FORMAT_R16G16B16A16_SINT;
        default:                        
            n_error("D3D11Types::AsDirect3DVertexDeclarationType(): invalid input parameter!");
            return DXGI_FORMAT_UNKNOWN;
    }
}

//------------------------------------------------------------------------------
/**
*/
D3DDECLUSAGE
D3D11Types::AsD3D11VertexDeclarationUsage(VertexComponent::SemanticName n)
{
    switch (n)
    {
        case VertexComponent::Position:     return D3DDECLUSAGE_POSITION;
        case VertexComponent::Normal:       return D3DDECLUSAGE_NORMAL;
        case VertexComponent::Tangent:      return D3DDECLUSAGE_TANGENT;
        case VertexComponent::Binormal:     return D3DDECLUSAGE_BINORMAL;
        case VertexComponent::TexCoord:     return D3DDECLUSAGE_TEXCOORD;
        case VertexComponent::SkinWeights:  return D3DDECLUSAGE_BLENDWEIGHT;
        case VertexComponent::SkinJIndices: return D3DDECLUSAGE_BLENDINDICES;
        case VertexComponent::Color:        return D3DDECLUSAGE_COLOR;
        default:
            n_error("D3D11Types::AsDirect3DVertexDeclarationUsage(): invalid input parameter!");
            return D3DDECLUSAGE_POSITION;
    }
}

//------------------------------------------------------------------------------
/**
*/
D3D11_PRIMITIVE_TOPOLOGY
D3D11Types::AsD3D11PrimitiveType(PrimitiveTopology::Code t)
{
    switch (t)
    {
        case PrimitiveTopology::PointList:      return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        case PrimitiveTopology::LineList:       return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case PrimitiveTopology::LineStrip:      return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case PrimitiveTopology::TriangleList:   return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PrimitiveTopology::TriangleStrip:  return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

        default:
            n_error("D3D11Types::AsDirect3DPrimitiveType(): unsupported topology '%s'!",
                PrimitiveTopology::ToString(t).AsCharPtr());
            return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

//------------------------------------------------------------------------------
/**
*/
D3D11_PRIMITIVE_TOPOLOGY 
D3D11Types::AsD3D11TessellateableType( D3D11_PRIMITIVE_TOPOLOGY t )
{
	switch (t)
	{
	case D3D11_PRIMITIVE_TOPOLOGY_POINTLIST:	return D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
	case D3D11_PRIMITIVE_TOPOLOGY_LINELIST:		return D3D11_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST;
	case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST:	return D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;

	default:
		n_error("D3D11Types::AsD3D11TessellateableType(): unsupported topology '%s'!",
			t);
		return t;
	}
}


//------------------------------------------------------------------------------
/**
*/
UINT
D3D11Types::AsD3D11MultiSampleType(AntiAliasQuality::Code c)
{
    switch (c)
    {
        case AntiAliasQuality::None:    
            return 1;
        case AntiAliasQuality::Low:     
            return 2;
        case AntiAliasQuality::Medium:
            return 4;    
        case AntiAliasQuality::High:    
            return 8;    
        
        default:
            n_error("D3D11Types::AsD3D11MultiSampleType(): unsupported AntiAliasQuality!");
            return 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
D3DX11_IMAGE_FILE_FORMAT
D3D11Types::AsD3DXImageFileFormat(ImageFileFormat::Code c)
{
    switch (c)
    {
        case ImageFileFormat::BMP:  return D3DX11_IFF_BMP;
        case ImageFileFormat::JPG:  return D3DX11_IFF_JPG;
        case ImageFileFormat::PNG:  return D3DX11_IFF_PNG;
        case ImageFileFormat::DDS:  return D3DX11_IFF_DDS;
        default:
            n_error("D3D11Types::AsD3DXImageFileFormat(): unsupported ImageFileFormat!");
            return D3DX11_IFF_BMP;
    }
}

//------------------------------------------------------------------------------
/**
*/
D3DPOOL
D3D11Types::AsD3D11Pool(ResourceBase::Usage usage, ResourceBase::Access access)
{
    switch (usage)
    {
        case ResourceBase::UsageImmutable:
            n_assert(ResourceBase::AccessNone == access);
            return D3DPOOL_MANAGED;

        case ResourceBase::UsageDynamic:
            n_assert(ResourceBase::AccessWrite == access);
            return D3DPOOL_DEFAULT;

        case ResourceBase::UsageCpu:
            return D3DPOOL_SYSTEMMEM;

        default:
            n_error("D3D11Util::AsD3D11Pool(): invalid usage parameter!");
            return D3DPOOL_SYSTEMMEM;
    }
}

//------------------------------------------------------------------------------
/**
*/
UINT
D3D11Types::AsD3D11Usage(ResourceBase::Usage usage)
{
    switch (usage)
    {
        case ResourceBase::UsageImmutable:
				return D3D11_USAGE_IMMUTABLE;

        case ResourceBase::UsageDynamic:
            return D3D11_USAGE_DYNAMIC;

        case ResourceBase::UsageCpu:
            return D3D11_USAGE_STAGING;

        default:
            n_error("D3D11Util::AsD3D11Usage(): invalid usage parameter!");
            return D3D11_USAGE_DEFAULT;
    }

}

//------------------------------------------------------------------------------
/**
*/
DXGI_FORMAT
D3D11Types::IndexTypeAsD3D11Format(IndexType::Code indexType)
{
    return (IndexType::Index16 == indexType) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
}

//------------------------------------------------------------------------------
/**
*/
Util::String D3D11Types::SemanticNameAsD3D11Format( UINT semantic, UINT semanticIndex )
{
	Util::String semanticString;
	switch (semantic)
	{
		case VertexComponentBase::Position:
			semanticString.Append("POSITION");
			break;
		case VertexComponentBase::Normal:
			semanticString.Append("NORMAL");
			break;
		case VertexComponentBase::Tangent:
			semanticString.Append("TANGENT");
			break;
		case VertexComponentBase::Binormal:
			semanticString.Append("BINORMAL");
			break;
		case VertexComponentBase::TexCoord:
			semanticString.Append("TEXCOORD");
			break;
		case VertexComponentBase::Color:
			semanticString.Append("COLOR");
			break;
		case VertexComponentBase::SkinWeights:
			semanticString.Append("BLENDWEIGHT");
			break;
		case VertexComponentBase::SkinJIndices:
			semanticString.Append("BLENDINDICES");
			break;
			
		default:
			n_error("D3D11Util::SemanticNameAsD3D11Format: invalid input semantic!");
			return "";
	}
	return semanticString;
	
}

//------------------------------------------------------------------------------
/**
*/
UINT D3D11Types::AsD3D11Access( Base::ResourceBase::Access access )
{
	switch (access)
	{
	case ResourceBase::AccessNone:
		return 0;
	case ResourceBase::AccessWrite:
		return D3D11_CPU_ACCESS_WRITE;
	case ResourceBase::AccessRead:
		return D3D11_CPU_ACCESS_READ;
	case ResourceBase::AccessReadWrite:
		return D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		
	default:
		n_error("D3D11Util::D3D11Types::AsD3D11Access: invalid input access!");
		return 0;
		
	}
}

//------------------------------------------------------------------------------
/**
*/
UINT D3D11Types::AsD3D11BufferType( Base::ResourceBase::Usage usage )
{
	switch ( usage )
	{
	case ResourceBase::UsageCpu:
		return D3D11_BIND_UNORDERED_ACCESS;
	default:
		return D3D11_BIND_VERTEX_BUFFER;
	}
}

//------------------------------------------------------------------------------
/**
*/
BOOL D3D11Types::CanCreateShaderResourceView( DXGI_FORMAT f )
{
	switch (f)
	{
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC5_UNORM:
		return TRUE;
	default:
		return FALSE;
	}
}

//------------------------------------------------------------------------------
/**
*/
UINT D3D11Types::AsByteSize( UINT semantic )
{

	switch (semantic)
	{
	case VertexComponentBase::Position:
		return sizeof ( D3DXVECTOR3 );
	case VertexComponentBase::Normal:
		return sizeof ( D3DXVECTOR3 );
	case VertexComponentBase::Tangent:
		return sizeof ( D3DXVECTOR3 );
	case VertexComponentBase::Binormal:
		return sizeof ( D3DXVECTOR3 );
	case VertexComponentBase::TexCoord:
		return sizeof ( D3DXVECTOR2 );
	case VertexComponentBase::Color:
		return sizeof ( DWORD );
	case VertexComponentBase::SkinWeights:
		return sizeof ( D3DXVECTOR4 );
	case VertexComponentBase::SkinJIndices:
		return sizeof ( D3DXVECTOR4 );
	default:
		n_error("Unknown vertex input semantic!");
		return 0;
	}
}

} // namespace Direct3D11
