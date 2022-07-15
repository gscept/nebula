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
    Util::FixedPool<uint32_t> texture1DPool;
    Util::FixedPool<uint32_t> texture1DArrayPool;
    Util::FixedPool<uint32_t> texture2DPool;
    Util::FixedPool<uint32_t> texture2DMSPool;
    Util::FixedPool<uint32_t> texture2DArrayPool;
    Util::FixedPool<uint32_t> texture3DPool;
    Util::FixedPool<uint32_t> textureCubePool;
    Util::FixedPool<uint32_t> textureCubeArrayPool;

    IndexT texture2DTextureVar;
    IndexT texture2DMSTextureVar;
    IndexT texture2DArrayTextureVar;
    IndexT texture3DTextureVar;
    IndexT textureCubeTextureVar;

    IndexT normalBufferTextureVar;
    IndexT depthBufferTextureVar;
    IndexT specularBufferTextureVar;
    IndexT depthBufferCopyTextureVar;
    
    IndexT environmentMapVar;
    IndexT irradianceMapVar;
    IndexT numEnvMipsVar;

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
    
    state.texture2DPool.SetSetupFunc(func);
    state.texture2DPool.Resize(Shared::MAX_2D_TEXTURES);
    state.texture2DMSPool.SetSetupFunc(func);
    state.texture2DMSPool.Resize(Shared::MAX_2D_MS_TEXTURES);
    state.texture3DPool.SetSetupFunc(func);
    state.texture3DPool.Resize(Shared::MAX_3D_TEXTURES);
    state.textureCubePool.SetSetupFunc(func);
    state.textureCubePool.Resize(Shared::MAX_CUBE_TEXTURES);
    state.texture2DArrayPool.SetSetupFunc(func);
    state.texture2DArrayPool.Resize(Shared::MAX_2D_ARRAY_TEXTURES);
    
    // create shader state for textures, and fetch variables
    CoreGraphics::ShaderId shader = CoreGraphics::ShaderGet("shd:shared.fxb"_atm);
    
    state.texture2DTextureVar = CoreGraphics::ShaderGetResourceSlot(shader, "Textures2D");
    state.texture2DMSTextureVar = CoreGraphics::ShaderGetResourceSlot(shader, "Textures2DMS");
    state.texture2DArrayTextureVar = CoreGraphics::ShaderGetResourceSlot(shader, "Textures2DArray");
    state.textureCubeTextureVar = CoreGraphics::ShaderGetResourceSlot(shader, "TexturesCube");
    state.texture3DTextureVar = CoreGraphics::ShaderGetResourceSlot(shader, "Textures3D");
    
    state.normalBufferTextureVar = CoreGraphics::ShaderGetConstantBinding(shader, "NormalBuffer");
    state.depthBufferTextureVar = CoreGraphics::ShaderGetConstantBinding(shader, "DepthBuffer");
    state.specularBufferTextureVar = CoreGraphics::ShaderGetConstantBinding(shader, "SpecularBuffer");
    state.depthBufferCopyTextureVar = CoreGraphics::ShaderGetConstantBinding(shader, "DepthBufferCopy");
    
    state.environmentMapVar = CoreGraphics::ShaderGetConstantBinding(shader, "EnvironmentMap");
    state.irradianceMapVar = CoreGraphics::ShaderGetConstantBinding(shader, "IrradianceMap");
    state.numEnvMipsVar = CoreGraphics::ShaderGetConstantBinding(shader, "NumEnvMips");
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
    BindlessIndex idx;
    IndexT var;
    switch (type)
    {
    case CoreGraphics::Texture2D:
        n_assert(!state.texture2DPool.IsFull());
        idx = state.texture2DPool.Alloc();
        var = state.texture2DTextureVar;
        break;
    case CoreGraphics::Texture2DArray:
        n_assert(!state.texture2DArrayPool.IsFull());
        idx = state.texture2DArrayPool.Alloc();
        var = state.texture2DArrayTextureVar;
        break;
    case CoreGraphics::Texture3D:
        n_assert(!state.texture3DPool.IsFull());
        idx = state.texture3DPool.Alloc();
        var = state.texture3DTextureVar;
        break;
    case CoreGraphics::TextureCube:
        n_assert(!state.textureCubePool.IsFull());
        idx = state.textureCubePool.Alloc();
        var = state.textureCubeTextureVar;
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
        var = state.texture2DTextureVar;
        break;
    case CoreGraphics::Texture2DArray:
        var = state.texture2DArrayTextureVar;
        break;
    case CoreGraphics::Texture3D:
        var = state.texture3DTextureVar;
        break;
    case CoreGraphics::TextureCube:
        var = state.textureCubeTextureVar;
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
        var = state.texture2DTextureVar;
        state.texture2DPool.Free(id);
        break;
    case CoreGraphics::Texture2DArray:
        var = state.texture2DArrayTextureVar;
        state.texture2DArrayPool.Free(id);
        break;
    case CoreGraphics::Texture3D:
        var = state.texture3DTextureVar;
        state.texture3DPool.Free(id);
        break;
    case CoreGraphics::TextureCube:
        var = state.textureCubeTextureVar;
        state.textureCubePool.Free(id);
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
}

} // namespace Graphics
