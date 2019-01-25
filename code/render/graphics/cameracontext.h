#pragma once
//------------------------------------------------------------------------------
/**
	Handles cameras

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "camerasettings.h"
#include "math/scalar.h"
namespace Graphics
{

class CameraContext : public Graphics::GraphicsContext
{
	_DeclareContext();
public:

	/// constructor
	CameraContext();
	/// destructor
	virtual ~CameraContext();

	/// create context
	static void Create();

	/// runs before frame is updated
	static void OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime, const Timing::Time time, const Timing::Tick ticks);

	/// setup as projection and fov
	static void SetupProjectionFov(const Graphics::GraphicsEntityId id, float aspect, float fov, float znear, float zfar);
	/// setup as ortographic
	static void SetupOrthographic(const Graphics::GraphicsEntityId id, float width, float height, float znear, float zfar);

	/// set transform
	static void SetTransform(const Graphics::GraphicsEntityId id, const Math::matrix44& mat);
	/// get transform
	static const Math::matrix44& GetTransform(const Graphics::GraphicsEntityId id);

	/// get projection
	static const Math::matrix44& GetProjection(const Graphics::GraphicsEntityId id);
	/// get view-projection
	static const Math::matrix44& GetViewProjection(const Graphics::GraphicsEntityId id);
	/// get settings
	static const CameraSettings& GetSettings(const Graphics::GraphicsEntityId id);

private:
	typedef Ids::IdAllocator<
		Graphics::CameraSettings,
		Math::matrix44,				// projection
		Math::matrix44,				// view-transform
		Math::matrix44				// view-projection
	> CameraAllocator;

	static CameraAllocator cameraAllocator;

	/// allocate a new slice for this context
	static Graphics::ContextEntityId Alloc();
	/// deallocate a slice
	static void Dealloc(const Graphics::ContextEntityId id);
};

//------------------------------------------------------------------------------
/**
*/
inline Graphics::ContextEntityId
CameraContext::Alloc()
{
	return cameraAllocator.AllocObject();
}

//------------------------------------------------------------------------------
/**
*/
inline void
CameraContext::Dealloc(const Graphics::ContextEntityId id)
{
	cameraAllocator.DeallocObject(id.id);
}

} // namespace Graphics
