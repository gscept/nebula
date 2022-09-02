//------------------------------------------------------------------------------
// bindlessregistry.cc
// (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "bindlessregistry.h"
#include "shared.h"
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
    BindlessIndex idx = state.texturePool.Alloc();
    IndexT var;
    switch (type)
    {
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
    state.bindResourceCriticalSection.Enter();
    IndexT i;
    for (i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        ResourceTableSetTexture(Graphics::GetTickResourceTableGraphics(i), info);
        ResourceTableSetTexture(Graphics::GetTickResourceTableCompute(i), info);
    }
    state.bindResourceCriticalSection.Leave();

    return idx;
}

//------------------------------------------------------------------------------
/**
*/
void 
ReregisterTexture(const CoreGraphics::TextureId& tex, CoreGraphics::TextureType type, BindlessIndex index, bool depth, bool stencil)
{
    IndexT var;
    switch (type)
    {
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
    }

    CoreGraphics::ResourceTableTexture info;
    info.tex = tex;
    info.index = index;
    info.sampler = CoreGraphics::InvalidSamplerId;
    info.isDepth = depth;
    info.isStencil = stencil;
    info.slot = var;

    // update textures for all tables
    state.bindResourceCriticalSection.Enter();
    IndexT i;
    for (i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        ResourceTableSetTexture(Graphics::GetTickResourceTableGraphics(i), info);
        ResourceTableSetTexture(Graphics::GetTickResourceTableCompute(i), info);
    }
    state.bindResourceCriticalSection.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void 
UnregisterTexture(const BindlessIndex id, const CoreGraphics::TextureType type)
{
    IndexT var;
    switch (type)
    {
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
    }    

    CoreGraphics::ResourceTableTexture info;
    info.tex = (CoreGraphics::TextureId)0;
    info.index = id;
    info.sampler = CoreGraphics::InvalidSamplerId;
    info.isDepth = false;
    info.isStencil = false;
    info.slot = var;

    // update textures for all tables
    state.bindResourceCriticalSection.Enter();
    IndexT i;
    for (i = 0; i < CoreGraphics::GetNumBufferedFrames(); i++)
    {
        ResourceTableSetTexture(Graphics::GetTickResourceTableGraphics(i), info);
        ResourceTableSetTexture(Graphics::GetTickResourceTableCompute(i), info);
    }
    state.bindResourceCriticalSection.Leave();

    // Free id at the end
    state.texturePool.Free(id);
}

} // namespace Graphics
