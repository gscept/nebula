#pragma once
//------------------------------------------------------------------------------
/**
	A camera represents a viewing point into a Stage by attachment through a View
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
namespace Graphics
{
class Camera : public Core::RefCounted
{
	__DeclareClass(Camera);
public:
	/// constructor
	Camera();
	/// destructor
	virtual ~Camera();

	/// set camera projection using field-of-view and aspect
	void SetProjectionFov(float aspect, float fov, float znear, float zfar);
	/// set camera projection off-center
	void SetProjection(float left, float right, float top, float bottom, float znear, float zfar);
	/// set camera as orthogonal projection
	void SetOrthogonal(float width, float height);
	/// set camera as orthogonal projection off-center
	void SetOrthogonal(float left, float right, float top, float bottom);

	/// set transform (moves camera but doesn't affect projection)
	void SetTransform(const Math::matrix44& transform);

private:
	Math::matrix44 view;
	Math::matrix44 projection;
};
} // namespace Graphics