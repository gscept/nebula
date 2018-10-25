#pragma once
//------------------------------------------------------------------------------
/**
	Adds a light representation to the graphics entity
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "coregraphics/shader.h"
#include "coregraphics/constantbuffer.h"
namespace Lighting
{
class LightContext : public Graphics::GraphicsContext
{
	DeclareContext();
public:

	enum LightType
	{
		GlobalLightType,
		PointLightType,
		SpotLightType
	};

	/// constructor
	LightContext();
	/// destructor
	virtual ~LightContext();

	/// setup entity as global light
	static void SetupGlobalLight(const Graphics::GraphicsEntityId id, const Math::float4& color, const float intensity, bool castShadows);
	/// setup entity as point light source
	static void SetupPointLight(const Graphics::GraphicsEntityId id, const Math::float4& color, const float intensity, bool castShadows);
	/// setup entity as spot light
	static void SetupSpotLight(const Graphics::GraphicsEntityId id, const Math::float4& color, const float intensity, const float coneAngle, bool castShadows);

	/// get transform, type must match the type the entity was created with
	static const Math::matrix44& GetTransform(const Graphics::GraphicsEntityId id);
	/// set transform, type must match the type the entity was created with
	static void SetTransform(const Graphics::GraphicsEntityId id, const Math::matrix44& transform);
	/// set color of light
	static void SetColor(const Graphics::GraphicsEntityId id, const Math::float4& color);
	/// set intensity of light
	static void SetIntensity(const Graphics::GraphicsEntityId id, const float intensity);

	/// do light classification for tiled/clustered compute
	static void OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime);

private:

	enum
	{
		Transform,
		Type,
		Color,
		Intensity,
		ShadowCaster
	};

	typedef Ids::IdAllocator<
		Math::matrix44,			// transform
		LightType,				// type
		Math::float4,			// color
		float,					// intensity
		bool					// shadow caster
	> GenericLightAllocator;
	static GenericLightAllocator genericLightAllocator;

	Util::HashTable<const Graphics::GraphicsEntityId, Ids::Id32> pointLightMapping;
	Util::HashTable<const Graphics::GraphicsEntityId, Ids::Id32> spotLightMapping;

	enum
	{
		PointLightTransform,
		PointLightColor,
		PointLightIntensity,
		PointLightShadowCasting
	};

	typedef Ids::IdAllocator<
	> PointLightAllocator;
	static PointLightAllocator pointLightAllocator;

	enum
	{
		SpotLightProjection,
		SpotLightConeAngle
	};

	typedef Ids::IdAllocator<
		Math::matrix44,				// projection
		float						// cone angle
	> SpotLightAllocator;
	static SpotLightAllocator spotLightAllocator;

	/// allocate a new slice for this context
	static Graphics::ContextEntityId Alloc();
	/// deallocate a slice
	static void Dealloc(Graphics::ContextEntityId id);

	CoreGraphics::ShaderId lightShader;
	CoreGraphics::ShaderProgramId lightsClassification;		// pre-pass for compute
	CoreGraphics::ShaderProgramId lightsCompute;			// run compute shader for lights
	CoreGraphics::ShaderProgramId lightsDraw;				// use draw-version (old) for lights

	CoreGraphics::ConstantBufferId tickParamsBuffer;		// contains lighting constants
};
} // namespace Lighting