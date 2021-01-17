#pragma once
//------------------------------------------------------------------------------
/**
    @class Lighting::CSMUtil
    
    Helper class for creating and rendering Cascading Shadow Maps
    
    @copyright
    (C) 2012-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "math/frustum.h"
#include "math/mat4.h"
#include "graphics/graphicsentity.h"

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
    void SetCameraEntity(const Graphics::GraphicsEntityId camera);
    /// get the camera entity
    const Graphics::GraphicsEntityId GetCameraEntity() const;
    /// sets the scene bounding box
    void SetShadowBox(const Math::bbox& sceneBox);
    /// sets the global light entity
    void SetGlobalLight(const Graphics::GraphicsEntityId globalLight);
    /// gets the global light entity
    const Graphics::GraphicsEntityId GetGlobalLight() const;
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
    const Math::mat4& GetCascadeViewProjection(IndexT cascadeIndex) const;
    /// gets the shadow view transform (valid after Compute)
    const Math::mat4& GetShadowView() const;
    /// returns raw pointer to array of cascade transforms
    const Math::mat4* GetCascadeProjectionTransforms() const;
    /// returns raw pointer to array of cascade distances
    const float* GetCascadeDistances() const;

    /// gets cascade debug camera (only valid after Compute, and if the debug flag is set)
    const Math::mat4& GetCascadeCamera(IndexT index)  const;

    /// computes the splits
    void Compute(const Graphics::GraphicsEntityId camera, const Graphics::GraphicsEntityId light);

    static const SizeT NumCascades = 4;

private:

    struct CascadeFrustum
    {
        float rightSlope;           // positive X-slope (X/Z)
        float leftSlope;            // negative X-slope
        float topSlope;             // positive Y-slope (Y/Z)
        float bottomSlope;          // negative Y-slope
        float nearPlane, farPlane;  // Z of near and far plane
    };

    struct Triangle
    {
        Math::vec4 pt[3];
        bool culled;
    };

    /// computes frustum corners from cascade
    void ComputeFrustumPoints(float cascadeBegin, float cascadeEnd, const Math::mat4& projection, Math::vec4* frustumCorners);
    /// computes frustum from projection
    void ComputeFrustum(CSMUtil::CascadeFrustum& frustum, const Math::mat4& projection);
    /// computes near and far values
    void ComputeNearAndFar(float& nearPlane, float& farPlane, const Math::vec4& lightCameraOrtoMin, const Math::vec4& lightCameraOrtoMax, const Math::vec4* lightAABBPoints);
    /// computes light-space AABB points
    void ComputeAABB(Math::vec4* lightAABBPoints, const Math::vec4& sceneCenter, const Math::vec4& sceneExtents);

    Math::bbox shadowBox;
    Math::vec4 frustumCenter;
    Graphics::GraphicsEntityId globalLight;
    Graphics::GraphicsEntityId cameraEntity;
    Math::mat4 cascadeProjectionTransform[NumCascades];
    Math::mat4 cascadeViewProjectionTransform[NumCascades];
    Math::mat4 shadowView;
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
CSMUtil::SetCameraEntity( const Graphics::GraphicsEntityId camera )
{
    this->cameraEntity = camera;
}

//------------------------------------------------------------------------------
/**
*/
inline const Graphics::GraphicsEntityId
CSMUtil::GetCameraEntity() const
{
    return this->cameraEntity;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
CSMUtil::SetGlobalLight( const Graphics::GraphicsEntityId globalLight )
{
    this->globalLight = globalLight;
}

//------------------------------------------------------------------------------
/**
*/
inline const Graphics::GraphicsEntityId
CSMUtil::GetGlobalLight() const
{
    return this->globalLight;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
CSMUtil::GetCascadeViewProjection( IndexT cascadeIndex ) const
{
    return this->cascadeViewProjectionTransform[cascadeIndex];
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
CSMUtil::GetShadowView() const
{
    return this->shadowView;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4*
CSMUtil::GetCascadeProjectionTransforms() const
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