//------------------------------------------------------------------------------
// bindlessregistry.cc
// (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "bindlessregistry.h"
#include "globalconstants.h"
namespace Graphics
{

struct
{
    Util::FixedPool<uint32_t> texturePool;

    Threading::CriticalSection bindResourceCriticalSection;

} state;

//------------------------------------------------------------------------------
/**
*/
void 
CreateBindlessRegistry(const BindlessRegistryCreateInfo& info)
{
    auto func = [](uint32_t& val, IndexT i) -> void {
        val = i;
    };

    state.texturePool.SetSetupFunc(func);
    state.texturePool.Resize(Shared::MAX_TEXTURES);
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyBindlessRegistry()
{
    // Do nothing
}

//------------------------------------------------------------------------------
/**
*/
BindlessIndex
RegisterTexture(const CoreGraphics::TextureId& tex, CoreGraphics::TextureType type, bool depth, bool stencil)
{
    Threading::CriticalScope _0(&state.bindResourceCriticalSection);
    BindlessIndex idx = state.texturePool.Alloc();
    IndexT var;
    switch (type)
    {
    case CoreGraphics::Texture1D:
        n_assert(!state.texturePool.IsFull());
        var = Shared::Textures1D::BINDING;
        break;
    case CoreGraphics::Texture1DArray:
        n_assert(!state.texturePool.IsFull());
        var = Shared::Textures1DArray::BINDING;
        break;
    case CoreGraphics::Texture2D:
        n_assert(!state.texturePool.IsFull());
        var = Shared::Textures2D::BINDING;
        break;
    case CoreGraphics::Texture2DArray:
        n_assert(!state.texturePool.IsFull());
        var = Shared::Textures2DArray::BINDING;
        break;
    case CoreGraphics::Texture3D:
        n_assert(!state.texturePool.IsFull());
        var = Shared::Textures3D::BINDING;
        break;
    case CoreGraphics::TextureCube:
        n_assert(!state.texturePool.IsFull());
        var = Shared::TexturesCube::BINDING;
        break;
    case CoreGraphics::TextureCubeArray:
        n_assert(!state.texturePool.IsFull());
        var = Shared::TexturesCubeArray::BINDING;
        break;
    default:
        n_error("Should not happen");
        idx = UINT_MAX;
        var = InvalidIndex;
    }

    CoreGraphics::ResourceTableTexture info;
    info.tex = tex;
    info.index = idx;
    info.sampler = CoreGraphics::InvalidSamplerId;
    info.isDepth = depth;
    info.isStencil = stencil;
    info.slot = var;

    // update textures for all tables
    IndexT i;
    for (i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        ResourceTableSetTexture(Graphics::GetTickResourceTable(i), info);
    }
    return idx;
}

//------------------------------------------------------------------------------
/**
*/
void 
ReregisterTexture(const CoreGraphics::TextureId& tex, CoreGraphics::TextureType type, BindlessIndex index, bool depth, bool stencil)
{
    Threading::CriticalScope _0(&state.bindResourceCriticalSection);
    IndexT var = -1;
    switch (type)
    {
    case CoreGraphics::Texture1D:
        var = Shared::Textures1D::BINDING;
        break;
    case CoreGraphics::Texture1DArray:
        var = Shared::Textures1DArray::BINDING;
        break;
    case CoreGraphics::Texture2D:
        var = Shared::Textures2D::BINDING;
        break;
    case CoreGraphics::Texture2DArray:
        var = Shared::Textures2DArray::BINDING;
        break;
    case CoreGraphics::Texture3D:
        var = Shared::Textures3D::BINDING;
        break;
    case CoreGraphics::TextureCube:
        var = Shared::TexturesCube::BINDING;
        break;
    case CoreGraphics::TextureCubeArray:
        var = Shared::TexturesCubeArray::BINDING;
        break;
    default: n_error("unhandled enum"); break;
    }

    CoreGraphics::ResourceTableTexture info;
    info.tex = tex;
    info.index = index;
    info.sampler = CoreGraphics::InvalidSamplerId;
    info.isDepth = depth;
    info.isStencil = stencil;
    info.slot = var;

    // update textures for all tables
    IndexT i;
    for (i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        ResourceTableSetTexture(Graphics::GetTickResourceTable(i), info);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
UnregisterTexture(const BindlessIndex id, const CoreGraphics::TextureType type)
{
    Threading::CriticalScope _0(&state.bindResourceCriticalSection);
    IndexT var = -1;
    CoreGraphics::TextureId fallback;
    switch (type)
    {
    case CoreGraphics::Texture1D:
        var = Shared::Textures1D::BINDING;
        fallback = CoreGraphics::White1D;
        break;
    case CoreGraphics::Texture1DArray:
        var = Shared::Textures1DArray::BINDING;
        fallback = CoreGraphics::White1DArray;
        break;
    case CoreGraphics::Texture2D:
        fallback = CoreGraphics::White2D;
        var = Shared::Textures2D::BINDING;
        break;
    case CoreGraphics::Texture2DArray:
        fallback = CoreGraphics::White2DArray;
        var = Shared::Textures2DArray::BINDING;
        break;
    case CoreGraphics::Texture3D:
        fallback = CoreGraphics::White3D;
        var = Shared::Textures3D::BINDING;
        break;
    case CoreGraphics::TextureCube:
        fallback = CoreGraphics::WhiteCube;
        var = Shared::TexturesCube::BINDING;
        break;
    case CoreGraphics::TextureCubeArray:
        fallback = CoreGraphics::WhiteCubeArray;
        var = Shared::TexturesCubeArray::BINDING;
        break;
    default: n_error("unhandled enum"); break;
    }    

    CoreGraphics::ResourceTableTexture info;
    info.tex = fallback;
    info.index = id;
    info.sampler = CoreGraphics::InvalidSamplerId;
    info.isDepth = false;
    info.isStencil = false;
    info.slot = var;

    // update textures for all tables
    IndexT i;
    for (i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        ResourceTableSetTexture(Graphics::GetTickResourceTable(i), info);
    }

    // Free id at the end
    state.texturePool.Free(id);
}

} // namespace Graphics
