#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::ParticleSystemRenderer
    
    Platform-wrapper for particle-system-instance rendering.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "particles/d3d11/d3d11particlesysteminstance.h"
namespace Particles
{
class ParticleSystemInstance : public Direct3D11::D3D11ParticleSystemInstance
{
	__DeclareClass(ParticleSystemInstance);
};
}
#elif __DX9__
#include "particles/d3d9/d3d9particlesysteminstance.h"
namespace Particles
{
class ParticleSystemInstance : public Direct3D9::D3D9ParticleSystemInstance
{
    __DeclareClass(ParticleSystemInstance);
};
}
#elif __OGL4__
#include "particles/ogl4/ogl4particlesysteminstance.h"
namespace Particles
{
class ParticleSystemInstance : public OpenGL4::OGL4ParticleSystemInstance
{
	__DeclareClass(ParticleSystemInstance);
};
}
#elif __VULKAN__
#include "particles/vk/vkparticlesysteminstance.h"
namespace Particles
{
class ParticleSystemInstance : public Vulkan::VkParticleSystemInstance
{
	__DeclareClass(ParticleSystemInstance);
};
}
#elif __XBOX360__
#include "particles/xbox360/xbox360particlesysteminstance.h"
namespace Particles
{
class ParticleSystemInstance : public Xbox360::Xbox360ParticleSystemInstance
{
    __DeclareClass(ParticleSystemInstance);
};
}
#elif __WII__
#include "particles/wii/wiiparticlesysteminstance.h"
namespace Particles
{
class ParticleSystemInstance : public Wii::WiiParticleSystemInstance
{
    __DeclareClass(ParticleSystemInstance);
};
}
#elif __PS3__
#include "particles/ps3/ps3particlesysteminstance.h"
namespace Particles
{
class ParticleSystemInstance : public PS3::PS3ParticleSystemInstance
{
    __DeclareClass(ParticleSystemInstance);
};
}
#else
#error "Particles::ParticleRenderer not implemented on this platform!"
#endif
//------------------------------------------------------------------------------


    