//------------------------------------------------------------------------------
// vkstreamshaderloader.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkshaderloader.h"
#include "vkshader.h"
#include "coregraphics/constantbuffer.h"
#include "effectfactory.h"
#include "coregraphics/config.h"
#include "coregraphics/shaderstate.h"

using namespace CoreGraphics;
using namespace IO;
namespace Vulkan
{

__ImplementClass(Vulkan::VkShaderLoader, 'VKSL', Resources::ResourceLoader);


//------------------------------------------------------------------------------
/**
	TODO: In Vulkan, we might be able to thread the shader creation process
*/
bool
VkShaderLoader::CanLoadAsync() const
{
	return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
VkShaderLoader::SetupResourceFromStream(const Ptr<IO::Stream>& stream)
{
	n_assert(stream.isvalid());
	n_assert(stream->CanBeMapped());
	n_assert(this->resource->IsA(VkShader::RTTI));
	const Ptr<VkShader>& res = this->resource.downcast<VkShader>();
	n_assert(!res->IsLoaded());

	// map stream to memory
	stream->SetAccessMode(Stream::ReadAccess);
	if (stream->Open())
	{
		void* srcData = stream->Map();
		uint srcDataSize = stream->GetSize();

		// load effect from memory
		AnyFX::ShaderEffect* effect = AnyFX::EffectFactory::Instance()->CreateShaderEffectFromMemory(srcData, srcDataSize);

		// catch any potential error coming from AnyFX
		if (!effect)
		{
			n_error("VkStreamShaderLoader::SetupResourceFromStream(): failed to load shader '%s'!",
				res->GetResourceId().Value());
			return false;
		}

		res->vkEffect = effect;
		res->shaderName = res->GetResourceId().AsString();
		res->shaderIdentifierCode = ShaderIdentifier::FromName(res->shaderName.AsString());
		res->Setup(effect);

		// setup shader variations
		const eastl::vector<AnyFX::ProgramBase*> programs = effect->GetPrograms();
		for (uint i = 0; i < programs.size(); i++)
		{
			// a shader variation in Nebula is equal to a program object in AnyFX
			Ptr<CoreGraphics::ShaderVariation> variation = CoreGraphics::ShaderVariation::Create();

			// get program object from shader subsystem
			AnyFX::VkProgram* program = static_cast<AnyFX::VkProgram*>(programs[i]);
			variation->Setup(program, res->pipelineLayout);
			res->variations.Add(variation->GetFeatureMask(), variation);
		}

		// make sure that the shader has one variation selected
		if (!res->variations.IsEmpty()) res->activeVariation = res->variations.ValueAtIndex(0);

		//res->mainState = res->CreateState({ NEBULAT_DEFAULT_GROUP });

#if __NEBULA3_HTTP__
		//res->debugState = res->CreateState();
#endif
		return true;
	}
	return false;
}

} // namespace Vulkan