#pragma once
//------------------------------------------------------------------------------
/**
    @class Lighting::PSSMUtil

    Helper class which compute LightProj matrices for Parallel-Split-
    Shadowmap rendering.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "math/matrix44.h"
#include "math/vector.h"
#include "math/bbox.h"
#include "util/fixedarray.h"
#include "graphics/cameraentity.h"

// fixme: maybe put it in a more central file
#ifndef FLT_MAX
#define FLT_MAX 3.402823466e+38F
#endif
//------------------------------------------------------------------------------
namespace Lighting
{
class PSSMUtil
{
public:
    /// constructor
    PSSMUtil();
    /// set camera entity which defines the view and projection transform
    void SetCameraEntity(const Ptr<Graphics::CameraEntity>& camera);
    /// get camera entity
    const Ptr<Graphics::CameraEntity>& GetCameraEntity() const;
    /// set light direction
    void SetLightDir(const Math::vector& dir);
    /// get light direction
    const Math::vector& GetLightDir() const;

    /// compute PSSM split volumes
    void Compute();

    /// get view matrix for a view frustum split (valid after Compute)
    const Math::matrix44& GetSplitViewTransform(IndexT splitIndex) const;
    /// get projection transform for a view frustum split (valid after Compute)
    const Math::matrix44& GetSplitProjTransform(IndexT splitIndex) const;
    /// get light projection transform for given frustum split (valid after Compute())
    const Math::matrix44& GetSplitViewProjTransform(IndexT splitIndex) const;
	

    /// get raw pointer to split distances
    const float* GetSplitDistances() const;
    /// get raw pointer to LightProjTransforms
    const Math::matrix44* GetSplitViewProjTransforms() const;
	/// gets inversed light projection matrices
	const Math::matrix44* GetSplitInvViewProjTransforms() const;
	/// get pointer to far plane
	const Math::float4* GetFarPlane() const;
	/// get pointer to near plane
	const Math::float4* GetNearPlane() const;

	/// updates inversed view projection matrices
	void UpdateSplitInvLightProjTransforms(const Math::matrix44& invView);

	/// sets if the PSSM util should render debug data
	void SetRenderDebug(bool state);

    /// number of view volume splits
    static const SizeT NumSplits = 4;

private:


	/// renders PSSM as a debug shape
	void RenderDebug(IndexT splitIndex);

    /// calculate split distances in projection space
    void CalculateSplitDistances();
    /// calculate corner points of a frustum split
    void CalculateFrustumPoints(IndexT splitIndex);
    /// calculate "best" light projection matrices for current frustum corner points
    void CalculateTransforms(IndexT splitIndex);


	bool renderDebug;
    static const SizeT NumCorners = 8;
    float maxShadowDistance;
    Ptr<Graphics::CameraEntity> cameraEntity;
    Math::vector lightDir;

    float splitDistances[NumSplits + 1];
    Math::float4 frustumCenter;
    Math::float4 frustumCorners[NumCorners];
    Math::matrix44 splitViewTransforms[NumSplits];
    Math::matrix44 splitProjTransforms[NumSplits];
    Math::matrix44 splitViewProjTransforms[NumSplits];
	Math::matrix44 splitInvViewProjTransforms[NumSplits];
};

//------------------------------------------------------------------------------
/**
*/
inline void
PSSMUtil::SetCameraEntity(const Ptr<Graphics::CameraEntity>& camera)
{
    this->cameraEntity = camera;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Graphics::CameraEntity>&
PSSMUtil::GetCameraEntity() const
{
    return this->cameraEntity;
}

//------------------------------------------------------------------------------
/**
*/
inline void
PSSMUtil::SetLightDir(const Math::vector& v)
{
    this->lightDir = v;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::vector&
PSSMUtil::GetLightDir() const
{
    return this->lightDir;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44&
PSSMUtil::GetSplitViewProjTransform(IndexT i) const
{
    n_assert(i < NumSplits);
    return this->splitViewProjTransforms[i];
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44&
PSSMUtil::GetSplitViewTransform(IndexT i) const
{
    n_assert(i < NumSplits);
    return this->splitViewTransforms[i];
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44&
PSSMUtil::GetSplitProjTransform(IndexT i) const
{
    n_assert(i < NumSplits);
    return this->splitProjTransforms[i];
}

//------------------------------------------------------------------------------
/**
*/
inline const float*
PSSMUtil::GetSplitDistances() const
{
    return this->splitDistances;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44*
PSSMUtil::GetSplitViewProjTransforms() const
{
    return this->splitViewProjTransforms;
}


//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44* 
PSSMUtil::GetSplitInvViewProjTransforms() const
{
	return this->splitInvViewProjTransforms;
}

//------------------------------------------------------------------------------
/**
	Returns pointer to corners with offset to far plane
*/
inline const Math::float4* 
PSSMUtil::GetFarPlane() const
{
	return &this->frustumCorners[4];
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::float4* 
PSSMUtil::GetNearPlane() const
{
	return &this->frustumCorners[0];
}


//------------------------------------------------------------------------------
/**
*/
inline void 
PSSMUtil::SetRenderDebug( bool state )
{
	this->renderDebug = state;
}
} // namespace Lighting
//------------------------------------------------------------------------------