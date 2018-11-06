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
#include "frame/framesubpasssystem.h"
namespace Lighting
{
class LightContext : public Graphics::GraphicsContext
{
	_DeclareContext();
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

	/// setup light context
	static void Create();

	/// setup entity as global light
	static void SetupGlobalLight(const Graphics::GraphicsEntityId id, const Math::float4& color, const float intensity, const Math::float4& ambient, const Math::float4& backlight, const float backlightFactor, const Math::vector& direction, bool castShadows);
	/// setup entity as point light source
	static void SetupPointLight(const Graphics::GraphicsEntityId id, const Math::float4& color, const float intensity, const Math::matrix44& transform, const float range, bool castShadows);
	/// setup entity as spot light
	static void SetupSpotLight(const Graphics::GraphicsEntityId id, const Math::float4& color, const float intensity, const Math::matrix44& transform, const float coneAngle, bool castShadows);

	/// set color of light
	static void SetColor(const Graphics::GraphicsEntityId id, const Math::float4& color);
	/// set intensity of light
	static void SetIntensity(const Graphics::GraphicsEntityId id, const float intensity);
	/// get transform
	static const Math::matrix44 GetTransform(const Graphics::GraphicsEntityId id);

	/// set transform, type must match the type the entity was created with
	static void SetSpotLightTransform(const Graphics::GraphicsEntityId id, const Math::matrix44& transform);
	/// set transform, type must match the type the entity was created with
	static void SetPointLightTransform(const Graphics::GraphicsEntityId id, const Math::matrix44& transform);
	/// set global light direction
	static void SetGlobalLightDirection(const Graphics::GraphicsEntityId id, const Math::vector& direction);

	/// do light classification for tiled/clustered compute
	static void OnBeforeView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);

private:

	friend struct Frame::FrameSubpassSystem::CompiledImpl;

	/// render global lights
	static void RenderLights();

	/// update global shadows
	static void UpdateGlobalShadowMap();
	/// update spotlight shadows
	static void UpdateSpotShadows();
	/// update pointligt shadows
	static void UpdatePointShadows();

	enum
	{
		Type,
		Color,
		Intensity,
		ShadowCaster,
		TypedLightId
	};

	typedef Ids::IdAllocator<
		LightType,				// type
		Math::float4,			// color
		float,					// intensity
		bool,					// shadow caster
		Ids::Id32				// typed light id (index into pointlights, spotlights and globallights)
	> GenericLightAllocator;
	static GenericLightAllocator genericLightAllocator;

	struct ConstantBufferSet
	{
		uint offset, slice;
	};

	enum
	{
		PointLightTransform,
		PointLightConstantBufferSet,
		PointLightShadowConstantBufferSet,
		PointLightDynamicOffsets
	};

	typedef Ids::IdAllocator<
		Math::matrix44,			// transform
		ConstantBufferSet,		// constant buffer binding for light
		ConstantBufferSet,		// constant buffer binding for shadows
		Util::FixedArray<uint>	// dynamic offsets
	> PointLightAllocator;
	static PointLightAllocator pointLightAllocator;

	enum
	{
		SpotLightTransform,
		SpotLightProjection,
		SpotLightConeAngle,
		SpotLightConstantBufferSet,
		SpotLightShadowConstantBufferSet,
		SpotLightDynamicOffsets
	};

	typedef Ids::IdAllocator<
		Math::matrix44,				// transform
		Math::matrix44,				// projection
		float,						// cone angle
		ConstantBufferSet,			// constant buffer binding for light
		ConstantBufferSet,			// constant buffer binding for shadows
		Util::FixedArray<uint>	// dynamic offsets
	> SpotLightAllocator;
	static SpotLightAllocator spotLightAllocator;

	enum
	{
		GlobalLightDirection,
		GlobalLightBacklight,
		GlobalLightBacklightOffset,
		GlobalLightAmbient,
		GlobalLightTransform
	};
	typedef Ids::IdAllocator<
		Math::float4,			// direction
		Math::float4,			// backlight color
		float,					// backlight offset
		Math::float4,			// ambient
		Math::matrix44			// transform (basically just a rotation in the direction)
	> GlobalLightAllocator;
	static GlobalLightAllocator globalLightAllocator;

	/// allocate a new slice for this context
	static Graphics::ContextEntityId Alloc();
	/// deallocate a slice
	static void Dealloc(Graphics::ContextEntityId id);
};
} // namespace Lighting