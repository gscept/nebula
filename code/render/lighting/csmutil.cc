//------------------------------------------------------------------------------
//  csmutil.cc
//  (C) 2012-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "lighting/csmutil.h"
#include "graphics/camerasettings.h"
#include "graphics/cameracontext.h"
#include "lighting/lightcontext.h"

using namespace Math;
using namespace Graphics;
using namespace CoreGraphics;

static const vec4 halfVector = vec4( 0.5f );
static const vec4 multiplyZWToZero = vec4( 1.0f, 1.0f, 0.0f, 0.0f );

namespace Lighting
{
//------------------------------------------------------------------------------
/**
*/
CSMUtil::CSMUtil(const uint numCascades) :
    numCascades(numCascades),
    cascadeMaxDistance(300)
{
    this->cascadeProjectionTransform.Resize(numCascades);
    this->cascadeViewProjectionTransform.Resize(numCascades);
    this->cascadeDistances.Resize(numCascades);
    this->intervalDistances.Resize(numCascades);
    this->cascadeDistances[0] = 15;
    this->cascadeDistances[1] = 50;
    this->cascadeDistances[2] = 120;
    this->cascadeDistances[3] = 300;
}

//------------------------------------------------------------------------------
/**
*/
CSMUtil::~CSMUtil()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Compute frustum points using computed frustum and cascade intervals,
    basically stolen from Math::frustum
*/
void 
ComputeFrustumPoints( float cascadeBegin, float cascadeEnd, const float aspect, const float fov, Math::vec4* frustumCorners )
{
    float tanHalfHFOV = tanf((fov * aspect) / 2.0f);
    float tanHalfVFOV = tanf(fov / 2.0f);
    float xNear = cascadeBegin * tanHalfHFOV;
    float xFar = cascadeEnd * tanHalfHFOV;
    float yNear = cascadeBegin * tanHalfVFOV;
    float yFar = cascadeEnd * tanHalfVFOV;

    // frustum corners in projection space
    frustumCorners[0].set( xNear,  yNear, -cascadeBegin, 1.0f);
    frustumCorners[1].set(-xNear,  yNear, -cascadeBegin, 1.0f);
    frustumCorners[2].set( xNear, -yNear, -cascadeBegin, 1.0f);
    frustumCorners[3].set(-xNear, -yNear, -cascadeBegin, 1.0f);

    frustumCorners[4].set( xFar,  yFar, -cascadeEnd, 1.0f);
    frustumCorners[5].set(-xFar,  yFar, -cascadeEnd, 1.0f);
    frustumCorners[6].set( xFar, -yFar, -cascadeEnd, 1.0f);
    frustumCorners[7].set(-xFar, -yFar, -cascadeEnd, 1.0f);
}

//------------------------------------------------------------------------------
/**
*/
void
ComputeAABB(vec4* lightAABBPoints, const vec4& sceneCenter, const vec4& sceneExtents)
{
    static const vec4 extentsMap[] =
    {
        vec4(1.0f, 1.0f, -1.0f, 1.0f),
        vec4(-1.0f, 1.0f, -1.0f, 1.0f),
        vec4(1.0f, -1.0f, -1.0f, 1.0f),
        vec4(-1.0f, -1.0f, -1.0f, 1.0f),
        vec4(1.0f, 1.0f, 1.0f, 1.0f),
        vec4(-1.0f, 1.0f, 1.0f, 1.0f),
        vec4(1.0f, -1.0f, 1.0f, 1.0f),
        vec4(-1.0f, -1.0f, 1.0f, 1.0f)
    };

    for (int index = 0; index < 8; ++index)
    {
        lightAABBPoints[index] = multiplyadd(extentsMap[index], sceneExtents, sceneCenter);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
CSMUtil::Compute(const Graphics::GraphicsEntityId camera, const Graphics::GraphicsEntityId light)
{
    n_assert(this->cameraEntity != Graphics::GraphicsEntityId::Invalid());
    const CameraSettings& camSettings = Graphics::CameraContext::GetSettings(camera);
    float aspect = camSettings.GetAspect();
    float fov = camSettings.GetFov();
    mat4 cameraTransform = Graphics::CameraContext::GetTransform(camera);

    // Get inversed shadow matrix, this is basically the global light transform, normalized and inversed
    mat4 lightView = inverse(Lighting::LightContext::GetTransform(light));

    mat4 viewToLight = lightView * cameraTransform;
    
    float intervalStart = 0;
    float intervalEnd = 0;

    for (int cascadeIndex = 0; cascadeIndex < this->numCascades; ++cascadeIndex)
    {
        intervalEnd = cascadeDistances[cascadeIndex];

        // Calculate frustum points, it will end up in a frustum in view space
        vec4 frustumPoints[8];
        ComputeFrustumPoints(intervalStart, intervalEnd, aspect, fov, frustumPoints);
        vec4 lightCameraOrthographicMin = vec4(FLT_MAX);
        vec4 lightCameraOrthographicMax = vec4(-FLT_MAX);

        intervalStart = intervalEnd;

        for (int icpIndex = 0; icpIndex < 8; ++icpIndex)
        {
            // Transform camera point to light space
            vec4 tempPoint = viewToLight * frustumPoints[icpIndex];

            // Find AABB corners
            lightCameraOrthographicMin = minimize(tempPoint, lightCameraOrthographicMin);
            lightCameraOrthographicMax = maximize(tempPoint, lightCameraOrthographicMax);
        }

        vec4 sceneCenter = this->shadowBox.center();
        vec4 sceneExtents = this->shadowBox.extents();
        vec4 sceneAABBLightPoints[8];
        ComputeAABB(sceneAABBLightPoints, sceneCenter, sceneExtents);
        for (int index = 0; index < 8; ++index)
        {
            sceneAABBLightPoints[index] = lightView * sceneAABBLightPoints[index];
        }

        vec4 lightSpaceAABBMinValue = vec4(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
        vec4 lightSpaceAABBMaxValue = vec4(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);

        for (int index = 0; index < 8; ++index)
        {
            lightSpaceAABBMinValue = minimize(sceneAABBLightPoints[index], lightSpaceAABBMinValue);
            lightSpaceAABBMaxValue = maximize(sceneAABBLightPoints[index], lightSpaceAABBMaxValue);
        }

        float nearPlane = lightSpaceAABBMaxValue.z;
        float farPlane = lightSpaceAABBMinValue.z;

        mat4 cascadeProjectionMatrix = orthooffcenterrh(lightCameraOrthographicMin.x,
                                                        lightCameraOrthographicMax.x,
                                                        lightCameraOrthographicMax.y,
                                                        lightCameraOrthographicMin.y,
                                                        nearPlane,
                                                        farPlane);

        this->intervalDistances[cascadeIndex] = intervalEnd;
        this->cascadeProjectionTransform[cascadeIndex] = cascadeProjectionMatrix;
        this->cascadeViewProjectionTransform[cascadeIndex] = cascadeProjectionMatrix * lightView;
    }
    this->shadowView = lightView;
}


} // namespace Lighting
