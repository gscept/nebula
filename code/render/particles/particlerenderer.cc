//------------------------------------------------------------------------------
//  particlerenderer.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "particles/particlerenderer.h"

namespace Particles
{
#if __DX11__
__ImplementClass(Particles::ParticleRenderer, 'PRRD', Direct3D11::D3D11ParticleRenderer);
#elif __DX9__
__ImplementClass(Particles::ParticleRenderer, 'PRRD', Direct3D9::D3D9ParticleRenderer);
#elif __OGL4__
__ImplementClass(Particles::ParticleRenderer, 'PRRD', OpenGL4::OGL4ParticleRenderer);
#elif __VULKAN__
__ImplementClass(Particles::ParticleRenderer, 'PRRD', Vulkan::VkParticleRenderer);
#elif __XBOX360__
__ImplementClass(Particles::ParticleRenderer, 'PRRD', Xbox360::Xbox360ParticleRenderer);
#elif __WII__
__ImplementClass(Particles::ParticleRenderer, 'PRRD', Wii::WiiParticleRenderer);
#elif __PS3__
__ImplementClass(Particles::ParticleRenderer, 'PRRD', Base::ParticleRendererBase);
#else
#error "Particles::ParticleRenderer not implemented on this platform!"
#endif
__ImplementSingleton(Particles::ParticleRenderer);

//------------------------------------------------------------------------------
/**
*/
ParticleRenderer::ParticleRenderer()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ParticleRenderer::~ParticleRenderer()
{
    __DestructSingleton;
}

} // namespace Particles
