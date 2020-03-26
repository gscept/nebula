#pragma once
//------------------------------------------------------------------------------
/**
    The Decal context manages the decal system and entities and rendering.

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
namespace Decals
{

class DecalContext : public Graphics::GraphicsContext
{
	_DeclareContext();
public:
	/// constructor
	DecalContext();
	/// destructor
	virtual ~DecalContext();

	/// setup decal context
	static void Create();
	/// discard decal context
	static void Discard();

	/// setup as albedo-normal-material decal
	static void SetupDecal(
		const Graphics::GraphicsEntityId id, 
		const Math::matrix44 transform,
		const CoreGraphics::TextureId albedo, 
		const CoreGraphics::TextureId normal, 
		const CoreGraphics::TextureId material);
	/// setup as emissive decal
	static void SetupDecal(
		const Graphics::GraphicsEntityId id,
		const Math::matrix44 transform, 
		const CoreGraphics::TextureId emissive);

	/// set transform of decal
	static void SetTransform(const Graphics::GraphicsEntityId id, const Math::matrix44 transform);

	/// update view dependent resources
	static void UpdateViewDependentResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);

#ifndef PUBLIC_BUILD
	/// render debug
	static void OnRenderDebug(uint32_t flags);
#endif

private:

	/// cull decals towards camera cluster and classify
	static void CullAndClassify();
	/// render PBR decals
	static void RenderPBR();
	/// render emissive decals
	static void RenderEmissive();

	enum DecalType
	{
		PBRDecal,
		EmissiveDecal
	};
	enum
	{
		Decal_Transform,
		Decal_Type,
		Decal_TypedId
	};
	typedef Ids::IdAllocator<
		Math::matrix44,
		DecalType,
		Ids::Id32
	> GenericDecalAllocator;
	static GenericDecalAllocator genericDecalAllocator;

	enum
	{
		DecalPBR_Albedo,
		DecalPBR_Normal,
		DecalPBR_Material,
	};
	typedef Ids::IdAllocator<
		CoreGraphics::TextureId,
		CoreGraphics::TextureId,
		CoreGraphics::TextureId
	> PBRDecalAllocator;
	static PBRDecalAllocator pbrDecalAllocator;

	enum
	{
		DecalEmissive_Emissive,
	};
	typedef Ids::IdAllocator<
		CoreGraphics::TextureId
	> EmissiveDecalAllocator;
	static EmissiveDecalAllocator emissiveDecalAllocator;

	/// allocate a new slice for this context
	static Graphics::ContextEntityId Alloc();
	/// deallocate a slice
	static void Dealloc(Graphics::ContextEntityId id);

};
} // namespace Decals
