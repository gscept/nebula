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
#include <array>

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
		SpotLightType,
		AreaLightType
	};

	/// constructor
	LightContext();
	/// destructor
	virtual ~LightContext();

	/// setup light context
	static void Create();

	/// setup entity as global light
	static void SetupGlobalLight(const Graphics::GraphicsEntityId id, const Math::float4& color, const float intensity, const Math::float4& ambient, const Math::float4& backlight, const float backlightFactor, const Math::vector& direction, bool castShadows = false);
	/// setup entity as point light source
	static void SetupPointLight(const Graphics::GraphicsEntityId id, 
		const Math::float4& color, 
		const float intensity, 
		const Math::matrix44& transform, 
		const float range, 
		bool castShadows = false, 
		const CoreGraphics::TextureId projection = CoreGraphics::TextureId::Invalid());

	/// setup entity as spot light
	static void SetupSpotLight(const Graphics::GraphicsEntityId id, 
		const Math::float4& color, 
		const float intensity, 
		const float innerConeAngle,
		const float outerConeAngle,
		const Math::matrix44& transform,
		bool castShadows = false, 
		const CoreGraphics::TextureId projection = CoreGraphics::TextureId::Invalid());

	/// set color of light
	static void SetColor(const Graphics::GraphicsEntityId id, const Math::float4& color);
	/// set intensity of light
	static void SetIntensity(const Graphics::GraphicsEntityId id, const float intensity);
	/// get transform
	static const Math::matrix44 GetTransform(const Graphics::GraphicsEntityId id);
	/// set transform depending on type
	static void SetTransform(const Graphics::GraphicsEntityId id, const Math::matrix44& transform);
	/// get the view transform including projections
	static const Math::matrix44 GetViewProjTransform(const Graphics::GraphicsEntityId id);

	/// do light classification for tiled/clustered compute
	static void OnBeforeView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
#ifndef PUBLIC_BUILD
	//
	static void OnRenderDebug(uint32_t flags);
#endif

private:

	friend struct Frame::FrameSubpassSystem::CompiledImpl;

	/// set transform, type must match the type the entity was created with
	static void SetSpotLightTransform(const Graphics::ContextEntityId id, const Math::matrix44& transform);
	/// set transform, type must match the type the entity was created with
	static void SetPointLightTransform(const Graphics::ContextEntityId id, const Math::matrix44& transform);
	/// set global light transform
	static void SetGlobalLightTransform(const Graphics::ContextEntityId id, const Math::matrix44& transform);
	/// set global light shadow transform
	static void SetGlobalLightViewProjTransform(const Graphics::ContextEntityId id, const Math::matrix44& transform);

	/// run light classification compute
	static void UpdateClustersAndCull();
	/// render lights
	static void RenderLights();
	/// run shadow map blurring
	static void BlurGlobalShadowMap();

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
		PointLight_Transform,
		PointLight_ConstantBufferSet,
		PointLight_ShadowConstantBufferSet,
		PointLight_DynamicOffsets,
		PointLight_ProjectionTexture
	};

	typedef Ids::IdAllocator<
		Math::matrix44,			// transform
		ConstantBufferSet,		// constant buffer binding for light
		ConstantBufferSet,		// constant buffer binding for shadows
		Util::FixedArray<uint>,	// dynamic offsets
		CoreGraphics::TextureId // projection (if invalid, don't use)
	> PointLightAllocator;
	static PointLightAllocator pointLightAllocator;

	enum
	{
		SpotLight_Transform,
		SpotLight_Projection,
		SpotLight_InvViewProjection,
		SpotLight_ConstantBufferSet,
		SpotLight_ShadowConstantBufferSet,
		SpotLight_DynamicOffsets,
		SpotLight_ConeAngles,
		SpotLight_ProjectionTexture,
	};

	typedef Ids::IdAllocator<
		Math::matrix44,				// transform
		Math::matrix44,				// projection
		Math::matrix44,				// inversed view-projection
		ConstantBufferSet,			// constant buffer binding for light
		ConstantBufferSet,			// constant buffer binding for shadows
		Util::FixedArray<uint>,		// dynamic offsets
		std::array<float, 2>,		// cone angle
		CoreGraphics::TextureId		// projection (if invalid, don't use)
	> SpotLightAllocator;
	static SpotLightAllocator spotLightAllocator;

	enum
	{
		GlobalLight_Direction,
		GlobalLight_Backlight,
		GlobalLight_BacklightOffset,
		GlobalLight_Ambient,
		GlobalLight_Transform,
		GlobalLight_ViewProjTransform,
	};
	typedef Ids::IdAllocator<
		Math::float4,			// direction
		Math::float4,			// backlight color
		float,					// backlight offset
		Math::float4,			// ambient
		Math::matrix44,			// transform (basically just a rotation in the direction)
		Math::matrix44			// transform for visibility and such
	> GlobalLightAllocator;
	static GlobalLightAllocator globalLightAllocator;

	/// allocate a new slice for this context
	static Graphics::ContextEntityId Alloc();
	/// deallocate a slice
	static void Dealloc(Graphics::ContextEntityId id);
};
} // namespace Lighting