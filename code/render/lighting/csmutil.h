#pragma once
//------------------------------------------------------------------------------
/**
    @class Lighting::CSMUtil
    
    Helper class for creating and rendering Cascading Shadow Maps
    
    (C) 2012-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "math/frustum.h"
#include "math/matrix44.h"
#include "graphics/globallightentity.h"
#include "graphics/cameraentity.h"

#ifndef FLT_MAX
#define FLT_MAX 3.402823466e+38F
#endif

#ifndef FLT_MIN
#define FLT_MIN 1.175494351e-38F
#endif

//------------------------------------------------------------------------------
namespace Lighting
{
class CSMUtil
{
public:

	enum FittingMethod
	{
		Cascade,
		Scene,

		NumFittingMethods
	};

	enum ClampingMethod
	{
		ZeroOne,
		AABB,
		SceneAABB,

		NumClampingMethods
	};

	/// constructor
	CSMUtil();
	/// destructor
	virtual ~CSMUtil();

	/// sets the camera entity
	void SetCameraEntity(const Ptr<Graphics::CameraEntity>& camera);
	/// get the camera entity
	const Ptr<Graphics::CameraEntity>& GetCameraEntity() const;
	/// sets the scene bounding box
	void SetShadowBox(const Math::bbox& sceneBox);
	/// sets the global light entity
	void SetGlobalLight(const Ptr<Graphics::GlobalLightEntity>& globalLight);
	/// gets the global light entity
	const Ptr<Graphics::GlobalLightEntity>& GetGlobalLight() const;
	/// sets the texture width for the CSM texture buffer
	void SetTextureWidth(int width);
	/// sets the CSM fitting method
	void SetFittingMethod(FittingMethod method);
	/// sets the CSM clamping method
	void SetClampingMethod(ClampingMethod method);
	/// sets the CSM blur size
	void SetBlurSize(int size);
	/// sets whether or not the CSM should clamp the cascades to fit the size of the texels
	void SetFitTexels(bool state);

	/// gets computed view projection transform (valid after Compute)
	const Math::matrix44& GetCascadeViewProjection(IndexT cascadeIndex) const;
	/// gets the shadow view transform (valid after Compute)
	const Math::matrix44& GetShadowView() const;
	/// returns raw pointer to array of cascade transforms
	const Math::matrix44* GetCascadeTransforms() const;
	/// returns raw pointer to array of cascade distances
	const float* GetCascadeDistances() const;

	/// gets cascade debug camera (only valid after Compute, and if the debug flag is set)
	const Math::matrix44& GetCascadeCamera(IndexT index)  const;

	/// computes the splits
	void Compute();

	static const SizeT NumCascades = 4;

private:

	struct CascadeFrustum
	{
		float rightSlope;			// positive X-slope (X/Z)
		float leftSlope;			// negative X-slope
		float topSlope;				// positive Y-slope (Y/Z)
		float bottomSlope;			// negative Y-slope
		float nearPlane, farPlane;	// Z of near and far plane
	};

	struct Triangle
	{
		Math::float4 pt[3];
		bool culled;
	};

	/// computes frustum corners from cascade
	void ComputeFrustumPoints(float cascadeBegin, float cascadeEnd, const Math::matrix44& projection, Math::float4* frustumCorners);
	/// computes frustum from projection
	void ComputeFrustum(CSMUtil::CascadeFrustum& frustum, const Math::matrix44& projection);
	/// computes near and far values
	void ComputeNearAndFar(float& nearPlane, float& farPlane, const Math::float4& lightCameraOrtoMin, const Math::float4& lightCameraOrtoMax, const Math::float4* lightAABBPoints);
	/// computes light-space AABB points
	void ComputeAABB(Math::float4* lightAABBPoints, const Math::float4& sceneCenter, const Math::float4& sceneExtents);

	Math::bbox shadowBox;
	Math::float4 frustumCenter;
	Ptr<Graphics::GlobalLightEntity> globalLight;
	Ptr<Graphics::CameraEntity> cameraEntity;
	Math::matrix44 cascadeProjectionTransform[NumCascades];
	Math::matrix44 cascadeViewProjectionTransform[NumCascades];
	Math::matrix44 shadowView;
	float cascadeDistances[NumCascades];
	float intervalDistances[NumCascades];
	float cascadeMaxDistance;
	
	int fittingMethod;
	int clampingMethod;
	int blurSize;
	int textureWidth;
	bool floorTexels;
}; 

//------------------------------------------------------------------------------
/**
*/
inline void 
CSMUtil::SetShadowBox( const Math::bbox& shadowRange )
{
	this->shadowBox = shadowRange;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
CSMUtil::SetCameraEntity( const Ptr<Graphics::CameraEntity>& camera )
{
	this->cameraEntity = camera;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Graphics::CameraEntity>& 
CSMUtil::GetCameraEntity() const
{
	return this->cameraEntity;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
CSMUtil::SetGlobalLight( const Ptr<Graphics::GlobalLightEntity>& globalLight )
{
	this->globalLight = globalLight;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Graphics::GlobalLightEntity>& 
CSMUtil::GetGlobalLight() const
{
	return this->globalLight;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44& 
CSMUtil::GetCascadeViewProjection( IndexT cascadeIndex ) const
{
	return this->cascadeViewProjectionTransform[cascadeIndex];
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44& 
CSMUtil::GetShadowView() const
{
	return this->shadowView;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44* 
CSMUtil::GetCascadeTransforms() const
{
	return this->cascadeProjectionTransform;
}

//------------------------------------------------------------------------------
/**
*/
inline const float* 
CSMUtil::GetCascadeDistances() const
{
	return this->intervalDistances;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
CSMUtil::SetTextureWidth( int width )
{
	this->textureWidth = width;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
CSMUtil::SetFittingMethod( FittingMethod method )
{
	this->fittingMethod = method;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
CSMUtil::SetClampingMethod( ClampingMethod method )
{
	this->clampingMethod = method;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
CSMUtil::SetBlurSize( int size )
{
	this->blurSize = size;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
CSMUtil::SetFitTexels( bool state )
{
	this->floorTexels = state;
}

} // namespace Lighting
//------------------------------------------------------------------------------