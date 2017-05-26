#pragma once
//------------------------------------------------------------------------------
/**
	Implements a particle system as used by Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "particles/base/particlesysteminstancebase.h"
namespace Vulkan
{
class VkParticleSystemInstance : public Particles::ParticleSystemInstanceBase
{
	__DeclareClass(VkParticleSystemInstance);
public:
	/// constructor
	VkParticleSystemInstance();
	/// destructor
	virtual ~VkParticleSystemInstance();

	/// generate vertex streams to render
	virtual void UpdateVertexStreams();
	/// render the particle system
	virtual void Render();
private:
};
} // namespace Vulkan