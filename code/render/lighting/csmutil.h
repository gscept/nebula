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

    /// constructor
    CSMUtil(const uint numCascades);
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
    /// sets the CSM blur size
    void SetBlurSize(int size);
    /// sets whether or not the CSM should clamp the cascades to fit the size of the texels
    void SetFitTexels(bool state);

    /// gets computed view projection transform (valid after Compute)
    const Math::mat4& GetCascadeViewProjection(IndexT cascadeIndex) const;
    /// gets the shadow view transform (valid after Compute)
    const Math::mat4& GetShadowView() const;
    /// returns raw pointer to array of cascade transforms
    const Util::FixedArray<Math::mat4> GetCascadeProjectionTransforms() const;
    /// returns raw pointer to array of cascade distances
    const Util::FixedArray<float> GetCascadeDistances() const;

    /// gets cascade debug camera (only valid after Compute, and if the debug flag is set)
    const Math::mat4& GetCascadeCamera(IndexT index)  const;

    /// computes the splits
    void Compute(const Graphics::GraphicsEntityId camera, const Graphics::GraphicsEntityId light);

private:

    struct CascadeFrustum
    {
        float rightSlope;           // positive X-slope (X/Z)
        float leftSlope;            // negative X-slope
        float topSlope;             // positive Y-slope (Y/Z)
        float bottomSlope;          // negative Y-slope
        float nearPlane, farPlane;  // Z of near and far plane
    };


    Math::bbox shadowBox;
    Math::vec4 frustumCenter;
    Graphics::GraphicsEntityId globalLight;
    Graphics::GraphicsEntityId cameraEntity;

    uint numCascades;
    Util::FixedArray<Math::mat4> cascadeProjectionTransform, cascadeViewProjectionTransform;
    Util::FixedArray<float> cascadeDistances, intervalDistances;
    Math::mat4 shadowView;
    float cascadeMaxDistance;
    
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
inline const Util::FixedArray<Math::mat4>
CSMUtil::GetCascadeProjectionTransforms() const
{
    return this->cascadeProjectionTransform;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::FixedArray<float>
CSMUtil::GetCascadeDistances() const
{
    return this->intervalDistances;
}

} // namespace Lighting
//------------------------------------------------------------------------------
