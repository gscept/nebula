//------------------------------------------------------------------------------
//  csmutil.cc
//  (C) 2012-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "lighting/csmutil.h"
#include "graphics/camerasettings.h"
#include "coregraphics/rendershape.h"
#include "coregraphics/shaperenderer.h"
#include "threading/thread.h"
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
CSMUtil::CSMUtil() :
    cascadeMaxDistance(300),
    fittingMethod(Scene),
    clampingMethod(AABB),
    blurSize(1),
    floorTexels(true),
    textureWidth(1024)
{
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
CSMUtil::ComputeFrustumPoints( float cascadeBegin, float cascadeEnd, const Math::mat4& projection, Math::vec4* frustumCorners )
{
    // frustum corners in projection space
    frustumCorners[0].set(1.0f, 1.0f, cascadeBegin, 1.0f);
    frustumCorners[1].set(1.0f, -1.0f, cascadeBegin, 1.0f);
    frustumCorners[2].set(-1.0f, -1.0f, cascadeBegin, 1.0f);
    frustumCorners[3].set(-1.0f, 1.0f, cascadeBegin, 1.0f);

    frustumCorners[4].set(1.0f, 1.0f, cascadeEnd, 1.0f);
    frustumCorners[5].set(-1.0f, 1.0f, cascadeEnd, 1.0f);
    frustumCorners[6].set(-1.0f, -1.0f, cascadeEnd, 1.0f);
    frustumCorners[7].set(1.0f, -1.0f, cascadeEnd, 1.0f);

    // compute frustum corners in world space
    IndexT i;
    for (i = 0; i < 8; i++)
    {
        frustumCorners[i] = Math::point(projection * frustumCorners[i]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
CSMUtil::ComputeNearAndFar(float& nearPlane, float& farPlane, const Math::vec4& lightCameraOrtoMin, const Math::vec4& lightCameraOrtoMax, const Math::vec4* lightAABBPoints)
{
    nearPlane = FLT_MAX;
    farPlane = -FLT_MAX; 

    Triangle triangleList[16];
    int triCount = 1;

    triangleList[0].pt[0] = lightAABBPoints[0];
    triangleList[0].pt[1] = lightAABBPoints[1];
    triangleList[0].pt[2] = lightAABBPoints[2];
    triangleList[0].culled = false;

    // These are the indices used to tessellate an AABB into a list of triangles.
    static const int AABBTriIndices[] = 
    {
        0,1,2,  1,2,3,
        4,5,6,  5,6,7,
        0,2,4,  2,4,6,
        1,3,5,  3,5,7,
        0,1,4,  1,4,5,
        2,3,6,  3,6,7 
    };

    int pointPassesCollision[3];

    // At a high level: 
    // 1. Iterate over all 12 triangles of the AABB.  
    // 2. Clip the triangles against each plane. Create new triangles as needed.
    // 3. Find the min and max z values as the near and far plane.

    //This is easier because the triangles are in camera spacing making the collisions tests simple comparisons.
    float lightCameraOrthoMinX = lightCameraOrtoMin.x;
    float lightCameraOrthoMaxX = lightCameraOrtoMax.x;
    float lightCameraOrthoMinY = lightCameraOrtoMin.y;
    float lightCameraOrthoMaxY = lightCameraOrtoMax.y;

    for (int aabbTriIterator = 0; aabbTriIterator < 12; ++aabbTriIterator)
    {
        triangleList[0].pt[0] = lightAABBPoints[ AABBTriIndices[aabbTriIterator*3 + 0] ];
        triangleList[0].pt[1] = lightAABBPoints[ AABBTriIndices[aabbTriIterator*3 + 1] ];
        triangleList[0].pt[2] = lightAABBPoints[ AABBTriIndices[aabbTriIterator*3 + 2] ];
        triangleList[0].culled = false;

        triCount = 1;

        for (int frustumPlaneIndex = 0; frustumPlaneIndex < 4; ++frustumPlaneIndex)
        {
            float edge;
            int component;

            if (frustumPlaneIndex == 0)
            {
                edge = lightCameraOrthoMinX;
                component = 0;
            }
            else if (frustumPlaneIndex == 1)
            {
                edge = lightCameraOrthoMaxX;
                component = 0;
            }
            else if (frustumPlaneIndex == 2)
            {
                edge = lightCameraOrthoMinY;
                component = 1;
            }
            else
            {
                edge = lightCameraOrthoMaxY;
                component = 1;
            }

            for (int triIndex = 0; triIndex < triCount; ++triIndex)
            {
                if (!triangleList[triIndex].culled)
                {
                    int insideVertCount = 0;
                    vec4 tempOrder;

                    if (frustumPlaneIndex == 0)
                    {
                        for (int triPtIndex = 0; triPtIndex < 3; ++triPtIndex)
                        {
                            if ( triangleList[triIndex].pt[triPtIndex].x > lightCameraOrtoMin.x )
                            {
                                pointPassesCollision[triPtIndex] = 1;
                            }
                            else
                            {
                                pointPassesCollision[triPtIndex] = 0;
                            }
                            insideVertCount += pointPassesCollision[triPtIndex];
                        }
                    }
                    else if (frustumPlaneIndex == 1)
                    {
                        for (int triPtIndex = 0; triPtIndex < 3; ++triPtIndex)
                        {
                            if ( triangleList[triIndex].pt[triPtIndex].x < lightCameraOrtoMax.x )
                            {
                                pointPassesCollision[triPtIndex] = 1;
                            }
                            else
                            {
                                pointPassesCollision[triPtIndex] = 0;
                            }
                            insideVertCount += pointPassesCollision[triPtIndex];
                        }
                    }
                    else if (frustumPlaneIndex == 2)
                    {
                        for (int triPtIndex = 0; triPtIndex < 3; ++triPtIndex)
                        {
                            if ( triangleList[triIndex].pt[triPtIndex].y > lightCameraOrtoMin.y )
                            {
                                pointPassesCollision[triPtIndex] = 1;
                            }
                            else
                            {
                                pointPassesCollision[triPtIndex] = 0;
                            }
                            insideVertCount += pointPassesCollision[triPtIndex];
                        }
                    }
                    else
                    {
                        for (int triPtIndex = 0; triPtIndex < 3; ++triPtIndex)
                        {
                            if ( triangleList[triIndex].pt[triPtIndex].y < lightCameraOrtoMax.y )
                            {
                                pointPassesCollision[triPtIndex] = 1;
                            }
                            else
                            {
                                pointPassesCollision[triPtIndex] = 0;
                            }
                            insideVertCount += pointPassesCollision[triPtIndex];
                        }
                    }

                    if (pointPassesCollision[1] && !pointPassesCollision[0])
                    {
                        tempOrder = triangleList[triIndex].pt[0];
                        triangleList[triIndex].pt[0] = triangleList[triIndex].pt[1];
                        triangleList[triIndex].pt[1] = tempOrder;
                        pointPassesCollision[0] = true;
                        pointPassesCollision[1] = false;
                    }
                    if (pointPassesCollision[2] && !pointPassesCollision[1])
                    {
                        tempOrder = triangleList[triIndex].pt[1];
                        triangleList[triIndex].pt[1] = triangleList[triIndex].pt[2];
                        triangleList[triIndex].pt[2] = tempOrder;
                        pointPassesCollision[1] = true;
                        pointPassesCollision[2] = false;
                    }
                    if (pointPassesCollision[1] && !pointPassesCollision[0])
                    {
                        tempOrder = triangleList[triIndex].pt[0];
                        triangleList[triIndex].pt[0] = triangleList[triIndex].pt[1];
                        triangleList[triIndex].pt[1] = tempOrder;
                        pointPassesCollision[0] = true;
                        pointPassesCollision[1] = false;
                    }

                    if ( insideVertCount == 0)
                    {
                        triangleList[triIndex].culled = true;
                    }
                    else if (insideVertCount == 1)
                    {
                        triangleList[triIndex].culled = false;

                        vec4 vert0ToVert1 = triangleList[triIndex].pt[1] - triangleList[triIndex].pt[0];
                        vec4 vert0ToVert2 = triangleList[triIndex].pt[2] - triangleList[triIndex].pt[0];

                        float hitPointTimeRatio = edge - triangleList[triIndex].pt[0][component];

                        float distanceAlong01 = hitPointTimeRatio / vert0ToVert1[component];
                        float distanceAlong02 = hitPointTimeRatio / vert0ToVert2[component];

                        vert0ToVert1 *= distanceAlong01;
                        vert0ToVert1 += triangleList[triIndex].pt[0];
                        vert0ToVert2 *= distanceAlong02;
                        vert0ToVert2 += triangleList[triIndex].pt[0];

                        triangleList[triIndex].pt[1] = vert0ToVert2;
                        triangleList[triIndex].pt[2] = vert0ToVert1;
                    }
                    else if (insideVertCount == 2)
                    {
                        triangleList[triCount] = triangleList[triIndex+1];

                        triangleList[triIndex].culled = false;
                        triangleList[triIndex+1].culled = false;

                        vec4 vert2ToVert0 = triangleList[triIndex].pt[0] - triangleList[triIndex].pt[2];
                        vec4 vert2ToVert1 = triangleList[triIndex].pt[1] - triangleList[triIndex].pt[2];

                        float hitPointTime20 = edge - triangleList[triIndex].pt[2][component];
                        float distanceAlong20 = hitPointTime20 / vert2ToVert0[component];

                        vert2ToVert0 *= distanceAlong20;
                        vert2ToVert0 += triangleList[triIndex].pt[2];

                        triangleList[triIndex+1].pt[0] = triangleList[triIndex].pt[0];
                        triangleList[triIndex+1].pt[1] = triangleList[triIndex].pt[1];
                        triangleList[triIndex+1].pt[2] = vert2ToVert0;

                        float hitPointTime21 = edge - triangleList[triIndex].pt[2][component];
                        float distanceAlong21 = hitPointTime21 / vert2ToVert1[component];

                        vert2ToVert1 *= distanceAlong21;
                        vert2ToVert1 += triangleList[triIndex].pt[2];

                        triangleList[triIndex].pt[0] = triangleList[triIndex+1].pt[1];
                        triangleList[triIndex].pt[1] = triangleList[triIndex+1].pt[2];
                        triangleList[triIndex].pt[2] = vert2ToVert1;

                        ++triCount;
                        ++triIndex;
                    }
                    else
                    {
                        triangleList[triIndex].culled = false;
                    }
                }
            }
        }
        for (int index = 0; index < triCount; ++index)
        {
            if (!triangleList[index].culled)
            {
                for (int vertexIndex = 0; vertexIndex < 3; ++vertexIndex)
                {
                    float triangleCoordZ = triangleList[index].pt[vertexIndex].z;
                    if (nearPlane > triangleCoordZ)
                    {
                        nearPlane = triangleCoordZ;
                    }
                    if (farPlane < triangleCoordZ)
                    {
                        farPlane = triangleCoordZ;
                    }
                }
            }
        }
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
    mat4 cameraProjection = Graphics::CameraContext::GetProjection(camera);
    mat4 cameraView = inverse(Graphics::CameraContext::GetTransform(camera));

    // get inversed shadow matrix, this is basically the global light transform, normalized and inversed
    mat4 lightView = inverse(Lighting::LightContext::GetTransform(light));
    
    // calculate light AABB based on the AABB of the scene
    vec4 sceneCenter = this->shadowBox.center();
    vec4 sceneExtents = this->shadowBox.extents();
    vec4 sceneAABBLightPoints[8];
    this->ComputeAABB(sceneAABBLightPoints, sceneCenter, sceneExtents);
    for (int index = 0; index < 8; ++index)
    {
        sceneAABBLightPoints[index] = lightView * sceneAABBLightPoints[index];
    }

    float intervalStart, intervalEnd;
    vec4 lightCameraOrthographicMin;
    vec4 lightCameraOrthographicMax;

    // calculate near and far range based on scene bounding box
    //float nearFarRange = camSettings.GetZFar() - camSettings.GetZNear();
    float nearFarRange = n_min(this->shadowBox.diagonal_size() / 2, 300.0f);
    vec4 unitsPerTexel = vec4(0,0,0,0);

    for (int cascadeIndex = 0; cascadeIndex < NumCascades; ++cascadeIndex)
    {
        if (fittingMethod == Cascade)
        {
            if (cascadeIndex == 0) intervalStart = 0;
            else intervalStart = cascadeDistances[cascadeIndex-1];
        }
        else
        {
            intervalStart = 0;
        }

        intervalStart /= cascadeMaxDistance;
        intervalStart *= nearFarRange;

        intervalEnd = cascadeDistances[cascadeIndex];
        intervalEnd /= cascadeMaxDistance;
        intervalEnd *= nearFarRange;

        vec4 frustumPoints[8];
        this->ComputeFrustumPoints(intervalStart, intervalEnd, cameraProjection, frustumPoints);
        lightCameraOrthographicMin = vec4(FLT_MAX);
        lightCameraOrthographicMax = vec4(-FLT_MAX);

        vec4 tempCornerPoint;
        for (int icpIndex = 0; icpIndex < 8; ++icpIndex)
        {
            frustumPoints[icpIndex] = cameraView * frustumPoints[icpIndex];
            tempCornerPoint = lightView * frustumPoints[icpIndex];
            
            lightCameraOrthographicMin = minimize(tempCornerPoint, lightCameraOrthographicMin);
            lightCameraOrthographicMax = maximize(tempCornerPoint, lightCameraOrthographicMax);
        }

        if (this->fittingMethod == Scene)
        {
            vec4 diagonal = frustumPoints[0] - frustumPoints[6];
            float length = length3(diagonal);
            diagonal = vec4(length);

            vec4 borderOffset = (diagonal - (lightCameraOrthographicMax - lightCameraOrthographicMin)) * halfVector;
            borderOffset *= multiplyZWToZero;

            lightCameraOrthographicMax += borderOffset;
            lightCameraOrthographicMin -= borderOffset;

            float ratio = length / (float)this->textureWidth;
            unitsPerTexel = vec4(ratio, ratio, 0, 0);
        }
        else if (this->fittingMethod == Cascade)
        {
            float scaleDueto = ((float)blurSize * 2 + 1) / ((float)this->textureWidth);
            vec4 scaleDuetoBlur = vec4(scaleDueto, scaleDueto, 0, 0);

            float normalizedBufferSize = 1/(float)this->textureWidth;
            vec4 normalizedBufferVector = vec4(normalizedBufferSize, normalizedBufferSize, 0, 0);

            vec4 borderOffset = lightCameraOrthographicMax - lightCameraOrthographicMin;
            borderOffset *= halfVector;
            borderOffset *= scaleDuetoBlur;

            lightCameraOrthographicMax += borderOffset;
            lightCameraOrthographicMin -= borderOffset;

            unitsPerTexel = lightCameraOrthographicMax - lightCameraOrthographicMin;
            unitsPerTexel *= normalizedBufferVector;
        }

        if (this->floorTexels)
        {
            lightCameraOrthographicMin /= unitsPerTexel;
            lightCameraOrthographicMin = floor(lightCameraOrthographicMin);
            lightCameraOrthographicMin *= unitsPerTexel;

            lightCameraOrthographicMax /= unitsPerTexel;
            lightCameraOrthographicMax = floor(lightCameraOrthographicMax);
            lightCameraOrthographicMax *= unitsPerTexel;
        }       

        float nearPlane = -intervalEnd;
        float farPlane = intervalEnd;

        if (this->clampingMethod == AABB)
        {
            vec4 lightSpaceAABBMinValue = vec4(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
            vec4 lightSpaceAABBMaxValue = vec4(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);

            for (int index = 0; index < 8; ++index)
            {
                lightSpaceAABBMinValue = minimize(sceneAABBLightPoints[index], lightSpaceAABBMinValue);
                lightSpaceAABBMaxValue = maximize(sceneAABBLightPoints[index], lightSpaceAABBMaxValue);
            }

            nearPlane = lightSpaceAABBMinValue.z;
            farPlane = lightSpaceAABBMaxValue.z;
        }
        else if (this->clampingMethod == SceneAABB)
        {
            this->ComputeNearAndFar(nearPlane, farPlane, lightCameraOrthographicMin, lightCameraOrthographicMax, sceneAABBLightPoints);
        }
        else
        {
            // fit zero-one
        }

        // okay, so making this matrix an LH matrix apparently works, algorithm assumes a DX handed mode
        mat4 cascadeProjectionMatrix = orthooffcenterlh(lightCameraOrthographicMin.x,
                                                                      lightCameraOrthographicMax.x,
                                                                      lightCameraOrthographicMin.y,
                                                                      lightCameraOrthographicMax.y,
                                                                      nearPlane, farPlane);         

        this->intervalDistances[cascadeIndex] = intervalEnd;
        this->cascadeProjectionTransform[cascadeIndex] = cascadeProjectionMatrix;
        this->cascadeViewProjectionTransform[cascadeIndex] = lightView * cascadeProjectionMatrix;
    }
    this->shadowView = lightView;
}

//------------------------------------------------------------------------------
/**
*/
void
CSMUtil::ComputeAABB(vec4* lightAABBPoints, const vec4& sceneCenter, const vec4& sceneExtents)
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


} // namespace Lighting
