#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::ParticleRenderer
    
    Platform-wrapper for particle rendering.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "particles/d3d11/d3d11particlerenderer.h"
namespace Particles
{
	class ParticleRenderer : public Direct3D11::D3D11ParticleRenderer
	{
		__DeclareClass(ParticleRenderer);
		__DeclareSingleton(ParticleRenderer);
	public:
		/// constructor
		ParticleRenderer();
		/// destructor
		virtual ~ParticleRenderer();
	};
}
#elif __DX9__
#include "particles/d3d9/d3d9particlerenderer.h"
namespace Particles
{
class ParticleRenderer : public Direct3D9::D3D9ParticleRenderer
{
    __DeclareClass(ParticleRenderer);
    __DeclareSingleton(ParticleRenderer);
public:
    /// constructor
    ParticleRenderer();
    /// destructor
    virtual ~ParticleRenderer();
};
}
#elif __OGL4__
#include "particles/ogl4/ogl4particlerenderer.h"
namespace Particles
{
class ParticleRenderer : public OpenGL4::OGL4ParticleRenderer
{
	__DeclareClass(ParticleRenderer);
	__DeclareSingleton(ParticleRenderer);
public:
	/// constructor
	ParticleRenderer();
	/// destructor
	virtual ~ParticleRenderer();
};
}
#elif __VULKAN__
#include "particles/vk/vkparticlerenderer.h"
namespace Particles
{
class ParticleRenderer : public Vulkan::VkParticleRenderer
{
	__DeclareClass(ParticleRenderer);
	__DeclareSingleton(ParticleRenderer);
public:
	/// constructor
	ParticleRenderer();
	/// destructor
	virtual ~ParticleRenderer();
};
}
#elif __XBOX360__
#include "particles/xbox360/xbox360particlerenderer.h"
namespace Particles
{
class ParticleRenderer : public Xbox360::Xbox360ParticleRenderer
{
    __DeclareClass(ParticleRenderer);
    __DeclareSingleton(ParticleRenderer);
public:
    /// constructor
    ParticleRenderer();
    /// destructor
    virtual ~ParticleRenderer();
};
}
#elif __WII__
#include "particles/wii/wiiparticlerenderer.h"
namespace Particles
{
class ParticleRenderer : public Wii::WiiParticleRenderer
{
    __DeclareClass(ParticleRenderer);
    __DeclareSingleton(ParticleRenderer);
public:
    /// constructor
    ParticleRenderer();
    /// destructor
    virtual ~ParticleRenderer();
};
}
#elif __PS3__
#include "particles/ps3/ps3particlerenderer.h"
namespace Particles
{
class ParticleRenderer : public PS3::PS3ParticleRenderer
{
    __DeclareClass(ParticleRenderer);
    __DeclareSingleton(ParticleRenderer);
public:
    /// constructor
    ParticleRenderer();
    /// destructor
    virtual ~ParticleRenderer();
};
}
#else
#error "Particles::ParticleRenderer not implemented on this platform!"
#endif
//------------------------------------------------------------------------------


    