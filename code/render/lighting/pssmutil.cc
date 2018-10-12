//------------------------------------------------------------------------------
//  pssmutil.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "lighting/pssmutil.h"
#include "debugrender/debugshaperenderer.h"
#include "coregraphics/rendershape.h"
#include "coregraphics/shaperenderer.h"
#include "threading/thread.h"

namespace Lighting
{

using namespace Graphics;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
PSSMUtil::PSSMUtil() :
    maxShadowDistance(200.0f),
    lightDir(0.0f, 1.0f, 0.0f),
	renderDebug(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
PSSMUtil::Compute()
{
    this->CalculateSplitDistances();
    IndexT splitIndex;
    for (splitIndex = 0; splitIndex < NumSplits; splitIndex++)
    {
        this->CalculateFrustumPoints(splitIndex);
        this->CalculateTransforms(splitIndex);
    }
}

//------------------------------------------------------------------------------
/**
    Calculate the split distances for the view frustum splits.
*/
void
PSSMUtil::CalculateSplitDistances()
{
    // lambda scales between between logarithmic and uniform split scheme
    const float splitDistanceLambda = 0.3f;

    // practical split scheme:
    //
    // CLi = n*(f/n)^(i/numsplits)
    // CUi = n + (f-n) * (i/numsplits)
    // Ci  = lerp(CLi, CUi, lambda)
    
    float zNear = this->cameraEntity->GetCameraSettings().GetZNear();
    float zFar  = this->maxShadowDistance;
    IndexT i;
    for (i = 0; i < NumSplits; i++)
    {
        float idm = i / float(NumSplits);
        float distLog = zNear * n_pow(zFar / zNear, idm);
        float distUniform = zNear + (zFar - zNear) * idm;
        this->splitDistances[i] = n_lerp(distLog, distUniform, splitDistanceLambda);
    }

    // clamp border values
    this->splitDistances[0] = zNear;
    this->splitDistances[NumSplits] = zFar;
}

//------------------------------------------------------------------------------
/**
    Calculate the corner points of a given view frustum split in world space.
    Result is in frustumCornerPoints member array.
*/
void
PSSMUtil::CalculateFrustumPoints(IndexT splitIndex)
{
    const matrix44& view  = this->cameraEntity->GetTransform();
    float zNear           = this->splitDistances[splitIndex];
    float zFar            = this->splitDistances[splitIndex + 1];

    const CameraSettings& camSettings = this->cameraEntity->GetCameraSettings();
    float cameraNormWidth  = camSettings.GetNearWidth() / camSettings.GetZNear();
    float cameraNormHeight = camSettings.GetNearHeight() / camSettings.GetZNear();

    float nearWidth       = cameraNormWidth * zNear;
    float nearHeight      = cameraNormHeight * zNear;
    float farWidth        = cameraNormWidth * zFar;
    float farHeight       = cameraNormHeight * zFar;

    point viewPos = view.get_position();
    vector viewX  = view.get_xaxis();
    vector viewY  = view.get_yaxis();
    vector viewZ  = -view.get_zaxis();

    point nearPlaneCenter = viewPos + viewZ * zNear;
    point farPlaneCenter  = viewPos + viewZ * zFar;    

    // near plane rectangle
    this->frustumCorners[0] = nearPlaneCenter - viewX * nearWidth - viewY * nearHeight;
    this->frustumCorners[1] = nearPlaneCenter - viewX * nearWidth + viewY * nearHeight;
    this->frustumCorners[2] = nearPlaneCenter + viewX * nearWidth + viewY * nearHeight;
    this->frustumCorners[3] = nearPlaneCenter + viewX * nearWidth - viewY * nearHeight;

    // far plane rectangle
    this->frustumCorners[4] = farPlaneCenter - viewX * farWidth - viewY * farHeight;
    this->frustumCorners[5] = farPlaneCenter - viewX * farWidth + viewY * farHeight;
    this->frustumCorners[6] = farPlaneCenter + viewX * farWidth + viewY * farHeight;
    this->frustumCorners[7] = farPlaneCenter + viewX * farWidth - viewY * farHeight;

    // scale frustum points outwards to avoid edge problems
    const float grow = 0.1f;
    this->frustumCenter.set(0.0f, 0.0f, 0.0f, 0.0f);
    IndexT i;
    for (i = 0; i < NumCorners; i++)
    {
        this->frustumCenter += this->frustumCorners[i];
    }
    this->frustumCenter *= 1.0f / float(NumCorners);
    for (i = 0; i < NumCorners; i++)
    {
        this->frustumCorners[i] += (this->frustumCorners[i] - this->frustumCenter) * grow;
    }
}

//------------------------------------------------------------------------------
/**
    Calculate the "best" LightProj matrix for the current set of
    frustum corner points.
*/
void
PSSMUtil::CalculateTransforms(IndexT splitIndex)
{
    n_assert(splitIndex < NumSplits);

    // first compute a temporary light-view matrix
    // NOTE: we use left-handed convention so that +z points "into" the screen
    point lightTarget = this->frustumCenter;
    point lightSource = lightTarget + this->lightDir;
    matrix44 lightView = matrix44::inverse(matrix44::lookatlh(lightSource, lightTarget, vector::upvec()));

    // transform frustum corners to temp light space to get the
    // aligned size of the frustum bounding box for determining
    // the final "position" of the light source (must be outside
    // the frustum box)
    float4 pMax(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);
    float4 pMin(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
    IndexT i;
    for (i = 0; i < NumCorners; i++)
    {
        float4 p = matrix44::transform(this->frustumCorners[i], lightView);
        pMax = float4::maximize(pMax, p);
        pMin = float4::minimize(pMin, p);
    }
	//n_assert(pMin.z() < 0.0f);

    // compute a final light-view matrix which is shifted backward
    // outside of the frustum box
    // NOTE: this distance and the new nearPlane also determine
    // which shadow casters outside of the view frustum box will
    // be rendered into the shadow map, but the higher the 
    // rendered z-range the lower the resulting precision in the
    // shadow map!
    float minDist = -pMin.z() + 10.0f;
    lightSource = lightTarget + (lightDir * minDist);
	lightView = matrix44::inverse(matrix44::lookatlh(lightSource, lightTarget, vector::upvec()));
    
    // compute the frustum bounding box in the final view space 
    // to determine the orthogonal projection matrix
    pMax.set(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);
    pMin.set(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
    for (i = 0; i < NumCorners; i++)
    {
        float4 p = matrix44::transform(this->frustumCorners[i], lightView);
        pMax = float4::maximize(pMax, p);
        pMin = float4::minimize(pMin, p);
    }
    //n_assert(pMin.z() > 0.0f);
    //n_assert(pMax.z() > pMin.z());


    // create an offcenter orthogonal projection matrix which
    // tightly encloses the current view frustum slice
    float l = pMin.x();
    float r = pMax.x();
    float b = pMin.y();
    float t = pMax.y();
    float zn = pMin.z();
    float zf = pMax.z();
    matrix44 lightProj = matrix44::orthooffcenterlh(l, r, b, t, zn, zf);

	lightView.set_zaxis(-lightView.get_zaxis());

    // compute lightViewProj matrix (transforms points from world space to projection space)
    matrix44 lightViewProj = matrix44::multiply(lightView, lightProj);

    // finally store computed matrices
    this->splitViewTransforms[splitIndex]     = lightView;
    this->splitProjTransforms[splitIndex]     = lightProj;
    this->splitViewProjTransforms[splitIndex] = lightViewProj;
}


//------------------------------------------------------------------------------
/**
*/
void 
PSSMUtil::UpdateSplitInvLightProjTransforms( const Math::matrix44& invView )
{
	IndexT splitIndex;
	for (splitIndex = 0; splitIndex < NumSplits; splitIndex++)
	{
		this->splitInvViewProjTransforms[splitIndex] = matrix44::multiply(invView, this->splitViewProjTransforms[splitIndex]);
	}
}


//------------------------------------------------------------------------------
/**
*/
void 
PSSMUtil::RenderDebug(IndexT splitIndex)
{
	/*
	float4 frustumLines[24] = {

		// near plane
		float4(-1,-1,0,1),	float4(-1,1,0,1),
		float4(-1,1,0,1),	float4(1,1,0,1),
		float4(1,1,0,1),	float4(1,-1,0,1),
		float4(1,-1,0,1),	float4(-1,-1,0,1),		

		// far plane
		float4(-1,-1,-1,1),	float4(-1,1,-1,1),
		float4(-1,1,-1,1),	float4(1,1,-1,1),
		float4(1,1,-1,1),	float4(1,-1,-1,1),
		float4(1,-1,-1,1),	float4(-1,-1,-1,1),		
		
		// far-near plane
		float4(-1,-1,0,1),	float4(-1,-1,-1,1),
		float4(-1,1,0,1),	float4(-1,1,-1,1),
		float4(1,1,0,1),	float4(1,1,-1,1),
		float4(1,-1,0,1),	float4(1,-1,-1,1),
		
	};
	
	CoreGraphics::RenderShape debugShape;


	debugShape.SetupPrimitives(Threading::Thread::GetMyThreadId(), 
							   this->splitViewTransforms[splitIndex], 
							   CoreGraphics::PrimitiveTopology::LineList, 
							   12, 
							   frustumLines, 
							   4, 
							   float4(1,0,0,1), 
							   CoreGraphics::RenderShape::AlwaysOnTop);

	CoreGraphics::ShapeRenderer::Instance()->AddShape(debugShape);
	*/
}

} // namespace Lighting
