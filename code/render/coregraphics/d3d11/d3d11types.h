#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11Types
    
    Provides static helper functions to convert from and to Direct3D
    data types and enumerations.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "coregraphics/pixelformat.h"
#include "coregraphics/vertexcomponent.h"
#include "coregraphics/primitivetopology.h"
#include "coregraphics/antialiasquality.h"
#include "coregraphics/imagefileformat.h"
#include "coregraphics/indextype.h"
#include "coregraphics/base/resourcebase.h"

namespace Direct3D11
{
class D3D11Types
{
public:
    /// convert Nebula pixel format to D3D11 pixel format
    static DXGI_FORMAT AsD3D11PixelFormat(CoreGraphics::PixelFormat::Code p);
    /// convert Direct3D to Nebula pixel format
    static CoreGraphics::PixelFormat::Code AsNebulaPixelFormat(DXGI_FORMAT f);
	/// convert Direct3D pixel format to byte size
	static UINT AsByteSize(DXGI_FORMAT f);
    /// convert vertex component type to D3D11 declaration type
    static DXGI_FORMAT AsD3D11VertexDeclarationType(CoreGraphics::VertexComponent::Format f);
    /// convert vertex component semantic name as D3D11 declaration usage
    static D3DDECLUSAGE AsD3D11VertexDeclarationUsage(CoreGraphics::VertexComponent::SemanticName n);
	/// convert the format to it's size
	static UINT AsByteSize( UINT semantic );
    /// convert primitive topology to D3D
    static D3D11_PRIMITIVE_TOPOLOGY AsD3D11PrimitiveType(CoreGraphics::PrimitiveTopology::Code t);
	/// convert primitive topology to tessellateable topology
	static D3D11_PRIMITIVE_TOPOLOGY AsD3D11TessellateableType(D3D11_PRIMITIVE_TOPOLOGY t);
    /// convert antialias quality to D3D multisample type
    static UINT AsD3D11MultiSampleType(CoreGraphics::AntiAliasQuality::Code c);
    /// convert image file format to D3DX file format
    static D3DX11_IMAGE_FILE_FORMAT AsD3DXImageFileFormat(CoreGraphics::ImageFileFormat::Code c);
    /// convert Nebula3 resource usage/access flag pair into D3D11 pool
    static D3DPOOL AsD3D11Pool(Base::ResourceBase::Usage usage, Base::ResourceBase::Access access);
    /// convert Nebula3 resource usage/access flag pair into D3D11 usage flags
    static UINT AsD3D11Usage(Base::ResourceBase::Usage usage);
	/// convert Nebula3 access to D3D11 CPU access
	static UINT AsD3D11Access( Base::ResourceBase::Access access);
	/// convert Nebula3 access to appropriate buffer
	static UINT AsD3D11BufferType( Base::ResourceBase::Usage usage );
	/// check to see if format is compatible with creating a shader resource view
	static BOOL CanCreateShaderResourceView( DXGI_FORMAT f );
    /// convert index type to DXGI format
    static DXGI_FORMAT IndexTypeAsD3D11Format(CoreGraphics::IndexType::Code indexType);
	/// convert vertex component semantics to DX11 format
	static Util::String SemanticNameAsD3D11Format( UINT semantic, UINT semanticIndex );
};

} // namespace Direct3D11
//------------------------------------------------------------------------------
