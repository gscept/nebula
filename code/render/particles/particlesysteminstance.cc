//------------------------------------------------------------------------------
//  particlerenderer.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "particles/particlesysteminstance.h"

namespace Particles
{
#if __DX11__
__ImplementClass(Particles::ParticleSystemInstance, 'PASI', Direct3D11::D3D11ParticleSystemInstance);
#elif __DX9__
__ImplementClass(Particles::ParticleSystemInstance, 'PASI', Direct3D9::D3D9ParticleSystemInstance);
#elif __OGL4__
__ImplementClass(Particles::ParticleSystemInstance, 'PASI', OpenGL4::OGL4ParticleSystemInstance);
#elif __VULKAN__
__ImplementClass(Particles::ParticleSystemInstance, 'PASI', Vulkan::VkParticleSystemInstance);
#elif __XBOX360__
__ImplementClass(Particles::ParticleSystemInstance, 'PASI', Xbox360::Xbox360ParticleSystemInstance);
#elif __WII__
__ImplementClass(Particles::ParticleSystemInstance, 'PASI', Wii::WiiParticleSystemInstance);
#elif __PS3__
__ImplementClass(Particles::ParticleSystemInstance, 'PASI', PS3::PS3ParticleSystemInstance);
#else
#error "Particles::ParticleRenderer not implemented on this platform!"
#endif


} // namespace Particles
