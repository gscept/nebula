//------------------------------------------------------------------------------
// bindlessregistry.cc
// (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "bindlessregistry.h"
#include "system_shaders/shared.h"
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
        var = Shared::Table_Tick::Textures1D_SLOT;
        break;
    case CoreGraphics::Texture1DArray:
        n_assert(!state.texturePool.IsFull());
        var = Shared::Table_Tick::Textures1DArray_SLOT;
        break;
    case CoreGraphics::Texture2D:
        n_assert(!state.texturePool.IsFull());
        var = Shared::Table_Tick::Textures2D_SLOT;
        break;
    case CoreGraphics::Texture2DArray:
        n_assert(!state.texturePool.IsFull());
        var = Shared::Table_Tick::Textures2DArray_SLOT;
        break;
    case CoreGraphics::Texture3D:
        n_assert(!state.texturePool.IsFull());
        var = Shared::Table_Tick::Textures3D_SLOT;
        break;
    case CoreGraphics::TextureCube:
        n_assert(!state.texturePool.IsFull());
        var = Shared::Table_Tick::TexturesCube_SLOT;
        break;
    case CoreGraphics::TextureCubeArray:
        n_assert(!state.texturePool.IsFull());
        var = Shared::Table_Tick::TexturesCubeArray_SLOT;
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
        var = Shared::Table_Tick::Textures1D_SLOT;
        break;
    case CoreGraphics::Texture1DArray:
        var = Shared::Table_Tick::Textures1DArray_SLOT;
        break;
    case CoreGraphics::Texture2D:
        var = Shared::Table_Tick::Textures2D_SLOT;
        break;
    case CoreGraphics::Texture2DArray:
        var = Shared::Table_Tick::Textures2DArray_SLOT;
        break;
    case CoreGraphics::Texture3D:
        var = Shared::Table_Tick::Textures3D_SLOT;
        break;
    case CoreGraphics::TextureCube:
        var = Shared::Table_Tick::TexturesCube_SLOT;
        break;
    case CoreGraphics::TextureCubeArray:
        var = Shared::Table_Tick::TexturesCubeArray_SLOT;
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
    switch (type)
    {
    case CoreGraphics::Texture1D:
        var = Shared::Table_Tick::Textures1D_SLOT;
        break;
    case CoreGraphics::Texture1DArray:
        var = Shared::Table_Tick::Textures1DArray_SLOT;
        break;
    case CoreGraphics::Texture2D:
        var = Shared::Table_Tick::Textures2D_SLOT;
        break;
    case CoreGraphics::Texture2DArray:
        var = Shared::Table_Tick::Textures2DArray_SLOT;
        break;
    case CoreGraphics::Texture3D:
        var = Shared::Table_Tick::Textures3D_SLOT;
        break;
    case CoreGraphics::TextureCube:
        var = Shared::Table_Tick::TexturesCube_SLOT;
        break;
    case CoreGraphics::TextureCubeArray:
        var = Shared::Table_Tick::TexturesCubeArray_SLOT;
        break;
    default: n_error("unhandled enum"); break;
    }    

    CoreGraphics::ResourceTableTexture info;
    info.tex = CoreGraphics::InvalidTextureId;
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
